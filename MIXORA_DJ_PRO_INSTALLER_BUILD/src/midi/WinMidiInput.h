#pragma once
#include <cstdint>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
namespace mixora {
struct IncomingMidi { unsigned device=0; uint8_t status=0,data1=0,data2=0; };
struct MidiPort { unsigned id=0; std::string name; std::string stableKey; };
class WinMidiInput {
public:
 WinMidiInput()=default;
 WinMidiInput(const WinMidiInput&)=delete;
 WinMidiInput& operator=(const WinMidiInput&)=delete;
 ~WinMidiInput(){close();}
 bool openAll();
 void close();
 std::vector<IncomingMidi> take();
 std::vector<MidiPort> availablePorts()const;
 unsigned portsOpen()const{return portsOpen_;}
private:
#ifdef _WIN32
 struct CallbackContext { WinMidiInput* owner=nullptr;unsigned port=0; };
 static void CALLBACK callback(HMIDIIN,UINT,DWORD_PTR,DWORD_PTR,DWORD_PTR);
 struct Connection { HMIDIIN handle=nullptr; std::unique_ptr<CallbackContext> context; };
 std::vector<Connection> handles_;
#endif
 std::mutex mutex_;
 std::vector<IncomingMidi> pending_;
 unsigned portsOpen_=0;
};
}
