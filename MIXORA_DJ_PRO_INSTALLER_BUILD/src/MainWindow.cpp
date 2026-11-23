#include "MainWindow.h"
#include "DeckWidget.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QComboBox>
#include <QShortcut>
#include <QListWidget>
#include <QLineEdit>
#include <QDirIterator>
#include <QDir>
#include <QSet>
#include <QMessageBox>
#include <QTimer>
#include <QSignalBlocker>
#include <QStringList>
#include <QKeySequence>
#include <QtMath>
#include <cmath>
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent){
 setWindowTitle("MIXORA DJ PRO 1.6 | Windows");setMinimumSize(1130,680);resize(1450,790);
 setStyleSheet(R"(
 QWidget{background:#101722;color:#d9e5f3;font-family:'Segoe UI';font-size:12px}
 QPushButton{background:#253448;border:1px solid #354b61;border-radius:5px;padding:7px;font-weight:700}
 QPushButton:hover{background:#36516b;border-color:#4ac4f3}
 QPushButton:pressed{background:#1a6e90}
 QLabel#trackLabel{font-size:16px;font-weight:700;color:#ffffff}
 QSlider::groove:horizontal{height:5px;background:#344458;border-radius:2px}
 QSlider::handle:horizontal{width:15px;margin:-6px 0;background:#50c7f6;border-radius:7px}
 QFrame#panel{border:1px solid #27374a;border-radius:9px;background:#172130}
 QFrame#panel QLabel,QFrame#panel QPushButton,QFrame#panel QSlider{background:transparent}
 )");
 auto* central=new QWidget;setCentralWidget(central);auto* root=new QVBoxLayout(central);root->setContentsMargins(18,14,18,12);root->setSpacing(12);
 auto* top=new QHBoxLayout;
 auto* logo=new QLabel("MIXORA  /  DJ PRO");logo->setStyleSheet("font-size:24px;font-weight:900;letter-spacing:2px;color:#eef6ff");top->addWidget(logo);top->addStretch();
 auto* hint=new QLabel("2 DECK  |  MIXER  |  AUDIO OUTPUT");hint->setStyleSheet("color:#8195ac;font-weight:700");top->addWidget(hint);root->addLayout(top);
 auto* decks=new QHBoxLayout;decks->setSpacing(12);
 auto panel=[&](DeckWidget* d){auto* f=new QFrame;f->setObjectName("panel");auto* l=new QVBoxLayout(f);l->setContentsMargins(0,0,0,0);l->addWidget(d);return f;};
 m_a=new DeckWidget("A",QColor("#3bd2fd"));m_b=new DeckWidget("B",QColor("#ff6aa7"));
 decks->addWidget(panel(m_a));decks->addWidget(panel(m_b));root->addLayout(decks,1);
 // File library: keyboard search, local-file import, deck assignment, persistent paths.
 auto* libraryPanel=new QFrame;libraryPanel->setObjectName("panel");
 auto* libraryRoot=new QVBoxLayout(libraryPanel);libraryRoot->setContentsMargins(14,10,14,10);
 auto* libraryTools=new QHBoxLayout;
 auto* libraryTitle=new QLabel("LIBRERIA MUSICALE");libraryTitle->setStyleSheet("font-weight:800;color:#a3dcf5");libraryTools->addWidget(libraryTitle);
 m_search=new QLineEdit;m_search->setPlaceholderText("Cerca un brano...");m_search->setClearButtonEnabled(true);libraryTools->addWidget(m_search,1);
 auto* importFiles=new QPushButton("AGGIUNGI FILE");libraryTools->addWidget(importFiles);
 auto* importFolder=new QPushButton("IMPORTA CARTELLA");libraryTools->addWidget(importFolder);
 auto* loadA=new QPushButton("CARICA SU A");libraryTools->addWidget(loadA);
 auto* loadB=new QPushButton("CARICA SU B");libraryTools->addWidget(loadB);
 libraryRoot->addLayout(libraryTools);
 m_library=new QListWidget;m_library->setMinimumHeight(115);m_library->setMaximumHeight(165);m_library->setAlternatingRowColors(true);
 m_library->setSelectionMode(QAbstractItemView::SingleSelection);libraryRoot->addWidget(m_library);
 root->addWidget(libraryPanel);
 connect(importFiles,&QPushButton::clicked,this,[this]{const auto files=QFileDialog::getOpenFileNames(this,"Importa brani",QString(),"Audio (*.mp3 *.wav *.flac *.ogg *.m4a *.aac *.wma)");addFiles(files);});
 connect(importFolder,&QPushButton::clicked,this,[this]{
  const auto directory=QFileDialog::getExistingDirectory(this,"Scegli cartella musicale");if(directory.isEmpty())return;
  QStringList files;QDirIterator it(directory,{"*.mp3","*.wav","*.flac","*.ogg","*.m4a","*.aac","*.wma"},QDir::Files,QDirIterator::Subdirectories);
  constexpr int maximum=20000;
  while(it.hasNext() && files.size()<maximum)files.append(it.next());
  addFiles(files);if(files.size()==maximum)m_status->setText("Importate fino a 20.000 tracce per operazione");
 });
 connect(m_search,&QLineEdit::textChanged,this,&MainWindow::filterLibrary);
 connect(loadA,&QPushButton::clicked,this,[this]{loadSelected(true);});
 connect(loadB,&QPushButton::clicked,this,[this]{loadSelected(false);});
 connect(m_library,&QListWidget::itemDoubleClicked,this,[this](QListWidgetItem*){loadSelected(true);});
 auto* mixer=new QFrame;mixer->setObjectName("panel");auto* mixing=new QHBoxLayout(mixer);mixing->setContentsMargins(20,10,20,10);
 auto* masterLabel=new QLabel("MASTER");masterLabel->setStyleSheet("font-weight:800;color:#92aacc");mixing->addWidget(masterLabel);
 m_master=new QSlider(Qt::Horizontal);m_master->setRange(0,100);m_master->setValue(85);m_master->setMaximumWidth(180);mixing->addWidget(m_master);
 mixing->addStretch();auto* aLabel=new QLabel("A");aLabel->setStyleSheet("color:#3bd2fd;font-weight:900");mixing->addWidget(aLabel);
 m_cross=new QSlider(Qt::Horizontal);m_cross->setRange(0,100);m_cross->setValue(50);m_cross->setMinimumWidth(250);m_cross->setMaximumWidth(440);mixing->addWidget(m_cross,2);
 auto* bLabel=new QLabel("B");bLabel->setStyleSheet("color:#ff6aa7;font-weight:900");mixing->addWidget(bLabel);mixing->addStretch();
 auto* reset=new QPushButton("CENTRA CROSSFADER");mixing->addWidget(reset);connect(reset,&QPushButton::clicked,this,[this]{m_cross->setValue(50);});
 root->addWidget(mixer);
 auto* audioRow=new QHBoxLayout;audioRow->addWidget(new QLabel("MASTER"));
 auto* outputs=new QComboBox;outputs->setMinimumWidth(210);audioRow->addWidget(outputs);
 audioRow->addSpacing(12);audioRow->addWidget(new QLabel("CUFFIE CUE"));
 auto* cueOutputs=new QComboBox;cueOutputs->setMinimumWidth(210);audioRow->addWidget(cueOutputs);
 auto* cueGainLabel=new QLabel("VOL CUE");audioRow->addWidget(cueGainLabel);
 auto* cueGain=new QSlider(Qt::Horizontal);cueGain->setRange(0,100);cueGain->setMaximumWidth(125);
 cueGain->setValue(QSettings("Mixora","DJPro").value("cueGain",65).toInt());audioRow->addWidget(cueGain);
 auto* refresh=new QPushButton("AGGIORNA");audioRow->addWidget(refresh);
 root->addLayout(audioRow);
 auto* audioNote=new QLabel("CUE sperimentale: seleziona cuffie su dispositivo diverso dal Master. Due player per deck, clock non condiviso; non usare in live.");
 audioNote->setWordWrap(true);audioNote->setStyleSheet("color:#c7ad77");root->addWidget(audioNote);
 auto populate=[this,outputs,cueOutputs](){
  const auto devices=QMediaDevices::audioOutputs();
  const auto masterId=QSettings("Mixora","DJPro").value("audioDeviceId").toByteArray();
  const auto cueId=QSettings("Mixora","DJPro").value("cueDeviceId").toByteArray();
  QSignalBlocker blockMaster(outputs),blockCue(cueOutputs);
  outputs->clear();cueOutputs->clear();int masterIndex=0,cueIndex=-1;
  for(int i=0;i<devices.size();++i){
   outputs->addItem(devices[i].description(),devices[i].id());
   cueOutputs->addItem(devices[i].description(),devices[i].id());
   if(devices[i].id()==masterId)masterIndex=i;
   if(!cueId.isEmpty()&&devices[i].id()==cueId)cueIndex=i;
  }
  if(devices.isEmpty()){m_a->setCueEnabled(false);m_b->setCueEnabled(false);return;}
  outputs->setCurrentIndex(masterIndex);m_a->setOutputDevice(devices[masterIndex]);m_b->setOutputDevice(devices[masterIndex]);
  if(cueIndex<0){for(int i=0;i<devices.size();++i){if(i!=masterIndex){cueIndex=i;break;}}}
  cueOutputs->setCurrentIndex(cueIndex);
  if(cueIndex>=0){m_a->setCueOutputDevice(devices[cueIndex]);m_b->setCueOutputDevice(devices[cueIndex]);}
  else{m_a->setCueEnabled(false);m_b->setCueEnabled(false);}
  cueOutputs->setEnabled(devices.size()>1);
 };
 connect(refresh,&QPushButton::clicked,this,populate);
 connect(outputs,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this,outputs,cueOutputs](int index){
  if(index<0)return;const auto devices=QMediaDevices::audioOutputs();
  for(const auto& d:devices)if(d.id()==outputs->itemData(index).toByteArray()){
   m_a->setOutputDevice(d);m_b->setOutputDevice(d);QSettings("Mixora","DJPro").setValue("audioDeviceId",d.id());break;
  }
  if(cueOutputs->currentData().toByteArray()==outputs->currentData().toByteArray()){
   m_a->setCueEnabled(false);m_b->setCueEnabled(false);
   m_status->setText("Attenzione: Master e CUE sullo stesso dispositivo. Preascolto disabilitato.");
  }
 });
 connect(cueOutputs,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this,cueOutputs,outputs](int index){
  if(index<0)return;
  if(cueOutputs->currentData().toByteArray()==outputs->currentData().toByteArray()){
   m_a->setCueEnabled(false);m_b->setCueEnabled(false);m_status->setText("CUE disabilitato: scegli un dispositivo diverso da Master");return;
  }
  for(const auto& device:QMediaDevices::audioOutputs())if(device.id()==cueOutputs->itemData(index).toByteArray()){
   m_a->setCueOutputDevice(device);m_b->setCueOutputDevice(device);
   QSettings("Mixora","DJPro").setValue("cueDeviceId",device.id());break;
  }
 });
 connect(cueGain,&QSlider::valueChanged,this,[this](int value){m_a->setCueVolume(value);m_b->setCueVolume(value);QSettings("Mixora","DJPro").setValue("cueGain",value);});
 m_a->setCueVolume(cueGain->value());m_b->setCueVolume(cueGain->value());populate();
 auto* shortcutA=new QShortcut(QKeySequence(Qt::Key_F1),this);connect(shortcutA,&QShortcut::activated,m_a,&DeckWidget::togglePlay);
 auto* shortcutB=new QShortcut(QKeySequence(Qt::Key_F2),this);connect(shortcutB,&QShortcut::activated,m_b,&DeckWidget::togglePlay);
 auto* footer=new QHBoxLayout;m_status=new QLabel("Pronto. Trascina un file audio su uno dei due deck.");m_status->setStyleSheet("color:#93a9bf");footer->addWidget(m_status);footer->addStretch();auto* version=new QLabel("BUILD 1.6.0 | QT6");version->setStyleSheet("color:#687e93");footer->addWidget(version);root->addLayout(footer);
 connect(m_a,&DeckWidget::cueToggleRequested,this,[this,outputs,cueOutputs](DeckWidget* deck,bool requested){
  if(requested && (cueOutputs->currentIndex()<0 || cueOutputs->currentData()==outputs->currentData())){
   deck->setCueEnabled(false);m_status->setText("CUE richiede un dispositivo audio differente dal Master");return;
  }
  deck->setCueEnabled(requested);
 });
 connect(m_b,&DeckWidget::cueToggleRequested,this,[this,outputs,cueOutputs](DeckWidget* deck,bool requested){
  if(requested && (cueOutputs->currentIndex()<0 || cueOutputs->currentData()==outputs->currentData())){
   deck->setCueEnabled(false);m_status->setText("CUE richiede un dispositivo audio differente dal Master");return;
  }
  deck->setCueEnabled(requested);
 });
 connect(m_cross,&QSlider::valueChanged,this,&MainWindow::updateMixer);connect(m_master,&QSlider::valueChanged,this,&MainWindow::updateMixer);
 for(auto* deck:{m_a,m_b}){
  connect(deck,&DeckWidget::statusMessage,this,[this](const QString& msg){m_status->setText(msg);});
  connect(deck,&DeckWidget::requestSync,this,[this](DeckWidget* source){source->syncTo(source==m_a?m_b:m_a);});
 }
 QSettings settings("Mixora","DJPro");m_master->setValue(settings.value("master",85).toInt());m_cross->setValue(settings.value("crossfader",50).toInt());
 m_tracks=settings.value("libraryPaths").toStringList();filterLibrary();
 connect(m_a,&DeckWidget::trackLoaded,this,[this](const QString& path){addFiles({path});QSettings("Mixora","DJPro").setValue("lastDeckA",path);});
 connect(m_b,&DeckWidget::trackLoaded,this,[this](const QString& path){addFiles({path});QSettings("Mixora","DJPro").setValue("lastDeckB",path);});
 connect(qApp,&QApplication::aboutToQuit,this,[this]{QSettings settings("Mixora","DJPro");settings.setValue("master",m_master->value());settings.setValue("crossfader",m_cross->value());settings.setValue("libraryPaths",m_tracks);});
 const QString lastA=settings.value("lastDeckA").toString(),lastB=settings.value("lastDeckB").toString();
 if(QFileInfo::exists(lastA))m_a->loadFile(lastA);
 if(QFileInfo::exists(lastB))m_b->loadFile(lastB);
 auto* quickLoadA=new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_1),this);connect(quickLoadA,&QShortcut::activated,this,[this]{loadSelected(true);});
 auto* quickLoadB=new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_2),this);connect(quickLoadB,&QShortcut::activated,this,[this]{loadSelected(false);});
 updateMixer();
}
void MainWindow::updateMixer(){const double x=m_cross->value()/100.,gain=m_master->value()/100.;
 // Equal-power crossfade: sum of squared coefficients is one
 m_a->setVolumeFactor(gain*std::cos(x*1.5707963267948966));m_b->setVolumeFactor(gain*std::sin(x*1.5707963267948966));}

