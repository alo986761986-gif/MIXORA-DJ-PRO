#include "LiveEngine.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>
namespace mixora {
LiveEngine::LiveEngine(uint32_t rate):sampleRate_(rate),mixer_(rate){}
void LiveEngine::load(int deck,WavFile file){
 if(deck<0||deck>1)throw std::invalid_argument("Deck must be 0 or 1");
 if(file.sampleRate!=sampleRate_)throw std::invalid_argument("Track sample rate differs from engine");
 tracks_[deck]=std::move(file);positions_[deck]=0;fractionalPositions_[deck]=0;rates_[deck]=1;beat_[deck]={};sync_[deck]=false;playing_[deck]=false;loopEnabled_[deck]=false;loopStart_[deck]=loopEnd_[deck]=0;hotCueSet_[deck].fill(false);
 mixer_.reset();
}
void LiveEngine::setPlaying(int d,bool p){if(d<0||d>1)throw std::invalid_argument("Deck");playing_[d]=p && positions_[d]<tracks_[d].frames.size();}
bool LiveEngine::playing(int d)const{return d>=0&&d<2&&playing_[d];}
void LiveEngine::seek(int d,size_t pos){if(d<0||d>1)throw std::invalid_argument("Deck");positions_[d]=std::min(pos,tracks_[d].frames.size());fractionalPositions_[d]=double(positions_[d]);mixer_.reset();}
size_t LiveEngine::position(int d)const{return (d>=0&&d<2)?positions_[d]:0;}
size_t LiveEngine::length(int d)const{return (d>=0&&d<2)?tracks_[d].frames.size():0;}
void LiveEngine::setLoop(int d,size_t start,size_t end){
 if(d<0||d>1)throw std::invalid_argument("Deck");
 if(start>=end||end>length(d))throw std::invalid_argument("Invalid loop bounds");
 loopStart_[d]=start;loopEnd_[d]=end;loopEnabled_[d]=true;
 if(fractionalPositions_[d]<double(start)||fractionalPositions_[d]>=double(end))seek(d,start);
}
void LiveEngine::clearLoop(int d){if(d<0||d>1)throw std::invalid_argument("Deck");loopEnabled_[d]=false;}
bool LiveEngine::loopEnabled(int d)const{return d>=0&&d<2&&loopEnabled_[d];}
size_t LiveEngine::loopStart(int d)const{return d>=0&&d<2?loopStart_[d]:0;}
size_t LiveEngine::loopEnd(int d)const{return d>=0&&d<2?loopEnd_[d]:0;}
void LiveEngine::setHotCue(int d,int slot){
 if(d<0||d>1||slot<0||slot>=4)throw std::invalid_argument("Hot cue index");
 if(!length(d))throw std::invalid_argument("No track loaded");
 hotCues_[d][slot]=std::min(position(d),length(d)-1);hotCueSet_[d][slot]=true;
}
bool LiveEngine::triggerHotCue(int d,int slot){
 if(d<0||d>1||slot<0||slot>=4)throw std::invalid_argument("Hot cue index");
 if(!hotCueSet_[d][slot])return false;
 seek(d,hotCues_[d][slot]);return true;
}
bool LiveEngine::hasHotCue(int d,int slot)const{return d>=0&&d<2&&slot>=0&&slot<4&&hotCueSet_[d][slot];}
void LiveEngine::setCrossfader(double x){mixer_.setCrossfader(x);}
void LiveEngine::setMaster(double m){mixer_.setMasterGain(m);}
void LiveEngine::setGain(int d,double g){mixer_.setDeckGain(d,g);}
void LiveEngine::setEQ(int d,double l,double m,double h){mixer_.setEQ(d,l,m,h);}
void LiveEngine::setCue(int d,bool enabled){
 if(d<0||d>1)throw std::invalid_argument("Deck");
 cueEnabled_[d]=enabled;
}
void LiveEngine::setCueGain(double gain){
 if(!std::isfinite(gain))throw std::invalid_argument("Invalid Cue gain");
 cueGain_=std::clamp(gain,0.,1.);
}
BeatAnalysis LiveEngine::beatAnalysis(int d) const {return (d>=0&&d<2)?beat_[d]:BeatAnalysis{};}
void LiveEngine::setBeatAnalysis(int d,BeatAnalysis a){if(d<0||d>1)throw std::invalid_argument("Deck");beat_[d]=a;}
bool LiveEngine::syncEnabled(int d)const{return d>=0&&d<2&&sync_[d];}
double LiveEngine::playbackRate(int d)const{return(d>=0&&d<2)?rates_[d]:1.;}
void LiveEngine::setSync(int d,bool on){if(d<0||d>1)throw std::invalid_argument("Deck");sync_[d]=on;if(!on)rates_[d]=1.;}
void LiveEngine::synchronize(int follower,int leader){
 if(follower<0||follower>1||leader<0||leader>1||follower==leader)throw std::invalid_argument("Invalid deck pair");
 const auto a=beat_[follower],b=beat_[leader];
 if(a.bpm<=0||b.bpm<=0)throw std::invalid_argument("BPM analysis required for both decks");
 // Single-speed resampling changes pitch: not a pitch-preserving time stretcher.
 const double targetRate=b.bpm*rates_[leader]/a.bpm;
 if(targetRate<0.5||targetRate>2.)throw std::invalid_argument("SYNC tempo ratio out of range");
 rates_[follower]=targetRate;sync_[follower]=true;
 const double followerPeriod=60./a.bpm*sampleRate_;
 const double leaderPeriod=60./b.bpm*sampleRate_;
 const double leaderPhase=std::fmod((fractionalPositions_[leader]-b.firstBeatSeconds*sampleRate_)/leaderPeriod,1.);
 const double phase=leaderPhase<0?leaderPhase+1.:leaderPhase;
 const double proposed=a.firstBeatSeconds*sampleRate_+phase*followerPeriod;
 const double current=fractionalPositions_[follower];
 // Choose closest equivalent beat position to current playback position.
 double shifted=proposed+std::round((current-proposed)/followerPeriod)*followerPeriod;
 shifted=std::clamp(shifted,0.,double(tracks_[follower].frames.size()));
 fractionalPositions_[follower]=shifted;positions_[follower]=size_t(shifted);
 mixer_.reset();
}
void LiveEngine::processBuses(StereoFrame* master,StereoFrame* cue,size_t n){
 if((!master||!cue)&&n)throw std::invalid_argument("Null output");
 for(size_t i=0;i<n;++i){
  StereoFrame in[2]{};
  for(int d=0;d<2;++d){
   if(playing_[d]){
    if(loopEnabled_[d] && fractionalPositions_[d]>=double(loopEnd_[d])){
     const double span=double(loopEnd_[d]-loopStart_[d]);
     fractionalPositions_[d]=double(loopStart_[d])+std::fmod(fractionalPositions_[d]-double(loopStart_[d]),span);
     positions_[d]=size_t(fractionalPositions_[d]);
    }
    if(fractionalPositions_[d]<tracks_[d].frames.size()){
     const auto index=size_t(fractionalPositions_[d]);
     const auto next=std::min(index+1,tracks_[d].frames.size()-1);
     const double frac=fractionalPositions_[d]-double(index);
     const auto a=tracks_[d].frames[index],b=tracks_[d].frames[next];
     in[d]={float(a.left+(b.left-a.left)*frac),float(a.right+(b.right-a.right)*frac)};
     fractionalPositions_[d]+=rates_[d];
     if(loopEnabled_[d] && fractionalPositions_[d]>=double(loopEnd_[d])){
      const double span=double(loopEnd_[d]-loopStart_[d]);
      fractionalPositions_[d]=double(loopStart_[d])+std::fmod(fractionalPositions_[d]-double(loopStart_[d]),span);
     }
     positions_[d]=std::min(size_t(fractionalPositions_[d]),tracks_[d].frames.size());
    }
    if(fractionalPositions_[d]>=tracks_[d].frames.size())playing_[d]=false;
   }
  }
  master[i]=mixer_.process(in[0],in[1]);
  // Pre-fader and pre-EQ Cue: crossfader and Master do not alter headphone bus.
  const double attenuation=(cueEnabled_[0]&&cueEnabled_[1])?0.5:1.0;
  const double a=cueEnabled_[0]?attenuation*cueGain_:0.0;
  const double b=cueEnabled_[1]?attenuation*cueGain_:0.0;
  cue[i]={static_cast<float>(std::clamp(in[0].left*a+in[1].left*b,-1.,1.)),
          static_cast<float>(std::clamp(in[0].right*a+in[1].right*b,-1.,1.))};
 }
}
void LiveEngine::process(StereoFrame* out,size_t n){
 if(!out&&n)throw std::invalid_argument("Null output");
 // Retain the stereo Master API for existing tests and standalone output.
 std::vector<StereoFrame> cue(n);
 processBuses(out,cue.data(),n);
}
}
