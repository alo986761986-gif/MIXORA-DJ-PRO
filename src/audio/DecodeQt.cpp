#include "DecodeQt.h"
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QAudioFormat>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <stdexcept>
#include <string>
namespace mixora {
WavFile decodeQtFile(const QString& filename){
 QAudioDecoder decoder;
 QAudioFormat requested;requested.setSampleRate(48000);requested.setChannelCount(2);requested.setSampleFormat(QAudioFormat::Float);
 decoder.setAudioFormat(requested);
 decoder.setSource(QUrl::fromLocalFile(filename));
 WavFile audio;
 QEventLoop loop;QTimer timeout;timeout.setSingleShot(true);
 bool failed=false;QString error;
 QObject::connect(&timeout,&QTimer::timeout,&loop,[&]{failed=true;error="Tempo massimo di decodifica superato (120 secondi)";loop.quit();});
 QObject::connect(&decoder,QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),&loop,[&](QAudioDecoder::Error){failed=true;error=decoder.errorString();loop.quit();});
 QObject::connect(&decoder,&QAudioDecoder::finished,&loop,&QEventLoop::quit);
 QObject::connect(&decoder,&QAudioDecoder::bufferReady,&loop,[&]{
  while(decoder.bufferAvailable()){
   const QAudioBuffer buffer=decoder.read();
   const QAudioFormat fmt=buffer.format();
   if(fmt.sampleRate()<=0 || (fmt.channelCount()!=1 && fmt.channelCount()!=2)){
    failed=true;error="Canali o frequenza audio non supportati";loop.quit();return;
   }
   if(audio.sampleRate==0)audio.sampleRate=static_cast<uint32_t>(fmt.sampleRate());
   if(audio.sampleRate!=static_cast<uint32_t>(fmt.sampleRate())){failed=true;error="Frequenza variabile non supportata";loop.quit();return;}
   const int channels=fmt.channelCount();
   const int frames=buffer.frameCount();
   if(audio.frames.size()+static_cast<size_t>(frames)>48000ULL*60*60*3){failed=true;error="Brano troppo lungo";loop.quit();return;}
   if(fmt.sampleFormat()==QAudioFormat::Float){
    const float* data=buffer.constData<float>();
    for(int i=0;i<frames;++i){float l=data[i*channels],r=channels==2?data[i*channels+1]:l;audio.frames.push_back({l,r});}
   }else if(fmt.sampleFormat()==QAudioFormat::Int16){
    const qint16* data=buffer.constData<qint16>();
    for(int i=0;i<frames;++i){float l=data[i*channels]/32768.f,r=channels==2?data[i*channels+1]/32768.f:l;audio.frames.push_back({l,r});}
   }else{failed=true;error="Formato PCM restituito dal decoder non supportato";loop.quit();return;}
  }
 });
 timeout.start(120000);decoder.start();
 if(decoder.isDecoding())loop.exec();
 decoder.stop();timeout.stop();
 if(failed || audio.frames.empty())throw std::runtime_error((failed?error:QString("File non decodificato")).toStdString());
 return audio;
}
}
