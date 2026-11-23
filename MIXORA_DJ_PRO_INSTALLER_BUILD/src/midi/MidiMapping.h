#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
namespace mixora {
constexpr unsigned kAnyMidiPort = std::numeric_limits<unsigned>::max();
struct MidiMessage { uint8_t status=0,data1=0,data2=0; };
struct MidiBinding { std::string control; uint8_t status=0,data1=0; bool button=false; unsigned port=kAnyMidiPort; std::string deviceKey; };
inline bool validMidiMessage(MidiMessage m) { const unsigned kind=m.status&0xF0; return (kind==0x90 || kind==0x80 || kind==0xB0) && m.data1<=127 && m.data2<=127; }
inline bool midiOn(MidiMessage m) { return (m.status&0xF0)!=0x80 && m.data2>0; }
class MidiMapping {
public:
 void bind(const MidiBinding& b) {
  if(b.control.empty() || !validMidiMessage({b.status,b.data1,0}))return;
  for(auto it=bindings_.begin();it!=bindings_.end();) {
   if(it->control==b.control || (((!b.deviceKey.empty() && it->deviceKey==b.deviceKey) || (b.deviceKey.empty() && it->deviceKey.empty() && it->port==b.port)) && it->status==b.status && it->data1==b.data1))it=bindings_.erase(it);else ++it;
  }
  bindings_.push_back(b);
 }
 void unbind(const std::string& control) {
  for(auto it=bindings_.begin();it!=bindings_.end();)if(it->control==control)it=bindings_.erase(it);else ++it;
 }
 // Actual hardware events MUST use port-aware lookup to prevent collisions.
 const MidiBinding* lookup(unsigned port,MidiMessage m) const {
  if(!validMidiMessage(m))return nullptr;
  for(const auto& b:bindings_)if(b.port==port && b.status==m.status && b.data1==m.data1)return &b;
  return nullptr;
 }
 // Stable identity match is deliberately strict: no fallback to a changing port number.
 const MidiBinding* lookup(const std::string& deviceKey,MidiMessage m) const {
  if(deviceKey.empty() || !validMidiMessage(m))return nullptr;
  for(const auto& b:bindings_)if(b.deviceKey==deviceKey && b.status==m.status && b.data1==m.data1)return &b;
  return nullptr;
 }
 // Compatibility-only lookup used by old standalone tests; never used by the live MIDI reader.
 const MidiBinding* lookup(MidiMessage m) const {return lookup(kAnyMidiPort,m);}
 const std::vector<MidiBinding>& bindings() const {return bindings_;}
private:std::vector<MidiBinding> bindings_;
};
}
