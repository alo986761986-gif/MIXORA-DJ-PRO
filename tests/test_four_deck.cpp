#include "../src/audio/LiveEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 mixora::LiveEngine e(48000); e.setMaster(1);e.setCrossfader(0);
 mixora::WavFile w;w.sampleRate=48000;w.frames.assign(256,{0.25f,0.25f});
 for(int d=0;d<4;++d){e.load(d,w);e.setPlaying(d,true);e.setCue(d,true);}
 mixora::StereoFrame out[64],cue[64];e.processBuses(out,cue,64);
 assert(e.position(0)==64&&e.position(1)==64&&e.position(2)==64&&e.position(3)==64);
 assert(std::abs(out[0].left-0.5)<0.02);
 assert(std::abs(cue[0].left-0.25)<0.02);
 e.setChannelFader(2,0);e.processBuses(out,cue,64);
 assert(std::abs(out[0].left-0.25)<0.02);
 e.setFilter(3,1.);e.setEQ(3,0,0,0);e.setLoop(3,40,80);
 e.processBuses(out,cue,64);
 assert(e.position(3)>=40&&e.position(3)<80);
 assert(std::isfinite(out[0].left));std::cout<<"FOUR_DECKS_LIVE_OK\n";
}