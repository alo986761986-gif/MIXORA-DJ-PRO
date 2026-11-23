#include "../src/audio/LiveEngine.h"
#include <cassert>
#include <stdexcept>
#include <iostream>
int main(){
 mixora::LiveEngine e(48000);
 mixora::WavFile w;w.sampleRate=48000;w.frames.resize(1000);
 for(size_t i=0;i<w.frames.size();++i)w.frames[i]={0.25f,0.25f};
 e.load(0,w);e.setPlaying(0,true);e.seek(0,125);e.setHotCue(0,2);
 assert(e.hasHotCue(0,2));e.seek(0,500);assert(e.triggerHotCue(0,2));assert(e.position(0)==125);
 assert(!e.triggerHotCue(0,1));e.setLoop(0,200,300);assert(e.loopEnabled(0));assert(e.position(0)==200);
 mixora::StereoFrame master[450],cue[450];e.processBuses(master,cue,450);
 assert(e.playing(0));assert(e.position(0)==250);assert(e.loopStart(0)==200&&e.loopEnd(0)==300);
 e.clearLoop(0);assert(!e.loopEnabled(0));e.processBuses(master,cue,450);assert(e.position(0)==700);
 bool threw=false;try{e.setLoop(0,900,1001);}catch(const std::invalid_argument&){threw=true;}assert(threw);
 e.load(0,w);assert(!e.hasHotCue(0,2)&&!e.loopEnabled(0));
 std::cout<<"transport_loop_hotcue PASS\n";
}
