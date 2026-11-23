#include "DeckWidget.h"
#include "WaveformWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QAudioFormat>
#include <QSignalBlocker>
#include <QFile>
#include <QtMath>
#include <algorithm>
#include <cmath>
static QString formatTime(qint64 ms){qint64 s=ms/1000;return QString("%1:%2").arg(s/60,2,10,QChar('0')).arg(s%60,2,10,QChar('0'));}
DeckWidget::DeckWidget(const QString& name,const QColor& color,QWidget* parent):QWidget(parent),m_name(name){
 setAcceptDrops(true);m_output.setVolume(.75);m_player.setAudioOutput(&m_output);m_cuePlayer.setAudioOutput(&m_cueOutput);m_cueOutput.setVolume(.65);
 m_cueTimer.setInterval(250);connect(&m_cueTimer,&QTimer::timeout,this,&DeckWidget::reconcileCue);m_cueTimer.start();
 auto* root=new QVBoxLayout(this);root->setSpacing(9);root->setContentsMargins(15,15,15,15);
 auto* headline=new QHBoxLayout;auto* id=new QLabel("DECK "+name);id->setStyleSheet(QString("color:%1;font-weight:800;font-size:17px").arg(color.name()));headline->addWidget(id);
 m_bpmLabel=new QLabel("BPM --");headline->addStretch();headline->addWidget(m_bpmLabel);root->addLayout(headline);
 m_title=new QLabel("Nessun brano caricato");m_title->setObjectName("trackLabel");m_title->setTextInteractionFlags(Qt::TextSelectableByMouse);m_title->setMinimumHeight(30);root->addWidget(m_title);
 m_wave=new WaveformWidget; m_wave->setColor(color);root->addWidget(m_wave);
 connect(m_wave,&WaveformWidget::seekRequested,this,&DeckWidget::seekToFraction);
 auto* inf=new QHBoxLayout;m_time=new QLabel("00:00 / 00:00");inf->addWidget(m_time);inf->addStretch();m_rateLabel=new QLabel("100.0%");inf->addWidget(m_rateLabel);root->addLayout(inf);
 auto* transport=new QHBoxLayout;
 auto makeBtn=[&](const QString& text){auto* b=new QPushButton(text);b->setMinimumHeight(37);transport->addWidget(b);return b;};
 auto* load=makeBtn("CARICA");m_play=makeBtn("PLAY");auto* cue=makeBtn("CUE");auto* sync=makeBtn("SYNC");m_loop=makeBtn("LOOP 4");m_cueMonitorButton=makeBtn("CUE HP OFF");m_cueMonitorButton->setCheckable(true);root->addLayout(transport);
 connect(m_cueMonitorButton,&QPushButton::toggled,this,[this](bool enabled){emit cueToggleRequested(this,enabled);});
 connect(load,&QPushButton::clicked,this,[this]{auto f=QFileDialog::getOpenFileName(this,"Carica brano",QString(),"Audio (*.mp3 *.wav *.flac *.ogg *.m4a *.aac *.wma);;Tutti i file (*)");if(!f.isEmpty())loadFile(f);});
 connect(m_play,&QPushButton::clicked,this,&DeckWidget::togglePlay);
 connect(cue,&QPushButton::clicked,this,[this]{if(!hasTrack())return;if(playing()){m_player.pause();m_player.setPosition(m_cueMs);}else{m_cueMs=m_player.position();emit statusMessage(m_name+": punto CUE impostato");}});
 connect(sync,&QPushButton::clicked,this,[this]{emit requestSync(this);});
 connect(m_loop,&QPushButton::clicked,this,[this]{if(!hasTrack()||m_bpm<=0){emit statusMessage("LOOP: serve un BPM rilevato");return;}m_loopEnabled=!m_loopEnabled;
 if(m_loopEnabled){m_loopStart=m_player.position();m_loopLength=qint64(4.0*60'000.0/m_bpm);m_loop->setText("LOOP ON");}else m_loop->setText("LOOP 4");});
 auto* ctl=new QHBoxLayout;auto* pitchText=new QLabel("TEMPO");m_pitch=new QSlider(Qt::Horizontal);m_pitch->setRange(500,1500);m_pitch->setValue(1000);ctl->addWidget(pitchText);ctl->addWidget(m_pitch,3);
 auto* volText=new QLabel("GAIN");m_gain=new QSlider(Qt::Horizontal);m_gain->setRange(0,100);m_gain->setValue(80);ctl->addWidget(volText);ctl->addWidget(m_gain,2);root->addLayout(ctl);
 connect(m_pitch,&QSlider::valueChanged,this,[this](int v){m_player.setPlaybackRate(v/1000.);m_cuePlayer.setPlaybackRate(v/1000.);m_rateLabel->setText(QString::number(v/10.,'f',1)+"%");});
 connect(m_gain,&QSlider::valueChanged,this,&DeckWidget::updatePlaybackVolume);
 auto* cues=new QHBoxLayout;for(int i=0;i<4;i++){auto* b=new QPushButton("HOT "+QString::number(i+1));cues->addWidget(b);connect(b,&QPushButton::clicked,this,[this,i]{if(!hasTrack())return;if(m_hotcues[i]<0){m_hotcues[i]=m_player.position();emit statusMessage("HOT CUE "+QString::number(i+1)+" memorizzato");}else m_player.setPosition(m_hotcues[i]);});}root->addLayout(cues);
 connect(&m_player,&QMediaPlayer::positionChanged,this,&DeckWidget::updatePosition);
 connect(&m_player,&QMediaPlayer::durationChanged,this,&DeckWidget::updateDuration);
 connect(&m_player,&QMediaPlayer::playbackStateChanged,this,[this](QMediaPlayer::PlaybackState s){m_play->setText(s==QMediaPlayer::PlayingState?"PAUSA":"PLAY");updateCueTransport();});
 connect(&m_player,&QMediaPlayer::errorOccurred,this,[this](QMediaPlayer::Error e,const QString& err){if(e!=QMediaPlayer::NoError)emit statusMessage(m_name+": errore audio - "+err);});
 connect(&m_cuePlayer,&QMediaPlayer::errorOccurred,this,[this](QMediaPlayer::Error e,const QString& err){if(e!=QMediaPlayer::NoError)emit statusMessage(m_name+": preascolto non disponibile - "+err);});
 connect(&m_decoder,&QAudioDecoder::bufferReady,this,&DeckWidget::collectBuffer);
 connect(&m_decoder,&QAudioDecoder::finished,this,&DeckWidget::finishAnalysis);
 connect(&m_decoder,&QAudioDecoder::errorOccurred,this,[this](QAudioDecoder::Error){emit statusMessage(m_name+": anteprima waveform non disponibile per questo formato");});
 updatePlaybackVolume();
}
void DeckWidget::dragEnterEvent(QDragEnterEvent* e){for(const auto& u:e->mimeData()->urls()){if(u.isLocalFile() && QFileInfo(u.toLocalFile()).isFile()){e->acceptProposedAction();break;}}}
void DeckWidget::dropEvent(QDropEvent* e){for(const QUrl& u:e->mimeData()->urls()){if(u.isLocalFile() && QFileInfo(u.toLocalFile()).isFile()){loadFile(u.toLocalFile());e->acceptProposedAction();break;}}}
void DeckWidget::loadFile(const QString& path){if(!QFileInfo(path).isFile() || !QFileInfo(path).isReadable()){emit statusMessage(m_name+": file audio non leggibile");return;}m_path=path;m_cueMs=0;m_bpm=0;m_loopEnabled=false;m_loop->setText("LOOP 4");m_hotcues.fill(-1);m_peaks.clear();m_wave->setPeaks({});m_bpmLabel->setText("BPM --");m_title->setText(QFileInfo(path).completeBaseName());m_title->setToolTip(path);m_player.stop();m_cuePlayer.stop();m_player.setSource(QUrl::fromLocalFile(path));m_cuePlayer.setSource(QUrl::fromLocalFile(path));analyzeAudio(path);emit trackLoaded(path);emit statusMessage(m_name+": "+QFileInfo(path).fileName());}
bool DeckWidget::hasTrack()const{return !m_path.isEmpty();}
bool DeckWidget::playing()const{return m_player.playbackState()==QMediaPlayer::PlayingState;}
QString DeckWidget::trackTitle()const{return QFileInfo(m_path).completeBaseName();}
void DeckWidget::togglePlay(){if(!hasTrack())return;if(playing())m_player.pause();else m_player.play();}
void DeckWidget::setPlaybackRate(double v){m_pitch->setValue(std::clamp(int(v*1000),500,1500));}
void DeckWidget::setOutputDevice(const QAudioDevice& device){m_output.setDevice(device);}
void DeckWidget::setCueOutputDevice(const QAudioDevice& device){m_cuePlayer.pause();m_cueOutput.setDevice(device);updateCueTransport();}
void DeckWidget::setCueVolume(int percentage){m_cueVolume=std::clamp(percentage,0,100);m_cueOutput.setVolume(m_cueVolume/100.0);}
void DeckWidget::setCueEnabled(bool enabled){
 m_cueEnabled=enabled;
 if(m_cueMonitorButton){QSignalBlocker guard(m_cueMonitorButton);m_cueMonitorButton->setChecked(enabled);m_cueMonitorButton->setText(enabled?"CUE HP ON":"CUE HP OFF");}
 updateCueTransport();
}
void DeckWidget::updateCueTransport(){
 if(!m_cueEnabled||!hasTrack()||!playing()){m_cuePlayer.pause();return;}
 m_cuePlayer.setPlaybackRate(m_player.playbackRate());
 if(m_cuePlayer.playbackState()!=QMediaPlayer::PlayingState){m_cuePlayer.setPosition(m_player.position());m_cuePlayer.play();}
}
void DeckWidget::reconcileCue(){
 if(!m_cueEnabled||!playing()||!hasTrack())return;
 if(m_cuePlayer.playbackState()!=QMediaPlayer::PlayingState){updateCueTransport();return;}
 if(std::abs(m_cuePlayer.position()-m_player.position())>110)m_cuePlayer.setPosition(m_player.position());
}

void DeckWidget::seekToFraction(double fraction){if(!hasTrack() || m_player.duration()<=0)return;m_player.setPosition(qint64(std::clamp(fraction,0.,1.)*m_player.duration()));}
void DeckWidget::setVolumeFactor(double f){m_externalVolume=std::clamp(f,0.,1.);updatePlaybackVolume();}
void DeckWidget::updatePlaybackVolume(){m_output.setVolume(m_externalVolume*m_gain->value()/100.);}
void DeckWidget::updatePosition(qint64 ms){if(m_loopEnabled && m_loopLength>0 && ms>=m_loopStart+m_loopLength && m_loopStart<m_player.duration()){m_player.setPosition(m_loopStart);return;}m_time->setText(formatTime(ms)+" / "+formatTime(m_player.duration()));m_wave->setProgress(m_player.duration()>0?double(ms)/m_player.duration():0);}
void DeckWidget::updateDuration(qint64){updatePosition(m_player.position());}
void DeckWidget::syncTo(const DeckWidget* master){if(!hasTrack()||!master||master==this||!master->hasTrack()||master->bpm()<=0||m_bpm<=0){emit statusMessage("SYNC: analizza prima entrambi i BPM");return;}
 // Consider half/double tempo candidates, common with EDM beat detection.
 const double reference=master->bpm()*master->playbackRate();
 double target=reference/m_bpm;
 for(const double ratio:{0.5,1.0,2.0}){
  const double candidate=reference/(m_bpm*ratio);
  if(candidate>=0.5&&candidate<=1.5&&std::abs(candidate-1.0)<std::abs(target-1.0))target=candidate;
 }
 if(target<.5||target>1.5){emit statusMessage("SYNC: differenza BPM fuori intervallo +/-50%");return;}
 setPlaybackRate(target);
 // Posizione beat approssimativa: le griglie rilevate sono stime, non marker di downbeat affidabili.
 const double beatMs=60000.0/m_bpm;const double masterBeatMs=60000.0/master->bpm();
 const double phase=std::fmod(double(master->position()),masterBeatMs)/masterBeatMs;
 const double oldBeat=std::floor(double(position())/beatMs);m_player.setPosition(qint64((oldBeat+phase)*beatMs));
 emit statusMessage(m_name+": SYNC BPM attivo (fase stimata; non adatto a live senza verifica)");}
void DeckWidget::analyzeAudio(const QString& filename){m_analysisPending=false;m_decoder.stop();m_peaks.clear();m_energy.clear();m_analyzedSeconds=0;m_analysisPending=true;m_decoder.setSource(QUrl::fromLocalFile(filename));m_decoder.start();}
void DeckWidget::collectBuffer(){const QAudioBuffer buf=m_decoder.read();if(!buf.isValid()||buf.frameCount()<=0)return;
 auto fmt=buf.format();const int channels=fmt.channelCount();if(channels<1)return;
 const int frames=buf.frameCount();const int chunk=1024;
 if(fmt.sampleRate()>0)m_analyzedSeconds+=double(frames)/fmt.sampleRate();
 for(int i=0;i<frames;i+=chunk){double sum=0,peak=0;int n=std::min(chunk,frames-i);
  for(int j=i;j<i+n;j++){
   double mono=0;
   for(int c=0;c<channels;c++){
    double v=0;
    if(fmt.sampleFormat()==QAudioFormat::Int16){auto* ptr=buf.constData<qint16>();v=ptr[j*channels+c]/32768.;}
    else if(fmt.sampleFormat()==QAudioFormat::Float){auto* ptr=buf.constData<float>();v=ptr[j*channels+c];}
    else if(fmt.sampleFormat()==QAudioFormat::Int32){auto* ptr=buf.constData<qint32>();v=ptr[j*channels+c]/2147483648.;}
    else if(fmt.sampleFormat()==QAudioFormat::UInt8){auto* ptr=buf.constData<quint8>();v=(ptr[j*channels+c]-128)/128.;}
    mono+=v;
   }
   mono/=channels;peak=std::max(peak,std::abs(mono));sum+=mono*mono;
  }
  m_peaks.append(float(std::min(1.,peak)));m_energy.append(float(std::sqrt(sum/n)));
 }
}
void DeckWidget::finishAnalysis(){if(!m_analysisPending)return;m_analysisPending=false;if(m_peaks.isEmpty()){emit statusMessage(m_name+": analisi audio non disponibile");return;}
 // Riduzione a un numero gestibile di barre
 QVector<float> visual;const int bucket=std::max(1,int(m_peaks.size()/1600));for(int i=0;i<m_peaks.size();i+=bucket){float a=0;for(int j=i;j<std::min(i+bucket,int(m_peaks.size()));j++)a=std::max(a,m_peaks[j]);visual.append(a);}m_wave->setPeaks(visual);
 // BPM indicativo mediante autocorrelazione dei transienti RMS. Non e' una beat-grid completa.
 const double durationSeconds=m_analyzedSeconds;
 if(durationSeconds<8||m_energy.size()<200)return;
 const double samplesPerSecond=m_energy.size()/durationSeconds;
 // Decimazione a ~100 Hz per ridurre il costo del calcolo
 const int stride=std::max(1,int(samplesPerSecond/100));QVector<double> onset;
 for(int i=stride;i<m_energy.size();i+=stride){double prev=m_energy[i-stride],now=m_energy[i];onset.append(std::max(0.,now-prev));}
 const double hz=samplesPerSecond/stride;int maxLag=int(hz*60/70),minLag=int(hz*60/180);
 maxLag=std::min(maxLag,int(onset.size())/2);double best=-1;int bestLag=0;
 const int available=std::min(int(onset.size()),int(hz*90));
 for(int lag=std::max(1,minLag);lag<=maxLag;lag++){double score=0;for(int k=lag;k<available;k++)score+=onset[k]*onset[k-lag];score/=std::max(1,available-lag);
 if(score>best){best=score;bestLag=lag;}}
 if(bestLag>0){m_bpm=std::round(60.*hz/bestLag*10.)/10.;m_bpmLabel->setText(QString("BPM %1 ~").arg(m_bpm,0,'f',1));}
 emit statusMessage(m_name+": analisi completata (BPM stimato)");}
