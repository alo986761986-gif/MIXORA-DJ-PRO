#include "Mixer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mixora {
namespace {
constexpr double pi=3.1415926535897932384626433832795;
double clamp(double v,double lo,double hi){return std::max(lo,std::min(v,hi));}
}
void Biquad::configure(Type type,double sr,double hz,double gainDb,double q){
 if(!std::isfinite(sr)||sr<8000||!std::isfinite(hz)||hz<=0||hz>=sr*.49||!std::isfinite(gainDb)||!std::isfinite(q)||q<=0)throw std::invalid_argument("Invalid EQ parameters");
 const double A=std::pow(10.,clamp(gainDb,-24.,24.)/40.);
 const double w=2*pi*hz/sr,co=std::cos(w),si=std::sin(w);
 const double alpha=si/(2*q);
 double b0,b1,b2,a0,a1,a2;
 if(type==Type::Peak){b0=1+alpha*A;b1=-2*co;b2=1-alpha*A;a0=1+alpha/A;a1=-2*co;a2=1-alpha/A;}
 else {
  const double sq=2*std::sqrt(A)*alpha;
  if(type==Type::LowShelf){
   b0=A*((A+1)-(A-1)*co+sq);b1=2*A*((A-1)-(A+1)*co);b2=A*((A+1)-(A-1)*co-sq);
   a0=(A+1)+(A-1)*co+sq;a1=-2*((A-1)+(A+1)*co);a2=(A+1)+(A-1)*co-sq;
  } else {
   b0=A*((A+1)+(A-1)*co+sq);b1=-2*A*((A-1)+(A+1)*co);b2=A*((A+1)+(A-1)*co-sq);
   a0=(A+1)-(A-1)*co+sq;a1=2*((A-1)-(A+1)*co);a2=(A+1)-(A-1)*co-sq;
  }
 }
 b0_=b0/a0;b1_=b1/a0;b2_=b2/a0;a1_=a1/a0;a2_=a2/a0;
}
StereoFrame Biquad::process(StereoFrame frame){
 double values[2]={frame.left,frame.right};
 for(int c=0;c<2;++c){
  const double in=values[c];
  const double out=b0_*in+b1_*x1_[c]+b2_*x2_[c]-a1_*y1_[c]-a2_*y2_[c];
  x2_[c]=x1_[c];x1_[c]=in;y2_[c]=y1_[c];y1_[c]=std::abs(out)<1e-30?0:out;
  values[c]=out;
 }
 return {static_cast<float>(values[0]),static_cast<float>(values[1])};
}
void Biquad::reset(){x1_.fill(0);x2_.fill(0);y1_.fill(0);y2_.fill(0);}
void ThreeBandEQ::configure(double sr,double low,double mid,double high){
 low_.configure(Biquad::Type::LowShelf,sr,180,low);
 mid_.configure(Biquad::Type::Peak,sr,1100,mid,0.85);
 high_.configure(Biquad::Type::HighShelf,sr,5500,high);
}
StereoFrame ThreeBandEQ::process(StereoFrame x){return high_.process(mid_.process(low_.process(x)));}
void ThreeBandEQ::reset(){low_.reset();mid_.reset();high_.reset();}
Mixer::Mixer(double sr):sampleRate_(sr){if(!std::isfinite(sr)||sr<16000||sr>384000)throw std::invalid_argument("Invalid sample rate");setEQ(0,0,0,0);setEQ(1,0,0,0);}
void Mixer::setCrossfader(double x){if(!std::isfinite(x))throw std::invalid_argument("Invalid crossfader");cross_=clamp(x,0,1);}
void Mixer::setMasterGain(double g){if(!std::isfinite(g))throw std::invalid_argument("Invalid master gain");master_=clamp(g,0,1);}
void Mixer::setDeckGain(int deck,double g){if(deck<0||deck>1||!std::isfinite(g))throw std::invalid_argument("Invalid deck gain");gains_[deck]=clamp(g,0,2);}
void Mixer::setEQ(int deck,double lo,double mid,double hi){if(deck<0||deck>1)throw std::invalid_argument("Invalid deck");eq_[deck].configure(sampleRate_,lo,mid,hi);}
StereoFrame Mixer::process(StereoFrame a,StereoFrame b){
 a=eq_[0].process(a);b=eq_[1].process(b);
 const double ga=std::cos(cross_*pi/2)*gains_[0]*master_,gb=std::sin(cross_*pi/2)*gains_[1]*master_;
 // Hard clipping guard; replace with a true-peak limiter before live use.
 return {static_cast<float>(clamp(a.left*ga+b.left*gb,-1,1)),static_cast<float>(clamp(a.right*ga+b.right*gb,-1,1))};
}
void Mixer::reset(){eq_[0].reset();eq_[1].reset();}
}
