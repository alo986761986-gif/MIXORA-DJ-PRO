#pragma once
#include "Wav.h"
#include <array>
namespace mixora {
struct RenderConfig {
 double crossfader=0.5;
 double masterGain=0.85;
 std::array<double,2> deckGain{1,1};
 std::array<std::array<double,3>,2> eqDb{{{0,0,0},{0,0,0}}};
 std::array<bool,2> cueOn{false,false};
 double cueGain=0.65;
};
struct RenderResult {WavFile master,cue;};
RenderResult render(const WavFile& a,const WavFile& b,const RenderConfig& settings);
}
