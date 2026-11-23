#include "Render.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
double value(const char* arg){size_t pos=0;double x=std::stod(arg,&pos);if(pos!=std::string(arg).size())throw std::invalid_argument("Expected numeric value");return x;}
}
int main(int argc,char** argv){
 if(argc<5){std::cerr<<"Usage: mixora_render deckA.wav deckB.wav master.wav cue.wav [--cross 0..1] [--master 0..1] [--cue-a] [--cue-b] [--cue-level 0..1] [--eq-a low mid high] [--eq-b low mid high]\n";return 2;}
 try{
  mixora::RenderConfig settings;
  for(int i=5;i<argc;++i){const std::string option=argv[i];
   if(option=="--cue-a")settings.cueOn[0]=true;
   else if(option=="--cue-b")settings.cueOn[1]=true;
   else if(option=="--cross"||option=="--master"||option=="--cue-level"){
    if(++i>=argc)throw std::invalid_argument("Missing option value");
    double x=value(argv[i]);
    if(option=="--cross")settings.crossfader=x;else if(option=="--master")settings.masterGain=x;else settings.cueGain=x;
   }else if(option=="--eq-a"||option=="--eq-b"){
    const int d=option=="--eq-a"?0:1;
    if(i+3>=argc)throw std::invalid_argument("Missing EQ values");
    for(int b=0;b<3;++b)settings.eqDb[d][b]=value(argv[++i]);
   }else throw std::invalid_argument("Unknown option: "+option);
  }
  auto a=mixora::readWav(argv[1]);auto b=mixora::readWav(argv[2]);
  auto result=mixora::render(a,b,settings);
  mixora::writeWav(argv[3],result.master);mixora::writeWav(argv[4],result.cue);
  std::cout<<"Rendered "<<result.master.frames.size()<<" stereo frames at "<<result.master.sampleRate<<" Hz\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
}