void MainWindow::addFiles(const QStringList& files){
 QSet<QString> existing;for(const auto& oldPath:m_tracks)existing.insert(oldPath);int added=0;
 for(const auto& p:files){QFileInfo info(p);if(!info.isFile()||!info.isReadable())continue;
  const QString path=info.canonicalFilePath();if(path.isEmpty()||existing.contains(path))continue;
  existing.insert(path);m_tracks.append(path);++added;
 }
 if(added){m_tracks.sort(Qt::CaseInsensitive);filterLibrary();QSettings("Mixora","DJPro").setValue("libraryPaths",m_tracks);m_status->setText(QString("Libreria: %1 nuovi brani, %2 totali").arg(added).arg(m_tracks.size()));}
}
void MainWindow::filterLibrary(){
 if(!m_library||!m_search)return;
 const QString selected=m_library->currentItem()?m_library->currentItem()->data(Qt::UserRole).toString():QString();
 m_library->clear();const QString needle=m_search->text().trimmed();
 for(const QString& path:m_tracks){QFileInfo info(path);if(!info.isFile())continue;
  if(!needle.isEmpty()&&!info.fileName().contains(needle,Qt::CaseInsensitive))continue;
  auto* item=new QListWidgetItem(info.completeBaseName(),m_library);item->setData(Qt::UserRole,path);item->setToolTip(path);
  if(path==selected)m_library->setCurrentItem(item);
 }
}
void MainWindow::loadSelected(bool deckA){
 auto* item=m_library->currentItem();if(!item){m_status->setText("Seleziona prima un brano dalla libreria");return;}
 const auto path=item->data(Qt::UserRole).toString();if(!QFileInfo::exists(path)){m_status->setText("File non trovato: aggiorna la libreria");return;}
 (deckA?m_a:m_b)->loadFile(path);
}
