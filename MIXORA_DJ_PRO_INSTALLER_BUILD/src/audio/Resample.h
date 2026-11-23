#pragma once
#include "Wav.h"
#include <cstdint>
namespace mixora {
// Linear interpolation for initial multi-rate support. Not production mastering quality.
WavFile resampleLinear(const WavFile& input, uint32_t outputRate);
}
