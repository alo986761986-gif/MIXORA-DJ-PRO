#pragma once
#include "../audio/Wav.h"
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

// Original MIXORA waveform widget. Peaks are derived from actual decoded samples.
// Waveform bars are deliberately NOT copied from third-party software assets.
class MixoraWaveform final : public QWidget {
public:
 explicit MixoraWaveform(int deck, bool overview=false, QWidget* parent=nullptr)
  : QWidget(parent), deck_(deck), overview_(overview) {
  setMinimumHeight(overview_?28:106);
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  setCursor(Qt::PointingHandCursor);
  setToolTip("Clicca per spostare la posizione del brano");
 }
 void setSeekHandler(std::function<void(double)> callback) { seek_=std::move(callback); }
 void setTrack(const mixora::WavFile& file, double bpm) {
  peaks_.clear();
  bpm_=bpm;
  sampleRate_=file.sampleRate;
  total_=file.frames.size();
  position_=0;
  if(!total_){update();return;}
  constexpr int bins=1120;
  peaks_.resize(bins);
  const size_t count=file.frames.size();
  for(int i=0;i<bins;++i){
   const size_t first=size_t(i)*count/bins;
   const size_t last=(std::max)(first+size_t(1),size_t(i+1)*count/bins);
   const size_t stride=(std::max)(size_t(1),(last-first)/50);
   float peak=0.f;
   for(size_t f=first;f<last;f+=stride) {
    const auto& s=file.frames[f];
    const float v=(std::max)(std::abs(s.left),std::abs(s.right));
    if(std::isfinite(v))peak=(std::max)(peak,v);
   }
   peaks_[i]=qBound(0.015f,peak,1.f);
  }
  update();
 }
 void setPosition(size_t frame) {
  const auto bounded=(std::min)(total_,frame);
  if(bounded!=position_){position_=bounded;update();}
 }
 void clear() {peaks_.clear();position_=0;total_=0;bpm_=0;update();}
protected:
 void paintEvent(QPaintEvent*) override {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing,false);
  const QRect r=rect();
  p.fillRect(r,QColor(overview_?"#101720":"#0b121b"));
  if(peaks_.isEmpty()){
   p.setPen(QColor("#65768a"));
   p.setFont(QFont("Segoe UI",overview_?8:11,QFont::Medium));
   p.drawText(r,Qt::AlignCenter,overview_?"OVERVIEW":"CARICA O TRASCINA UNA TRACCIA");
   return;
  }
  const int w=r.width(),h=r.height(),mid=h/2;
  const double fraction=total_?double(position_)/double(total_):0.0;
  const int playX=qBound(0,int(fraction*w),w-1);
  p.setPen(QPen(QColor("#1d2b3a"),1));
  p.drawLine(0,mid,w,mid);
  if(bpm_>0 && sampleRate_>0 && total_>0 && !overview_){
   const double beats=double(total_)/double(sampleRate_)*bpm_/60.0;
   if(beats>0 && beats<15000){
    const int firstStep=(std::max)(1,int(std::ceil(beats/(std::max)(w/32,1))));
    const int count=int(beats/firstStep);
    for(int k=0;k<=count;++k){
     const double x=double(k*firstStep)/beats*w;
     const bool phrase=((k*firstStep)%16)==0;
     p.setPen(QPen(QColor(phrase?"#2a3a4c":"#1a2734"),1));
     p.drawLine(int(x),0,int(x),h);
    }
   }
  }
  for(int x=0;x<w;++x){
   const int b=(std::min)(peaks_.size()-1,int(double(x)*peaks_.size()/w));
   const float peak=peaks_[b];
   const int reach=(std::max)(1,int((h/2-3)*peak));
   const bool played=x<=playX;
   const QColor base= deck_==0 ? QColor(played?"#239bd0":"#44cfff") : QColor(played?"#be734e":"#ffb15d");
   p.setPen(QPen(base,overview_?1:1));
   p.drawLine(x,mid-reach,x,mid+reach);
   if(!overview_ && reach>10){
    p.setPen(QPen(deck_==0?QColor("#b0f4ff"):QColor("#ffe0a0"),1));
    p.drawLine(x,mid-reach,x,mid-reach+(std::min)(4,reach/4));
   }
  }
  p.setPen(QPen(QColor("#f9f9fc"),overview_?1:2));
  p.drawLine(playX,0,playX,h);
  if(!overview_){
   p.fillRect(QRect(0,0,5,h),QColor(deck_==0?"#27baf7":"#ffad57"));
   p.setPen(QColor("#aabac7"));
   p.setFont(QFont("Segoe UI",8));
   p.drawText(QRect(12,4,w-16,18),Qt::AlignLeft,"WAVEFORM  •  PCM");
  }
 }
 void mousePressEvent(QMouseEvent* event) override {
  if(event->button()==Qt::LeftButton && total_>0 && seek_){
   const double x=qBound(0.0,event->position().x()/(std::max)(1,width()),1.0);
   seek_(x);
   event->accept();
  }else QWidget::mousePressEvent(event);
 }
private:
 int deck_=0;
 bool overview_=false;
 QVector<float> peaks_;
 size_t total_=0,position_=0;
 uint32_t sampleRate_=0;
 double bpm_=0;
 std::function<void(double)> seek_;
};
