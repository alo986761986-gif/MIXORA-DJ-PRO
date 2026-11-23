#pragma once
#include "Wav.h"
#include <array>
#include <cstddef>
#include <memory>
#include "BeatAnalysis.h"

namespace mixora {
// Two sample-locked PCM decks on one shared output timeline.
// No resampling / time stretching: both inputs must match the output sample rate.
class LiveEngine {
public:
 explicit LiveEngine(uint32_t sampleRate);
 void load(int deck, WavFile file);
 void setPlaying(int deck, bool playing);
 bool playing(int deck) const;
 void seek(int deck, size_t frame);
 size_t position(int deck) const;
 size_t length(int deck) const;
 void setCrossfader(double value);
 void setMaster(double value);
 void setGain(int deck, double gain);
 void setEQ(int deck, double low, double mid, double high);
 void process(StereoFrame* output, size_t count);
 // Four-channel routing: channel 1/2 Master, channel 3/4 Cue.
 // Both buses advance together from the same source frame positions.
 void processBuses(StereoFrame* master, StereoFrame* cue, size_t count);
 void setCue(int deck, bool enabled);
 // Transport controls in source frames. Loop ranges are [start,end).
 void setLoop(int deck, size_t start, size_t end);
 void clearLoop(int deck);
 bool loopEnabled(int deck) const;
 size_t loopStart(int deck) const;
 size_t loopEnd(int deck) const;
 void setHotCue(int deck, int slot);
 bool triggerHotCue(int deck, int slot);
 bool hasHotCue(int deck, int slot) const;

 void setCueGain(double gain);
 BeatAnalysis beatAnalysis(int deck) const;
 void setBeatAnalysis(int deck, BeatAnalysis analysis);
 void setSync(int deck, bool on);
 bool syncEnabled(int deck) const;
 void synchronize(int follower, int leader);
 double playbackRate(int deck) const;
 uint32_t sampleRate() const {return sampleRate_;}
private:
 uint32_t sampleRate_;
 Mixer mixer_;
 std::array<WavFile,2> tracks_;
 std::array<size_t,2> positions_{};
 std::array<double,2> fractionalPositions_{};
 std::array<double,2> rates_{{1.,1.}};
 std::array<BeatAnalysis,2> beat_{};
 std::array<bool,2> sync_{};
 std::array<bool,2> playing_{};
 std::array<bool,2> cueEnabled_{};
 std::array<bool,2> loopEnabled_{};
 std::array<size_t,2> loopStart_{},loopEnd_{};
 std::array<std::array<size_t,4>,2> hotCues_{};
 std::array<std::array<bool,4>,2> hotCueSet_{};
 double cueGain_=0.7;
};
}
