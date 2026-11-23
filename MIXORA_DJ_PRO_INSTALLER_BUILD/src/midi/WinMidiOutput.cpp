#include "WinMidiOutput.h"
#ifdef _WIN32
namespace mixora {
std::vector<MidiPort> WinMidiOutput::availablePorts()const{
 std::vector<MidiPort> ports;
 for(UINT i=0;i<midiOutGetNumDevs();++i){MIDIOUTCAPSA caps{};if(midiOutGetDevCapsA(i,&caps,sizeof(caps))==MMSYSERR_NOERROR)ports.push_back({i,caps.szPname});}
 return ports;
}
bool WinMidiOutput::open(unsigned port){
 close();return midiOutOpen(&handle_,port,0,0,CALLBACK_NULL)==MMSYSERR_NOERROR;
}
bool WinMidiOutput::send(uint8_t status,uint8_t data1,uint8_t data2){
 if(!handle_ || data1>127 || data2>127)return false;
 const uint8_t kind=status&0xf0;
 if(kind!=0x80 && kind!=0x90 && kind!=0xb0)return false;
 DWORD msg=static_cast<DWORD>(status)|(static_cast<DWORD>(data1)<<8)|(static_cast<DWORD>(data2)<<16);
 return midiOutShortMsg(handle_,msg)==MMSYSERR_NOERROR;
}
void WinMidiOutput::close(){if(handle_){midiOutReset(handle_);midiOutClose(handle_);handle_=nullptr;}}
bool WinMidiOutput::isOpen()const{return handle_!=nullptr;}
}
#else
namespace mixora {
std::vector<MidiPort> WinMidiOutput::availablePorts()const{return {};}
bool WinMidiOutput::open(unsigned){return false;}
bool WinMidiOutput::send(uint8_t,uint8_t,uint8_t){return false;}
void WinMidiOutput::close(){}
bool WinMidiOutput::isOpen()const{return false;}
}
#endif
