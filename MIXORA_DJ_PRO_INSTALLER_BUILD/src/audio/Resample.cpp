#include "Resample.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <limits>
namespace mixora {
WavFile resampleLinear(const WavFile& input,uint32_t dst){
 if(input.sampleRate<8000 || input.sampleRate>384000 || dst<8000 || dst>384000)
  throw std::invalid_argument("Unsupported sample rate");
 WavFile result;result.sampleRate=dst;
 if(input.frames.empty())return result;
 if(input.sampleRate==dst){result.frames=input.frames;return result;}
 const long double n=static_cast<long double>(input.frames.size())*dst/input.sampleRate;
 if(n>static_cast<long double>(std::numeric_limits<size_t>::max()/sizeof(StereoFrame)))
  throw std::length_error("Resampled file too large");
 const size_t total=std::max<size_t>(1,static_cast<size_t>(std::llround(n)));
 result.frames.resize(total);
 const double ratio=static_cast<double>(input.sampleRate)/dst;
 for(size_t i=0;i<total;++i){
  const double position=static_cast<double>(i)*ratio;
  const size_t a=std::min(static_cast<size_t>(position),input.frames.size()-1);
  const size_t b=std::min(a+1,input.frames.size()-1);
  const float t=static_cast<float>(std::clamp(position-static_cast<double>(a),0.,1.));
  result.frames[i]={input.frames[a].left*(1-t)+input.frames[b].left*t,
                    input.frames[a].right*(1-t)+input.frames[b].right*t};
 }
 return result;
}
}
