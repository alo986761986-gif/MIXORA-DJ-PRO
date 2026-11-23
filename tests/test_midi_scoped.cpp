#include "../src/midi/MidiMapping.h"
#include <cassert>
#include <iostream>
int main(){
 mixora::MidiMapping map;
 map.bind({"z1_gain",0xB0,42,false,0});
 map.bind({"x1_sync",0xB0,42,true,1});
 assert(map.lookup(0,{0xB0,42,127})->control=="z1_gain");
 assert(map.lookup(1,{0xB0,42,127})->control=="x1_sync");
 assert(!map.lookup(2,{0xB0,42,127}));
 assert(!map.lookup({0xB0,42,127}));
 map.bind({"x1_play",0xB0,43,true,1});
 assert(map.lookup(0,{0xB0,42,30}));
 map.bind({"z1_gain",0xB0,55,false,0});
 assert(!map.lookup(0,{0xB0,42,64}));
 assert(map.lookup(1,{0xB0,42,64}));
 assert(map.lookup(0,{0xB0,55,64}));
 map.unbind("x1_sync");assert(!map.lookup(1,{0xB0,42,127}));
 assert(!map.lookup(1,{0x90,44,127}));
 std::cout<<"Port-scoped MIDI mapping: OK\n";
}
