#include "WinMidiInput.h"
#include "DeviceIdentity.h"
#ifdef _WIN32
namespace mixora {
void CALLBACK WinMidiInput::callback(HMIDIIN,UINT msg,DWORD_PTR instance,DWORD_PTR a,DWORD_PTR) {
 if(msg!=MIM_DATA || !instance)return;
 const auto* context=reinterpret_cast<const CallbackContext*>(instance);
 auto* self=context->owner;
 const auto word=static_cast<uint32_t>(a);
 const unsigned status=word&0xff;
 const unsigned kind=status&0xf0;
 if(kind!=0xB0 && kind!=0x90 && kind!=0x80)return;
 std::lock_guard<std::mutex> lock(self->mutex_);
 if(self->pending_.size()<1024)self->pending_.push_back({context->port,static_cast<uint8_t>(status),static_cast<uint8_t>((word>>8)&0x7f),static_cast<uint8_t>((word>>16)&0x7f)});
}
bool WinMidiInput::openAll(){
 close();
 for(UINT i=0;i<midiInGetNumDevs();++i){
  Connection connection;
  connection.context=std::make_unique<CallbackContext>();connection.context->owner=this;connection.context->port=i;
  if(midiInOpen(&connection.handle,i,reinterpret_cast<DWORD_PTR>(&callback),reinterpret_cast<DWORD_PTR>(connection.context.get()),CALLBACK_FUNCTION)!=MMSYSERR_NOERROR)continue;
  if(midiInStart(connection.handle)!=MMSYSERR_NOERROR){midiInClose(connection.handle);continue;}
  handles_.push_back(std::move(connection));
 }
 portsOpen_=static_cast<unsigned>(handles_.size());return portsOpen_!=0;
}
void WinMidiInput::close(){
 for(auto& connection:handles_){midiInStop(connection.handle);midiInReset(connection.handle);midiInClose(connection.handle);}
 handles_.clear();portsOpen_=0;
 std::lock_guard<std::mutex> lock(mutex_);pending_.clear();
}
std::vector<MidiPort> WinMidiInput::availablePorts()const{
 std::vector<MidiPort> ports;
 for(UINT i=0;i<midiInGetNumDevs();++i){MIDIINCAPSA caps{};if(midiInGetDevCapsA(i,&caps,sizeof(caps))==MMSYSERR_NOERROR)ports.push_back({i,caps.szPname,{}});}
 std::vector<std::string> names;for(const auto& port:ports)names.push_back(port.name);
 const auto keys=midiIdentityKeys(names);
 for(size_t i=0;i<ports.size();++i)ports[i].stableKey=keys[i];
 return ports;
}
std::vector<IncomingMidi> WinMidiInput::take(){std::lock_guard<std::mutex> lock(mutex_);std::vector<IncomingMidi> events;events.swap(pending_);return events;}
}
#else
namespace mixora {
bool WinMidiInput::openAll(){close();return false;}
void WinMidiInput::close(){portsOpen_=0;std::lock_guard<std::mutex> lock(mutex_);pending_.clear();}
std::vector<MidiPort> WinMidiInput::availablePorts()const{return {};}
std::vector<IncomingMidi> WinMidiInput::take(){std::lock_guard<std::mutex> lock(mutex_);std::vector<IncomingMidi> events;events.swap(pending_);return events;}
}
#endif
