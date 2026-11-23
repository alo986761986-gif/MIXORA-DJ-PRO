#include "../src/midi/WinMidiInput.h"
#include "../src/midi/WinMidiOutput.h"
#include <cassert>
#include <iostream>
int main(){
 mixora::WinMidiInput in;mixora::WinMidiOutput out;
#ifndef _WIN32
 assert(in.availablePorts().empty()); assert(!in.openAll());assert(in.take().empty());
 assert(out.availablePorts().empty());assert(!out.open(0));assert(!out.send(0xB0,42,127));
#endif
 in.close();out.close();std::cout<<"MIDI port tests passed\n";
}
