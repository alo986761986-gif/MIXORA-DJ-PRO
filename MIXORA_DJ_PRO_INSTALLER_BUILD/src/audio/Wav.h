#pragma once
#include "../dsp/Mixer.h"
#include <cstdint>
#include <string>
#include <vector>
namespace mixora {
struct WavFile {
 uint32_t sampleRate=0;
 std::vector<StereoFrame> frames;
};
WavFile readWav(const std::string& path);
void writeWav(const std::string& path,const WavFile& audio);
}
