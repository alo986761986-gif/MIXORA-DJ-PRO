#include "../src/audio/LiveEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace mixora;
int main(){
 LiveEngine e(48000);
 WavFile a; a.sampleRate=48000;a.frames.assign(5,{0.5f,0.5f});
 WavFile b=a;b.frames.assign(5,{-0.25f,-0.25f});
 e.load(0,a);e.load(1,b);e.setMaster(1);e.setCrossfader(0);
 e.setPlaying(0,true);e.setPlaying(1,true);
 StereoFrame out[3];e.process(out,3);
 for(auto x:out){assert(std::abs(x.left-0.5f)<0.02f);assert(std::abs(x.right-0.5f)<0.02f);}
 assert(e.position(0)==3&&e.position(1)==3);
 e.setCrossfader(1);e.process(out,3);
 assert(std::abs(out[0].left+0.25f)<0.02f);assert(e.position(0)==5);assert(!e.playing(0));
 assert(std::abs(out[2].left)<0.0001f);
 e.seek(0,0);e.setPlaying(0,true);e.setPlaying(1,false);e.setCrossfader(0);e.setEQ(0,0,0,0);
 e.process(out,3);assert(std::abs(out[0].left-0.5f)<0.02f);
 bool threw=false;try{WavFile mismatch=a;mismatch.sampleRate=44100;e.load(0,mismatch);}catch(const std::invalid_argument&){threw=true;}
 assert(threw);
 std::cout<<"LIVE_ENGINE_TESTS_OK\n";
}
