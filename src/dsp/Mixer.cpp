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

Mixer::Mixer(double sr):sampleRate_(sr){
 if(!std::isfinite(sr)||sr<16000||sr>384000)throw std::invalid_argument("Invalid sample rate");
 for(int i=0;i<4;++i)setEQ(i,0,0,0);
}
void Mixer::setCrossfader(double x){if(!std::isfinite(x))throw std::invalid_argument("Invalid crossfader");cross_=clamp(x,0,1);}
void Mixer::setMasterGain(double g){if(!std::isfinite(g))throw std::invalid_argument("Invalid master gain");master_=clamp(g,0,1);}
void Mixer::setDeckGain(int d,double g){if(d<0||d>=4||!std::isfinite(g))throw std::invalid_argument("Invalid deck gain");gains_[d]=clamp(g,0,2);}
void Mixer::setChannelFader(int d,double f){if(d<0||d>=4||!std::isfinite(f))throw std::invalid_argument("Invalid fader");faders_[d]=clamp(f,0,1);}
void Mixer::setFilter(int d,double value){if(d<0||d>=4||!std::isfinite(value))throw std::invalid_argument("Invalid filter");filters_[d]=clamp(value,0,1);}
void Mixer::setEQ(int d,double lo,double mid,double hi){if(d<0||d>=4)throw std::invalid_argument("Invalid deck");eq_[d].configure(sampleRate_,lo,mid,hi);}
StereoFrame Mixer::process(StereoFrame a,StereoFrame b){return process(std::array<StereoFrame,4>{{a,b,{},{}}});}
StereoFrame Mixer::process(const std::array<StereoFrame,4>& input){
 double left=0,right=0;
 const double sideA=std::cos(cross_*pi/2),sideB=std::sin(cross_*pi/2);
 for(int d=0;d<4;++d){
  StereoFrame x=eq_[d].process(input[d]);
  // Simple, stateful one-pole resonant-free musical filter, neutral at centre.
  const double value=filters_[d];
  if(value<0.495){
   const double strength=(0.5-value)*2.;
   const double hz=20000.*std::pow(40./20000.,strength);
   const double alpha=1.-std::exp(-2.*pi*hz/sampleRate_);
   double* state=filterLow_[d].data();
   state[0]+=alpha*(double(x.left)-state[0]);
   state[1]+=alpha*(double(x.right)-state[1]);
   x={float(state[0]),float(state[1])};
  }else if(value>0.505){
   const double strength=(value-0.5)*2.;
   const double hz=20.*std::pow(8000./20.,strength);
   const double alpha=1.-std::exp(-2.*pi*hz/sampleRate_);
   double* state=filterHigh_[d].data();
   state[0]+=alpha*(double(x.left)-state[0]);
   state[1]+=alpha*(double(x.right)-state[1]);
   x={float(double(x.left)-state[0]),float(double(x.right)-state[1])};
  }
  const double level=(d%2==0?sideA:sideB)*gains_[d]*faders_[d];
  left+=x.left*level;
  right+=x.right*level;
 }
 // Bounded floating point output, not a mastering-grade true peak limiter.
 return {float(clamp(left*master_,-1.,1.)),float(clamp(right*master_,-1.,1.))};
}
void Mixer::reset(){
 for(auto& e:eq_)e.reset();
 for(auto& st:filterLow_)st.fill(0.);
 for(auto& st:filterHigh_)st.fill(0.);
}
}
