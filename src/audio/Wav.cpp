#include "Wav.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
namespace mixora {
namespace {
uint16_t u16(const unsigned char* p){return uint16_t(p[0]) | uint16_t(p[1])<<8;}
uint32_t u32(const unsigned char* p){return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;}
void put16(std::ostream& out,uint16_t v){for(int s=0;s<16;s+=8)out.put(static_cast<char>((v>>s)&255));}
void put32(std::ostream& out,uint32_t v){for(int s=0;s<32;s+=8)out.put(static_cast<char>((v>>s)&255));}
void exact(std::istream& in,char* p,std::streamsize len){if(!in.read(p,len))throw std::runtime_error("WAV truncated");}
}
WavFile readWav(const std::string& path){
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot open WAV: "+path);
 unsigned char hdr[12];exact(in,reinterpret_cast<char*>(hdr),12);
 if(std::memcmp(hdr,"RIFF",4)!=0||std::memcmp(hdr+8,"WAVE",4)!=0)throw std::runtime_error("Not RIFF/WAVE");
 const uint64_t fileEnd=uint64_t(u32(hdr+4))+8;
 if(fileEnd<12)throw std::runtime_error("Corrupt RIFF size");
 uint16_t encoding=0,channels=0,bits=0,blockAlign=0;uint32_t sr=0;bool fmtFound=false,dataFound=false;
 std::vector<unsigned char> raw;
 while(in && uint64_t(in.tellg())+8<=fileEnd){
  unsigned char chunk[8];exact(in,reinterpret_cast<char*>(chunk),8);const uint32_t len=u32(chunk+4);
  const std::streamoff start=in.tellg();if(start<0||uint64_t(start)+len>fileEnd)throw std::runtime_error("Invalid WAV chunk length");
  if(std::memcmp(chunk,"fmt ",4)==0){
   if(len<16||len>1024)throw std::runtime_error("Invalid fmt chunk");
   std::vector<unsigned char> b(len);exact(in,reinterpret_cast<char*>(b.data()),len);
   encoding=u16(b.data());channels=u16(b.data()+2);sr=u32(b.data()+4);blockAlign=u16(b.data()+12);bits=u16(b.data()+14);fmtFound=true;
  } else if(std::memcmp(chunk,"data",4)==0){
   if(len>uint32_t(1u<<30))throw std::runtime_error("WAV exceeds 1 GiB safety limit");
   raw.resize(len);exact(in,reinterpret_cast<char*>(raw.data()),len);dataFound=true;
  } else {in.seekg(len,std::ios::cur);if(!in)throw std::runtime_error("Invalid chunk seek");}
  if(len&1){char pad;exact(in,&pad,1);}
 }
 if(!fmtFound||!dataFound)throw std::runtime_error("WAV missing fmt or data");
 if(channels<1||channels>2||sr<8000||sr>192000||!((encoding==1&&bits==16)||(encoding==3&&bits==32))||blockAlign!=channels*(bits/8))throw std::runtime_error("Supports mono/stereo PCM16 or float32 WAV only");
 if(raw.size()%blockAlign)throw std::runtime_error("WAV data is not frame aligned");
 WavFile out;out.sampleRate=sr;out.frames.reserve(raw.size()/blockAlign);
 for(size_t off=0;off<raw.size();off+=blockAlign){
  auto sample=[&](int channel)->float{
   const unsigned char* p=raw.data()+off+channel*(bits/8);
   if(encoding==1)return float(static_cast<int16_t>(u16(p)))/32768.f;
   uint32_t value=u32(p);float f=0;std::memcpy(&f,&value,sizeof f);
   if(!std::isfinite(f))throw std::runtime_error("WAV contains non-finite float samples");
   return std::clamp(f,-1.f,1.f);
  };
  float l=sample(0),r=channels==2?sample(1):l;out.frames.push_back({l,r});
 }
 return out;
}
void writeWav(const std::string& path,const WavFile& audio){
 if(audio.sampleRate<8000||audio.sampleRate>192000||audio.frames.size()>(std::numeric_limits<uint32_t>::max()-36u)/4u)throw std::runtime_error("Invalid WAV output size or sample rate");
 std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot write WAV: "+path);
 const uint32_t dataSize=uint32_t(audio.frames.size()*4);
 out.write("RIFF",4);put32(out,36+dataSize);out.write("WAVEfmt ",8);put32(out,16);put16(out,1);put16(out,2);put32(out,audio.sampleRate);put32(out,audio.sampleRate*4);put16(out,4);put16(out,16);out.write("data",4);put32(out,dataSize);
 for(const auto& frame:audio.frames)for(float x:{frame.left,frame.right}){
  if(!std::isfinite(x))throw std::runtime_error("Non-finite sample");
  const float s=std::clamp(x,-1.f,1.f);
  const int v=int(std::lround(s<0?s*32768.f:s*32767.f));put16(out,static_cast<uint16_t>(static_cast<int16_t>(v)));
 }
 if(!out)throw std::runtime_error("WAV writing failed");
}
}
