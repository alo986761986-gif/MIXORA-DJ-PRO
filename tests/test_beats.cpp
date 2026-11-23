#include "../src/audio/BeatAnalysis.h"
#include "../src/audio/LiveEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace mixora;
static WavFile clicks(int bpm){
 WavFile w;w.sampleRate=48000;w.frames.resize(48000*12);
 const int period=48000*60/bpm;
 for(size_t beat=0;beat<w.frames.size();beat+=period)
  for(size_t j=0;j<240&&beat+j<w.frames.size();++j){float v=float(0.8*(1.-double(j)/240));w.frames[beat+j]={v,v};}
 return w;
}
int main(){
 auto a=clicks(120),b=clicks(100);
 auto beatsA=analyzeBeats(a),beatsB=analyzeBeats(b);
 std::cout<<"detected BPM "<<beatsA.bpm<<" and "<<beatsB.bpm<<"\n";
 assert(std::abs(beatsA.bpm-120)<2);assert(std::abs(beatsB.bpm-100)<2);
 assert(beatsA.confidence>.3&&beatsB.confidence>.3);
 LiveEngine e(48000);e.load(0,a);e.load(1,b);
 e.setBeatAnalysis(0,beatsA);e.setBeatAnalysis(1,beatsB);
 e.seek(0,48000*2);e.seek(1,48000*2);e.synchronize(1,0);
 assert(e.syncEnabled(1));assert(std::abs(e.playbackRate(1)-1.2)<.03);
 const auto beforeA=e.position(0),beforeB=e.position(1);
 e.setPlaying(0,true);e.setPlaying(1,true);StereoFrame out[480];e.process(out,480);
 assert(e.position(0)>beforeA);assert(e.position(1)>beforeB);
 e.setSync(1,false);assert(e.playbackRate(1)==1.);
 WavFile silence=a;for(auto& s:silence.frames)s={0,0};assert(analyzeBeats(silence).bpm==0);
 std::cout<<"BEAT_SYNC_TESTS_OK\n";
}
