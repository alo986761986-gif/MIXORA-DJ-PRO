#pragma once
#include <cstdint>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include "WinMidiInput.h"
namespace mixora {
class WinMidiOutput {
public:
 WinMidiOutput()=default;
 WinMidiOutput(const WinMidiOutput&)=delete;
 WinMidiOutput& operator=(const WinMidiOutput&)=delete;
 ~WinMidiOutput(){close();}
 std::vector<MidiPort> availablePorts()const;
 bool open(unsigned port);
 bool send(uint8_t status,uint8_t data1,uint8_t data2);
 void close();
 bool isOpen()const;
private:
#ifdef _WIN32
 HMIDIOUT handle_=nullptr;
#endif
};
}
