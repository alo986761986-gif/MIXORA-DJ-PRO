#include "../audio/LiveEngine.h"
#include "../audio/Wav.h"
#include "../audio/Resample.h"
#include "../audio/DecodeQt.h"
#include "MixoraWaveform.h"
#include "../midi/MidiMapping.h"
#include "../midi/DeviceIdentity.h"
#include "../midi/WinMidiInput.h"
#include "../midi/WinMidiOutput.h"
#include <QSettings>
#include <QSignalBlocker>
#include <QApplication>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QIODevice>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QMessageBox>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QLineEdit>
#include <QDirIterator>
#include <QDir>
#include <QGroupBox>
#include <QByteArray>
#include <QCheckBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <exception>
#include <cmath>
#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

// QAudioSink pulls interleaved float32 little-endian PCM from this device.
// One shared callback processes both decks. Must be accessed on GUI/audio Qt thread only.
class PCMDevice : public QIODevice {
public:
 explicit PCMDevice(mixora::LiveEngine* engine,bool fourChannels,QObject* parent=nullptr):QIODevice(parent),engine_(engine),fourChannels_(fourChannels){}
 bool isSequential()const override{return true;}
 qint64 bytesAvailable()const override{return 65536+QIODevice::bytesAvailable();}
protected:
 qint64 readData(char* dest,qint64 maxlen)override{
  if(!engine_||!dest||maxlen<=0)return 0;
  const size_t frameBytes=fourChannels_?16:8;
  const size_t frames=static_cast<size_t>(maxlen)/frameBytes;
  if(!frames)return 0;
  masterBuffer_.resize(frames);cueBuffer_.resize(frames);
  engine_->processBuses(masterBuffer_.data(),cueBuffer_.data(),frames);
  static_assert(sizeof(mixora::StereoFrame)==8,"StereoFrame must be 2x float32");
  if(fourChannels_){
   interleaved_.resize(frames*4);
   for(size_t i=0;i<frames;++i){
    interleaved_[i*4]=masterBuffer_[i].left;
    interleaved_[i*4+1]=masterBuffer_[i].right;
    interleaved_[i*4+2]=cueBuffer_[i].left;
    interleaved_[i*4+3]=cueBuffer_[i].right;
   }
   std::memcpy(dest,interleaved_.data(),frames*frameBytes);
  }else std::memcpy(dest,masterBuffer_.data(),frames*frameBytes);
  return static_cast<qint64>(frames*frameBytes);
 }
 qint64 writeData(const char*,qint64)override{return -1;}
private:
 mixora::LiveEngine* engine_;
 bool fourChannels_;
 std::vector<mixora::StereoFrame> masterBuffer_,cueBuffer_;
 std::vector<float> interleaved_;
};

