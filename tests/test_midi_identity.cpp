#include "../src/midi/DeviceIdentity.h"
#include "../src/midi/MidiMapping.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace mixora;
 auto original=midiIdentityKeys({"X1 MK1","Z1","Other"});
 auto reversed=midiIdentityKeys({"Other","Z1","X1 MK1"});
 assert(original[0]==reversed[2]);assert(original[1]==reversed[1]);
 assert(normalizeMidiName("  Z1  ")=="z1");
 auto twins=midiIdentityKeys({"X1 MK1","X1 MK1"});assert(twins[0]!=twins[1]);
 MidiMapping map;map.bind({"play_a",0xB0,42,true,kAnyMidiPort,original[0]});
 map.bind({"gain_a",0xB0,42,false,kAnyMidiPort,original[1]});
 assert(map.lookup(reversed[2],{0xB0,42,127})->control=="play_a");
 assert(map.lookup(reversed[1],{0xB0,42,127})->control=="gain_a");
 assert(!map.lookup("unknown",{0xB0,42,127}));
 assert(!map.lookup(0,{0xB0,42,127}));
 std::cout<<"Persistent MIDI identity/mapping: OK\n";
}
