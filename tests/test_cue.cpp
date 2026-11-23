#include "../src/audio/LiveEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace mixora;
int main(){
 LiveEngine e(48000);
 WavFile a;a.sampleRate=48000;a.frames.assign(4,{0.4f,0.2f});
 WavFile b;b.sampleRate=48000;b.frames.assign(4,{-0.2f,-0.1f});
 e.load(0,a);e.load(1,b);
 e.setMaster(0.0);e.setCrossfader(1.0);
 e.setCue(0,true);e.setCueGain(1.0);
 e.setPlaying(0,true);e.setPlaying(1,true);
 StereoFrame m[2],c[2];e.processBuses(m,c,2);
 for(int i=0;i<2;++i){
  assert(std::abs(m[i].left)<1e-5f);
  assert(std::abs(c[i].left-0.4f)<1e-5f);
  assert(std::abs(c[i].right-0.2f)<1e-5f);
 }
 e.setCue(1,true);e.processBuses(m,c,2);
 assert(std::abs(c[0].left-0.1f)<1e-5f);
 assert(e.position(0)==4 && e.position(1)==4);
 assert(!e.playing(0) && !e.playing(1));
 e.processBuses(m,c,2);
 assert(std::abs(c[0].left)<1e-5f);
 bool threw=false;try{e.setCue(2,true);}catch(const std::invalid_argument&){threw=true;}
 assert(threw);
 std::cout<<"FOUR_CHANNEL_CUE_TESTS_OK\n";
}
