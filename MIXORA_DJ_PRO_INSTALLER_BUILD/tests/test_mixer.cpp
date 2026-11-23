#include "../src/dsp/Mixer.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace mixora;
static bool approx(double a,double b,double eps=0.0001){return std::abs(a-b)<eps;}
int main(){
 Mixer m(48000);m.setMasterGain(1);m.setCrossfader(0);
 for(int i=0;i<200;++i){auto f=m.process({.25f,-.25f},{.8f,.8f});assert(approx(f.left,.25));assert(approx(f.right,-.25));}
 m.setCrossfader(1);m.reset();for(int i=0;i<200;++i){auto f=m.process({.25f,-.25f},{.8f,-.8f});assert(approx(f.left,.8));assert(approx(f.right,-.8));}
 m.setCrossfader(.5);m.reset();const auto center=m.process({.2f,.2f},{.2f,.2f});assert(approx(center.left,.282842712,0.0002));
 m.setCrossfader(0);m.setEQ(0,-12,0,0);m.reset();float low=0;
 for(int i=0;i<48000;++i){const float sample=static_cast<float>(.2*std::sin(2*3.141592653589793*60*i/48000));low=m.process({sample,sample},{}).left;if(i>20000)assert(std::isfinite(low));}
 bool threw=false;try{m.setDeckGain(7,1);}catch(const std::invalid_argument&){threw=true;}assert(threw);
 m.setEQ(0,24,24,24);m.reset();for(int i=0;i<10000;++i){auto f=m.process({.9f,.9f},{.9f,.9f});assert(std::isfinite(f.left));assert(f.left<=1&&f.left>=-1);}
 std::cout<<"Mixer DSP: 6 checks passed\n";
}
