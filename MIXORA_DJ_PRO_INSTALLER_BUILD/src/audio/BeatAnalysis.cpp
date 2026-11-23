#include "BeatAnalysis.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace mixora {
BeatAnalysis analyzeBeats(const WavFile& file){
 BeatAnalysis result;
 if(file.sampleRate<8000 || file.frames.size()<file.sampleRate*4ULL)return result;
 constexpr int hz=200;
 const size_t hop=std::max<size_t>(1,file.sampleRate/hz);
 const size_t count=std::min<size_t>(file.frames.size()/hop,200*120); // max first two minutes
 std::vector<double> energy(count),onsets(count);
 for(size_t i=0;i<count;++i){
  double sum=0;
  for(size_t j=i*hop;j<std::min((i+1)*hop,file.frames.size());++j){
   const auto& f=file.frames[j];sum+=(double(f.left)*f.left+double(f.right)*f.right)*.5;
  }
  energy[i]=std::sqrt(sum/hop);
 }
 // Positive novelty removes steady amplitude and emphasizes percussive starts.
 double average=0;
 for(size_t i=1;i<count;++i){onsets[i]=std::max(0.,energy[i]-energy[i-1]);average+=onsets[i];}
 average/=count;
 if(average<1e-6)return result;
 const int minLag=hz*60/180,maxLag=hz*60/70;
 double best=-1;int bestLag=0;
 for(int lag=minLag;lag<=maxLag;++lag){
  double score=0,normA=0,normB=0;
  for(size_t i=lag;i<count;++i){
   const double a=onsets[i],b=onsets[i-lag];
   score+=a*b;normA+=a*a;normB+=b*b;
  }
  if(normA<1e-12||normB<1e-12)continue;
  score/=std::sqrt(normA*normB);
  if(score>best){best=score;bestLag=lag;}
 }
 if(bestLag==0||best<0.12)return result;
 result.bpm=60.0*hz/bestLag;
 result.confidence=std::clamp(best,0.,1.);
 // Strongest onset in the first beat period is a rough beat phase anchor.
 size_t start=0;double peak=0;
 for(size_t i=0;i<std::min<size_t>(bestLag,count);++i){if(onsets[i]>peak){peak=onsets[i];start=i;}}
 result.firstBeatSeconds=double(start*hop)/file.sampleRate;
 return result;
}
}
