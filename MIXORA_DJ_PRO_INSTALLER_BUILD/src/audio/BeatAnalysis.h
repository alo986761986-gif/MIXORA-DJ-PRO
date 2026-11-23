#pragma once
#include "Wav.h"
namespace mixora {
struct BeatAnalysis {double bpm=0; double confidence=0; double firstBeatSeconds=0;};
// Heuristic onset-autocorrelation estimate, not a full beat grid detector.
BeatAnalysis analyzeBeats(const WavFile& file);
}
