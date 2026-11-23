#include "../src/audio/Resample.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
int main(){
 mixora::WavFile x;x.sampleRate=44100;
 x.frames.resize(44100);
 for(size_t i=0;i<x.frames.size();++i){float v=std::sin(2*3.141592653589793*440*i/44100);x.frames[i]={v,v};}
 auto y=mixora::resampleLinear(x,48000);assert(y.sampleRate==48000&&y.frames.size()==48000);
 auto z=mixora::resampleLinear(y,44100);assert(z.frames.size()==x.frames.size());
 double error=0;for(size_t i=0;i<x.frames.size();++i)error+=std::abs(z.frames[i].left-x.frames[i].left);
 assert(error/x.frames.size()<0.01);
 auto same=mixora::resampleLinear(x,44100);assert(same.frames.size()==x.frames.size());
 mixora::WavFile empty;empty.sampleRate=44100;assert(mixora::resampleLinear(empty,48000).frames.empty());
 bool caught=false;try{mixora::resampleLinear(x,0);}catch(const std::invalid_argument&){caught=true;}assert(caught);
}