class LiveWindow : public QMainWindow {
public:
 ~LiveWindow() override {
  // Only send on explicit opt-in; never touch unknown devices by default.
  clearSyncLeds();
  midiOutput_.close();
  midiInput_.close();
  if(sink_)sink_->stop();
 }
 LiveWindow(){
  setAcceptDrops(true);
  setWindowTitle("MIXORA DJ PRO 2.6 | PERFORMANCE EDITION");
  setMinimumSize(990,650);resize(1460,880);
  setStyleSheet(R"CSS(
QWidget{background:#10141b;color:#dce5ee;font:12px 'Segoe UI';}
QLabel{background:transparent;}
QGroupBox{background:#1a232e;border:1px solid #314253;border-radius:7px;margin-top:12px;padding:8px;}
QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 5px;color:#a9b9cb;}
QPushButton{background:#263747;border:1px solid #3c5266;border-radius:5px;padding:7px;font-weight:600;}
QPushButton:checked{background:#166f9c;color:#fff;border-color:#4cc7fa;}
QPushButton:disabled{background:#222a33;color:#637585;}
QPushButton:hover{background:#354b5f;}
QSlider::groove:horizontal{background:#36465a;height:6px;border-radius:3px;}
QSlider::handle:horizontal{background:#acd7ed;width:14px;margin:-5px 0;border-radius:7px;}
QComboBox,QLineEdit{background:#1f2c39;border:1px solid #425568;border-radius:4px;padding:5px;}
QTreeWidget{background:#121b25;border:1px solid #293949;alternate-background-color:#1c2835;selection-background-color:#136b9c;}
QHeaderView::section{background:#293a49;border:0;border-right:1px solid #405366;padding:6px;}
)CSS");
  auto* central=new QWidget;setCentralWidget(central);
   auto* root=new QVBoxLayout(central);root->setContentsMargins(10,7,10,7);root->setSpacing(7);
   auto* topbar=new QHBoxLayout;
   auto* title=new QLabel("MIXORA  DJ PRO");title->setStyleSheet("font-size:25px;font-weight:800;color:#55c9fb;");
   topbar->addWidget(title);
   auto* subtitle=new QLabel("  PERFORMANCE EDITION / DECKS A + B");
   subtitle->setStyleSheet("font-size:10px;color:#859aad;font-weight:700;");
   topbar->addWidget(subtitle);topbar->addStretch();
   auto* advancedToggle=new QPushButton("AUDIO / MIDI SETTINGS");advancedToggle->setCheckable(true);topbar->addWidget(advancedToggle);
   root->addLayout(topbar);
   auto* advancedPanel=new QWidget;auto* advancedLayout=new QVBoxLayout(advancedPanel);
   advancedPanel->setVisible(false);
   connect(advancedToggle,&QPushButton::toggled,advancedPanel,&QWidget::setVisible);
   auto* outputRow=new QHBoxLayout;outputRow->addWidget(new QLabel("USCITA MASTER"));outputs_=new QComboBox;outputRow->addWidget(outputs_,1);
   auto* refresh=new QPushButton("AGGIORNA DISPOSITIVI");outputRow->addWidget(refresh);advancedLayout->addLayout(outputRow);
   auto* routingRow=new QHBoxLayout;
   routingRow->addWidget(new QLabel("ROUTING"));
   routing_=new QComboBox;
   routing_->addItem("Stereo Master 1/2 (nessun Cue fisico)",false);
   routing_->addItem("Quad 1/2 Master + 3/4 Cue (richiede device 4ch)",true);
   routingRow->addWidget(routing_,1);
   routingRow->addWidget(new QLabel("CUE VOL"));
   cueVolume_=new QSlider(Qt::Horizontal);cueVolume_->setRange(0,100);cueVolume_->setValue(70);
   routingRow->addWidget(cueVolume_);advancedLayout->addLayout(routingRow);
   connect(cueVolume_,&QSlider::valueChanged,this,[this](int v){if(engine_)engine_->setCueGain(v/100.);});
   connect(refresh,&QPushButton::clicked,this,[this]{refreshDevices();});refreshDevices();
   auto* deckRow=new QHBoxLayout;deckRow->setSpacing(8);
for(int d=0;d<2;++d){
   auto* group=new QGroupBox(d==0?"DECK A":"DECK B");auto* v=new QVBoxLayout(group);
   auto* tools=new QHBoxLayout;auto* load=new QPushButton("CARICA BRANO");tools->addWidget(load);
   names_[d]=new QLabel("Nessun file");tools->addWidget(names_[d],1);
   play_[d]=new QPushButton("PLAY");play_[d]->setEnabled(false);tools->addWidget(play_[d]);
   cueButtons_[d]=new QPushButton("CUE OFF");cueButtons_[d]->setCheckable(true);
   cueButtons_[d]->setEnabled(false);tools->addWidget(cueButtons_[d]);
   connect(cueButtons_[d],&QPushButton::toggled,this,[this,d](bool on){
    cueButtons_[d]->setText(on?"CUE ON":"CUE OFF");
    if(engine_)engine_->setCue(d,on);
   });
   v->addLayout(tools);
   auto* knobs=new QHBoxLayout;
   const char* captions[]={"GAIN","LOW","MID","HIGH"};
   for(int k=0;k<4;++k){
    auto* groupKnob=new QVBoxLayout;groupKnob->addWidget(new QLabel(captions[k]));
    auto* slider=new QSlider(Qt::Horizontal);slider->setRange(k==0?0:-18,k==0?200:18);slider->setValue(k==0?100:0);
    slider->setMinimumWidth(105);groupKnob->addWidget(slider);controls_[d][k]=slider;knobs->addLayout(groupKnob);
    connect(slider,&QSlider::valueChanged,this,[this,d,k](int){applyDeck(d,k);});
   }
   v->addLayout(knobs);
   auto* beatRow=new QHBoxLayout;
   bpmLabels_[d]=new QLabel("BPM: --");beatRow->addWidget(bpmLabels_[d]);
   syncButtons_[d]=new QPushButton("SYNC OFF");syncButtons_[d]->setCheckable(true);syncButtons_[d]->setEnabled(false);
   beatRow->addWidget(syncButtons_[d]);beatRow->addStretch();v->addLayout(beatRow);
   connect(syncButtons_[d],&QPushButton::toggled,this,[this,d](bool enabled){
    if(!engine_)return;
    if(enabled){
     try{engine_->synchronize(d,1-d);syncButtons_[d]->setText("SYNC ON (pitch varia)");}
     catch(const std::exception& ex){syncButtons_[d]->setChecked(false);status_->setText(QString::fromUtf8(ex.what()));}
    }else{engine_->setSync(d,false);syncButtons_[d]->setText("SYNC OFF");}
   });
   auto* transport=new QHBoxLayout;
   cuePointButtons_[d]=new QPushButton("CUE POINT");cuePointButtons_[d]->setEnabled(false);
   transport->addWidget(cuePointButtons_[d]);
   loopButtons_[d]=new QPushButton("LOOP 4 BEATS");loopButtons_[d]->setCheckable(true);loopButtons_[d]->setEnabled(false);
   transport->addWidget(loopButtons_[d]);
   for(int slot=0;slot<4;++slot){
    auto* hc=new QPushButton(QString("HOT %1").arg(slot+1));hc->setEnabled(false);hotButtons_[d][slot]=hc;
    transport->addWidget(hc);
    connect(hc,&QPushButton::clicked,this,[this,d,slot]{if(!engine_)return;
     try{
      // Shift+click writes cue; normal click jumps to its saved position.
      if(QApplication::keyboardModifiers()&Qt::ShiftModifier){engine_->setHotCue(d,slot);hotButtons_[d][slot]->setText(QString("HOT %1 *").arg(slot+1));}
      else if(!engine_->triggerHotCue(d,slot))status_->setText("SHIFT+clic per salvare un Hot Cue");
     }catch(const std::exception& ex){status_->setText(ex.what());}
    });
   }
   connect(cuePointButtons_[d],&QPushButton::clicked,this,[this,d]{if(!engine_)return;engine_->seek(d,0);updateProgress();});
   connect(loopButtons_[d],&QPushButton::toggled,this,[this,d](bool on){
    if(!engine_)return;
    if(!on){engine_->clearLoop(d);loopButtons_[d]->setText("LOOP 4 BEATS");return;}
    const auto info=engine_->beatAnalysis(d);
    if(info.bpm<=0){loopButtons_[d]->setChecked(false);status_->setText("Loop: BPM non disponibile");return;}
    const size_t duration=std::max<size_t>(1,size_t(std::llround(4.*60./info.bpm*engine_->sampleRate())));
    const size_t start=engine_->position(d),end=(std::min)(engine_->length(d),start+duration);
    if(end<=start){loopButtons_[d]->setChecked(false);return;}
    engine_->setLoop(d,start,end);loopButtons_[d]->setText("LOOP ON");
   });
   v->addLayout(transport);
   root->addWidget(group);
   connect(load,&QPushButton::clicked,this,[this,d]{loadDeck(d);});
   connect(play_[d],&QPushButton::clicked,this,[this,d]{if(!engine_)return;
    if(engine_->position(d)>=engine_->length(d))engine_->seek(d,0);
    engine_->setPlaying(d,!engine_->playing(d));updatePlayButtons();
   });
  }
  auto* masterRow=new QHBoxLayout;masterRow->addWidget(new QLabel("CROSSFADER  A — B"));cross_=new QSlider(Qt::Horizontal);cross_->setRange(0,100);cross_->setValue(50);masterRow->addWidget(cross_,2);
  masterRow->addWidget(new QLabel("MASTER"));master_=new QSlider(Qt::Horizontal);master_->setRange(0,100);master_->setValue(75);masterRow->addWidget(master_,1);root->addLayout(masterRow);
  connect(cross_,&QSlider::valueChanged,this,[this](int x){if(engine_)engine_->setCrossfader(x/100.);});
  connect(master_,&QSlider::valueChanged,this,[this](int x){if(engine_)engine_->setMaster(x/100.);});
  auto* midiRow=new QHBoxLayout;
  midiRow->addWidget(new QLabel("MIDI LEARN (porta dedicata)"));
  midiTarget_=new QComboBox;
  const QStringList targets={"play_a","play_b","sync_a","sync_b","cue_a","cue_b","cross","master","gain_a","gain_b","low_a","mid_a","high_a","low_b","mid_b","high_b","cue_vol","cuepoint_a","cuepoint_b","loop_a","loop_b","hot1_a","hot2_a","hot3_a","hot4_a","hot1_b","hot2_b","hot3_b","hot4_b"};
  for(const auto& key:targets)midiTarget_->addItem(key,key);
  midiRow->addWidget(midiTarget_,1);
  midiRow->addWidget(new QLabel("PORTA PER LEARN"));
  midiLearnPort_=new QComboBox; midiLearnPort_->setMinimumWidth(175); midiRow->addWidget(midiLearnPort_);
  auto* learn=new QPushButton("IMPARA CONTROLLO");learn->setCheckable(true);midiLearnButton_=learn;
  midiRow->addWidget(learn);
  auto* midiRefresh=new QPushButton("RICOLLEGA MIDI");midiRow->addWidget(midiRefresh);
  root->addLayout(midiRow);
  connect(learn,&QPushButton::toggled,this,[this,learn](bool checked){learning_=checked;learn->setText(checked?"MUOVI UN CONTROLLO...":"IMPARA CONTROLLO");});
  connect(midiRefresh,&QPushButton::clicked,this,[this]{clearSyncLeds();midiOutput_.close();ledEnabled_->setChecked(false);midiInput_.openAll();refreshMidiInputPorts();refreshMidiOutputPorts();midiStatus_->setText(QString("Ingressi MIDI aperti: %1. Uscita LED disattivata.").arg(midiInput_.portsOpen()));});
  midiStatus_=new QLabel; midiStatus_->setWordWrap(true);
  // No presumed factory mapping: X1 and Z1 must be learned on their own ports.
  restoreBindings();midiInput_.openAll();refreshMidiInputPorts();
  midiStatus_->setText(QString("Porte MIDI aperte: %1. Z1 e X1 richiedono test fisico/mapping.").arg(midiInput_.portsOpen()));
  root->addWidget(midiStatus_);
  auto* midiInfo=new QLabel; midiInfo->setWordWrap(true);
#ifdef Q_OS_WIN
  QStringList midiNames;
  for(UINT i=0;i<midiInGetNumDevs();++i){MIDIINCAPSW caps{};if(midiInGetDevCapsW(i,&caps,sizeof(caps))==MMSYSERR_NOERROR)midiNames<<QString::fromWCharArray(caps.szPname);}
  midiInfo->setText("Porte MIDI Windows rilevate (non garantisce compatibilita controller): "+(midiNames.isEmpty()?QString("nessuna"):midiNames.join(" | ")));
#else
  midiInfo->setText("Diagnostica porte MIDI disponibile nelle build Windows.");
#endif
  root->addWidget(midiInfo);
  auto* ledRow=new QHBoxLayout;
  ledRow->addWidget(new QLabel("MIDI OUT (LED sperimentali)"));
  midiOutputPorts_=new QComboBox;ledRow->addWidget(midiOutputPorts_,1);
  ledEnabled_=new QCheckBox("ABILITA LED CC42/43 (solo se verificati)");ledRow->addWidget(ledEnabled_);
  root->addLayout(ledRow);
  refreshMidiOutputPorts();
  connect(ledEnabled_,&QCheckBox::toggled,this,[this](bool checked){
   if(!checked){clearSyncLeds();midiOutput_.close();return;}
   if(midiOutputPorts_->currentIndex()<0 || !midiOutput_.open(midiOutputPorts_->currentData().toUInt())){
    QSignalBlocker blocker(ledEnabled_);ledEnabled_->setChecked(false);
    midiStatus_->setText("Apertura MIDI OUT fallita. Controlla driver e modalita MIDI.");return;
   }
   sendSyncLeds();
  });
  connect(midiOutputPorts_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int){if(ledEnabled_->isChecked())ledEnabled_->setChecked(false);});
  for(int d=0;d<2;++d)connect(syncButtons_[d],&QPushButton::toggled,this,[this](bool){sendSyncLeds();});
  auto* stop=new QPushButton("STOP DECK A + B");root->addWidget(stop);
  connect(stop,&QPushButton::clicked,this,[this]{if(!engine_)return;for(int d=0;d<2;++d){engine_->setPlaying(d,false);engine_->seek(d,0);}updatePlayButtons();});
  status_=new QLabel("Carica il primo brano per inizializzare l'audio.");status_->setWordWrap(true);root->addWidget(status_);
  auto* warning=new QLabel("ATTENZIONE: prototipo realtime: MP3/FLAC dipendono dai codec Qt. Per il Cue reale seleziona 4 canali e una scheda con uscite 1/2 e 3/4 indipendenti. Non provato con Z1. EQ non anti-click, nessun limiter true-peak: non usare durante esibizioni.");warning->setWordWrap(true);warning->setStyleSheet("color:#e3b77b");root->addWidget(warning);
  progress_=new QLabel("A 00:00 / 00:00     |     B 00:00 / 00:00");root->addWidget(progress_);
  auto* timer=new QTimer(this);timer->setInterval(30);connect(timer,&QTimer::timeout,this,[this]{pollMidi();});timer->start();
  auto* uiTimer=new QTimer(this);uiTimer->setInterval(200);connect(uiTimer,&QTimer::timeout,this,[this]{updatePlayButtons();updateProgress();});uiTimer->start();
 }
private:
 void restoreBindings(){
  QSettings settings("MIXORA","DJ PRO");
  QSettings oldSettings("MIXORA","DJ PRO 2.4");
  // Read old identity-bound profiles only when the new app has no saved bindings.
  const bool migrate=!settings.contains("midi/bindings_v2/size") && oldSettings.contains("midi/bindings_v2/size");
  QSettings& source=migrate?oldSettings:settings;
  const int count=source.beginReadArray("midi/bindings_v2");
  for(int i=0;i<count;++i){
   source.setArrayIndex(i);
   const auto control=source.value("control").toString();
   const auto device=source.value("device").toString();
   bool ok1=false,ok2=false;
   int status=source.value("status").toInt(&ok1),data=source.value("data1").toInt(&ok2);
   if(!control.isEmpty() && !device.isEmpty() && ok1 && ok2 && status>=0 && status<=255 && data>=0 && data<=127)
    mapping_.bind({control.toStdString(),static_cast<uint8_t>(status),static_cast<uint8_t>(data),source.value("button").toBool(),mixora::kAnyMidiPort,device.toStdString()});
  }
  source.endArray();
  if(migrate)saveBindings();
  // Deliberately do not migrate port-number bindings: a reused WinMM ID could control the wrong deck.
 }
 void saveBindings(){
  QSettings settings("MIXORA","DJ PRO");
  settings.beginWriteArray("midi/bindings_v2");
  int i=0;
  for(const auto& b:mapping_.bindings()){
   if(b.deviceKey.empty())continue;
   settings.setArrayIndex(i++);
   settings.setValue("control",QString::fromStdString(b.control));
   settings.setValue("device",QString::fromStdString(b.deviceKey));
   settings.setValue("status",b.status);
   settings.setValue("data1",b.data1);
   settings.setValue("button",b.button);
  }
  settings.endArray();settings.sync();
 }
 std::string deviceKeyForPort(unsigned port) const {
  for(const auto& entry:midiInput_.availablePorts())if(entry.id==port)return entry.stableKey;
  return {};
 }
 void refreshMidiInputPorts(){
  if(!midiLearnPort_)return;
  const auto selected=midiLearnPort_->currentData().toString();
  QSignalBlocker blocker(midiLearnPort_);midiLearnPort_->clear();
  const auto ports=midiInput_.availablePorts();
  for(const auto& port:ports){
   const QString name=QString::fromStdString(port.name);
   midiLearnPort_->addItem(name+QString(" [porta %1]").arg(port.id),QString::fromStdString(port.stableKey));
  }
  const int index=midiLearnPort_->findData(selected);if(index>=0)midiLearnPort_->setCurrentIndex(index);
  // Identical device names are not guaranteed to preserve physical identity on replug.
  std::vector<std::string> keys;for(const auto& port:ports)keys.push_back(mixora::normalizeMidiName(port.name));
  std::sort(keys.begin(),keys.end());
  if(std::adjacent_find(keys.begin(),keys.end())!=keys.end() && midiStatus_)
   midiStatus_->setText("ATTENZIONE: dispositivi MIDI con nome identico. Dopo un riavvio verifica MIDI Learn: Windows puo invertire l'ordine.");
 }
 void refreshMidiOutputPorts(){
  if(!midiOutputPorts_)return;
  midiOutputPorts_->clear();
  for(const auto& port:midiOutput_.availablePorts())midiOutputPorts_->addItem(QString::fromStdString(port.name)+QString(" [ID %1]").arg(port.id),port.id);
 }
 void clearSyncLeds(){
  if(!midiOutput_.isOpen())return;
  midiOutput_.send(0xB0,42,0);
  midiOutput_.send(0xB0,43,0);
 }
 void sendSyncLeds(){
  if(!ledEnabled_ || !ledEnabled_->isChecked() || !midiOutput_.isOpen())return;
  // Experimental X1 CC numbers; hardware-specific confirmation is mandatory.
  for(int d=0;d<2;++d)midiOutput_.send(0xB0,static_cast<uint8_t>(d==0?42:43),syncButtons_[d]->isChecked()?127:0);
 }
 void pollMidi(){
  for(const auto& event:midiInput_.take()){
   const mixora::MidiMessage msg{event.status,event.data1,event.data2};
   if(!mixora::validMidiMessage(msg))continue;
   if(learning_){
    if(!midiLearnPort_ || midiLearnPort_->currentIndex()<0 || QString::fromStdString(deviceKeyForPort(event.device))!=midiLearnPort_->currentData().toString())continue;
    if(!mixora::midiOn(msg))continue;
    const auto control=midiTarget_->currentData().toString();
    if(deviceKeyForPort(event.device).empty())continue;
    const bool button=control.startsWith("play_")||control.startsWith("sync_")||control.startsWith("cue_")||control.startsWith("cuepoint_")||control.startsWith("loop_")||control.startsWith("hot");
    mapping_.bind({control.toStdString(),msg.status,msg.data1,button,mixora::kAnyMidiPort,deviceKeyForPort(event.device)});saveBindings();midiLearnButton_->setChecked(false);
    midiStatus_->setText(QString("Porta %1 associata a %2: status %3, data1 %4").arg(event.device).arg(control).arg(msg.status).arg(msg.data1));
    continue;
   }
   const auto* binding=mapping_.lookup(deviceKeyForPort(event.device),msg);if(!binding)continue;
   const QString target=QString::fromStdString(binding->control);
   if(binding->button&&!mixora::midiOn(msg))continue;
   if(target=="play_a"||target=="play_b"){
    const int d=target.endsWith('a')?0:1;if(play_[d]->isEnabled())play_[d]->click();
   }else if(target=="sync_a"||target=="sync_b"){
    const int d=target.endsWith('a')?0:1;if(syncButtons_[d]->isEnabled())syncButtons_[d]->click();
   }else if(target=="cue_a"||target=="cue_b"){
    const int d=target.endsWith('a')?0:1;if(cueButtons_[d]->isEnabled())cueButtons_[d]->click();
   }else if(target=="loop_a"||target=="loop_b"){
    const int d=target.endsWith('a')?0:1;if(loopButtons_[d]->isEnabled())loopButtons_[d]->click();
   }else if(target=="cuepoint_a"||target=="cuepoint_b"){
    const int d=target.endsWith('a')?0:1;if(cuePointButtons_[d]->isEnabled())cuePointButtons_[d]->click();
   }else if(target.startsWith("hot") && (target.endsWith("_a")||target.endsWith("_b"))){
    const int d=target.endsWith('a')?0:1;const int slot=target.mid(3,1).toInt()-1;
    if(slot>=0&&slot<4&&engine_){
     // MIDI hardware: first press stores a cue, later presses jump to it.
     if(!engine_->hasHotCue(d,slot)){engine_->setHotCue(d,slot);hotButtons_[d][slot]->setText(QString("HOT %1 *").arg(slot+1));}
     else engine_->triggerHotCue(d,slot);
    }
   }else{
    const int value=msg.data2;
    if(target=="cross")cross_->setValue((value*100+63)/127);
    else if(target=="master")master_->setValue((value*100+63)/127);
    else if(target=="cue_vol")cueVolume_->setValue((value*100+63)/127);
    else {
     for(int d=0;d<2;++d){const QString suffix=d==0?"_a":"_b";
      if(!target.endsWith(suffix))continue;
      const QString type=target.left(target.size()-2);
      int k=-1;if(type=="gain")k=0;else if(type=="low")k=1;else if(type=="mid")k=2;else if(type=="high")k=3;
      if(k>=0){auto* slider=controls_[d][k];const int scaled=slider->minimum()+((slider->maximum()-slider->minimum())*value+63)/127;slider->setValue(scaled);}
     }
    }
   }
  }
 }
 void refreshDevices(){
  const QByteArray previous=outputs_->currentData().toByteArray();outputs_->clear();
  for(const auto& device:QMediaDevices::audioOutputs())outputs_->addItem(device.description(),device.id());
  const int found=outputs_->findData(previous);if(found>=0)outputs_->setCurrentIndex(found);
 }
 void dragEnterEvent(QDragEnterEvent* e)override{
  if(e->mimeData()->hasUrls())e->acceptProposedAction();
 }
 void dropEvent(QDropEvent* e)override{
  for(const auto& url:e->mimeData()->urls())if(url.isLocalFile()){
   const int deck=engine_&&engine_->length(0)>0?1:0;
   loadDeckFile(deck,url.toLocalFile());e->acceptProposedAction();break;
  }
 }
 void loadDeck(int d){
  const QString file=QFileDialog::getOpenFileName(this,"Carica brano","","Audio (*.wav *.mp3 *.flac *.ogg *.m4a *.aac);;Tutti i file (*)");if(file.isEmpty())return;
  loadDeckFile(d,file);
 }
 void loadDeckFile(int d,const QString& file){
  try{
   // Entire file is decoded into RAM. This prototype is unsuitable for very long files.
   auto wav=mixora::decodeQtFile(file);
   if(!engine_){
    if(wav.sampleRate<16000)throw std::runtime_error("Il DSP richiede almeno 16 kHz");
    engine_=std::make_unique<mixora::LiveEngine>(48000);
    engine_->setCrossfader(cross_->value()/100.);engine_->setMaster(master_->value()/100.);engine_->setCueGain(cueVolume_->value()/100.);
   }
   if(wav.sampleRate!=engine_->sampleRate())wav=mixora::resampleLinear(wav,engine_->sampleRate());
   // Avoid rewriting the current deck while QAudioSink is pulling frames.
   if(sink_)sink_->suspend();
   // Analyze before moving file into playback engine. Long files are analyzed only for 2 minutes.
   const auto analysis=mixora::analyzeBeats(wav);
   engine_->load(d,std::move(wav));engine_->setBeatAnalysis(d,analysis);
   if(syncButtons_[d]->isChecked())syncButtons_[d]->setChecked(false);
   bpmLabels_[d]->setText(analysis.bpm>0?QString("BPM: %1 (stima %2%)").arg(analysis.bpm,0,'f',1).arg(int(analysis.confidence*100)):"BPM: non rilevato");
   syncButtons_[d]->setEnabled(analysis.bpm>0);
   for(int k=0;k<4;++k)applyDeck(d,k);
   names_[d]->setText(QFileInfo(file).fileName());play_[d]->setEnabled(true);cueButtons_[d]->setEnabled(true);engine_->setCue(d,cueButtons_[d]->isChecked());
   cuePointButtons_[d]->setEnabled(true);loopButtons_[d]->setChecked(false);loopButtons_[d]->setEnabled(analysis.bpm>0);
   for(int slot=0;slot<4;++slot){hotButtons_[d][slot]->setEnabled(true);hotButtons_[d][slot]->setText(QString("HOT %1").arg(slot+1));}
   if(!sink_)startAudio();else sink_->resume();
   status_->setText(QString("Caricato Deck %1. Premi PLAY per riprodurre.").arg(d?'B':'A'));
  }catch(const std::exception& ex){if(sink_&&sink_->state()==QAudio::SuspendedState)sink_->resume();QMessageBox::warning(this,"Impossibile caricare audio",QString::fromUtf8(ex.what()));}
 }
 void startAudio(){
  if(outputs_->currentIndex()<0)throw std::runtime_error("Nessuna uscita audio disponibile");
  QAudioDevice selected;bool found=false;
  for(const auto& dev:QMediaDevices::audioOutputs())if(dev.id()==outputs_->currentData().toByteArray()){selected=dev;found=true;break;}
  if(!found)throw std::runtime_error("Dispositivo audio non disponibile");
  QAudioFormat fmt;fmt.setSampleRate(static_cast<int>(engine_->sampleRate()));fmt.setChannelCount(routing_->currentData().toBool()?4:2);fmt.setSampleFormat(QAudioFormat::Float);
  if(!selected.isFormatSupported(fmt))throw std::runtime_error("Formato non supportato dal dispositivo: servono float32 e il numero di canali selezionato alla frequenza del WAV.");
  device_=std::make_unique<PCMDevice>(engine_.get(),routing_->currentData().toBool());device_->open(QIODevice::ReadOnly);
  sink_=std::make_unique<QAudioSink>(selected,fmt);
  sink_->setBufferSize(32768);
  sink_->start(device_.get());
  if(sink_->error()!=QAudio::NoError)throw std::runtime_error("Errore apertura uscita Qt Audio");
  outputs_->setEnabled(false);routing_->setEnabled(false); // changing routing requires restart
 }
 void applyDeck(int d,int){
  if(!engine_)return;
  engine_->setGain(d,controls_[d][0]->value()/100.);
  engine_->setEQ(d,controls_[d][1]->value(),controls_[d][2]->value(),controls_[d][3]->value());
 }
 void updatePlayButtons(){if(!engine_)return;for(int d=0;d<2;++d)play_[d]->setText(engine_->playing(d)?"PAUSA":"PLAY");}
 void updateProgress(){if(!engine_)return;auto time=[this](size_t frames){size_t sec=frames/engine_->sampleRate();return QString("%1:%2").arg(sec/60,2,10,QChar('0')).arg(sec%60,2,10,QChar('0'));};
  progress_->setText(QString("A %1 / %2     |     B %3 / %4").arg(time(engine_->position(0)),time(engine_->length(0)),time(engine_->position(1)),time(engine_->length(1))));
  if(sink_&&sink_->error()!=QAudio::NoError)status_->setText("Errore audio Qt: verifica dispositivo di uscita e riavvia il player.");
 }
 mixora::WinMidiInput midiInput_; mixora::WinMidiOutput midiOutput_; QComboBox* midiOutputPorts_=nullptr; QCheckBox* ledEnabled_=nullptr; mixora::MidiMapping mapping_; QComboBox* midiTarget_=nullptr; QComboBox* midiLearnPort_=nullptr; QLabel* midiStatus_=nullptr; QPushButton* midiLearnButton_=nullptr;bool learning_=false;
 QComboBox* outputs_=nullptr;
 QLabel* bpmLabels_[2]{};QPushButton* syncButtons_[2]{};
 QLabel* names_[2]{};QPushButton* play_[2]{};QPushButton* cueButtons_[2]{};QPushButton* cuePointButtons_[2]{};QPushButton* loopButtons_[2]{};QPushButton* hotButtons_[2][4]{};QSlider* controls_[2][4]{};
 QComboBox* routing_=nullptr;QSlider* cueVolume_=nullptr;
 QSlider* cross_=nullptr;QSlider* master_=nullptr;QLabel* status_=nullptr;QLabel* progress_=nullptr;
 std::unique_ptr<mixora::LiveEngine> engine_;
 std::unique_ptr<PCMDevice> device_;
 std::unique_ptr<QAudioSink> sink_;
};
int main(int argc,char** argv){
 QApplication app(argc,argv);
 LiveWindow w;
 // Automated Windows installer smoke test (constructs the real Qt window).
 if(app.arguments().contains("--smoke-test"))return 0;
 w.show();
 return app.exec();
}
