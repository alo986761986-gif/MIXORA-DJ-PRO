#include "../src/audio/Render.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace mixora;
static bool near(float a,float b,float eps=0.0001f){return std::abs(a-b)<eps;}
int main(){
 WavFile a,b;a.sampleRate=b.sampleRate=48000;a.frames.assign(256,{.25f,-.25f});b.frames.assign(128,{.5f,.5f});
 RenderConfig c;c.crossfader=0;c.masterGain=1;c.cueOn={false,true};c.cueGain=1;
 auto r=render(a,b,c);
 assert(r.master.frames.size()==256&&r.cue.frames.size()==256);
 assert(near(r.master.frames[0].left,.25)&&near(r.master.frames[0].right,-.25));
 assert(near(r.cue.frames[0].left,.5f/std::sqrt(2.f)));
 assert(near(r.cue.frames[200].left,0));
 c.crossfader=1;auto r2=render(a,b,c);assert(near(r2.master.frames[0].left,.5)&&near(r2.master.frames[200].left,0));
 bool failed=false;b.sampleRate=44100;try{(void)render(a,b,c);}catch(const std::invalid_argument&){failed=true;}assert(failed);b.sampleRate=48000;
 c.crossfader=0;c.eqDb[0]={-9,0,0};auto r3=render(a,b,c);assert(std::isfinite(r3.master.frames[255].left));
 const std::string file="mixora_test_roundtrip.wav";writeWav(file,a);auto loaded=readWav(file);std::remove(file.c_str());
 assert(loaded.sampleRate==48000&&loaded.frames.size()==256&&near(loaded.frames[0].left,.25f,1.f/32768));
 std::cout<<"Render core: 7 checks passed\n";
}
