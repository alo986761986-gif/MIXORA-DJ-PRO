#include "Render.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace mixora {
RenderResult render(const WavFile& a,const WavFile& b,const RenderConfig& c){
 if(!a.sampleRate||a.sampleRate!=b.sampleRate)throw std::invalid_argument("WAV sample rates must match (resample externally)");
 if(!std::isfinite(c.cueGain)||c.cueGain<0||c.cueGain>1)throw std::invalid_argument("Invalid cue level");
 const size_t count=std::max(a.frames.size(),b.frames.size());
 Mixer master(a.sampleRate);master.setCrossfader(c.crossfader);master.setMasterGain(c.masterGain);
 Mixer cue(a.sampleRate);cue.setCrossfader(0.5);cue.setMasterGain(c.cueGain);
 for(int d=0;d<2;++d){
  master.setDeckGain(d,c.deckGain[d]);
  master.setEQ(d,c.eqDb[d][0],c.eqDb[d][1],c.eqDb[d][2]);
  cue.setDeckGain(d,c.cueOn[d]?c.deckGain[d]:0.0);
  cue.setEQ(d,c.eqDb[d][0],c.eqDb[d][1],c.eqDb[d][2]);
 }
 // Master and Cue buses are calculated from the same pair of sample frames;
 // they share a deterministic PCM timeline (unlike the Qt QMediaPlayer preview).
 RenderResult out;out.master.sampleRate=a.sampleRate;out.cue.sampleRate=a.sampleRate;
 out.master.frames.reserve(count);out.cue.frames.reserve(count);
 for(size_t i=0;i<count;++i){
  const StereoFrame left=i<a.frames.size()?a.frames[i]:StereoFrame{};
  const StereoFrame right=i<b.frames.size()?b.frames[i]:StereoFrame{};
  out.master.frames.push_back(master.process(left,right));
  out.cue.frames.push_back(cue.process(left,right));
 }
 return out;
}
}
