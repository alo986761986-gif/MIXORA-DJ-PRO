#include "WaveformWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
WaveformWidget::WaveformWidget(QWidget* p):QWidget(p){setMinimumHeight(100);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
void WaveformWidget::setPeaks(const QVector<float>& a){m_peaks=a;update();}
void WaveformWidget::setProgress(double v){m_progress=std::clamp(v,0.,1.);update();}
void WaveformWidget::setColor(const QColor& c){m_color=c;update();}
void WaveformWidget::paintEvent(QPaintEvent*){
 QPainter p(this);p.setRenderHint(QPainter::Antialiasing,false);p.fillRect(rect(),QColor("#111a27"));
 const int w=width(),h=height(),cy=h/2;
 p.setPen(QPen(QColor("#293a4f"),1));p.drawLine(0,cy,w,cy);
 if(m_peaks.isEmpty()){p.setPen(QColor("#65768b"));p.drawText(rect(),Qt::AlignCenter,"TRASCINA UN BRANO QUI");return;}
 for(int x=0;x<w;x+=2){int i=int(double(x)/std::max(1,w)*m_peaks.size());i=std::clamp(i,0,int(m_peaks.size())-1);
  int bh=int(m_peaks[i]*(h*.46));QColor c=m_color;if(double(x)/std::max(1,w)>m_progress)c=c.darker(185);
  p.setPen(QPen(c,2));p.drawLine(x,cy-bh,x,cy+bh);
 }
 const int px=int(m_progress*w);p.setPen(QPen(QColor("#ffffff"),2));p.drawLine(px,0,px,h);
}

void WaveformWidget::mousePressEvent(QMouseEvent* e){if(e->button()==Qt::LeftButton && width()>0){emit seekRequested(std::clamp(double(e->position().x())/width(),0.,1.));e->accept();}else QWidget::mousePressEvent(e);}
