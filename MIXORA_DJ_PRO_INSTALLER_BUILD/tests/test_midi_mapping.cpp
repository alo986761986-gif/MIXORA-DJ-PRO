#include "../src/midi/MidiMapping.h"
#include <cassert>
#include <iostream>
int main(){
 mixora::MidiMapping m;
 m.bind({"sync_a",0xB0,42,true});m.bind({"sync_b",0xB0,43,true});
 assert(m.lookup({0xB0,42,127})->control=="sync_a");
 assert(m.lookup({0xB0,43,127})->control=="sync_b");
 assert(!m.lookup({0xB1,42,127}));
 assert(!m.lookup({0xF8,0,0}));
 assert(mixora::midiOn({0x90,60,127}));
 assert(!mixora::midiOn({0x90,60,0}));
 assert(!mixora::midiOn({0x80,60,127}));
 m.bind({"sync_a",0xB0,52,true});assert(!m.lookup({0xB0,42,127}));assert(m.lookup({0xB0,52,127}));
 m.bind({"cross",0xB0,60,false});assert(m.lookup({0xB0,60,64})->control=="cross");
 m.unbind("cross");assert(!m.lookup({0xB0,60,64}));
 std::cout<<"MIDI mapping tests passed\n";
}
