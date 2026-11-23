#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace mixora {
struct StereoFrame { float left=0.f, right=0.f; };
class Biquad {
public:
 enum class Type {LowShelf, Peak, HighShelf};
 void configure(Type type, double sampleRate, double freq, double gainDb, double q=0.7071067811865476);
 StereoFrame process(StereoFrame x);
 void reset();
private:
 double b0_=1,b1_=0,b2_=0,a1_=0,a2_=0;
 std::array<double,2> x1_{},x2_{},y1_{},y2_{};
};
class ThreeBandEQ {
public:
 void configure(double sampleRate, double lowDb, double midDb, double highDb);
 StereoFrame process(StereoFrame x);
 void reset();
private:
 Biquad low_,mid_,high_;
};
class Mixer {
public:
 explicit Mixer(double sampleRate=48000);
 void setCrossfader(double position); // 0=A, 1=B
 void setMasterGain(double gain); // linear 0..1
 void setDeckGain(int deck, double gain); // linear 0..2
 void setEQ(int deck, double lowDb, double midDb, double highDb);
 void setChannelFader(int deck, double level); // 0..1, four independent channel faders
 void setFilter(int deck, double value); // 0..1; 0.5 bypass; lowpass <0.5, highpass >0.5
 StereoFrame process(StereoFrame a, StereoFrame b); // backward compatible 2-channel mode
 StereoFrame process(const std::array<StereoFrame,4>& decks);
 void reset();
private:
 double sampleRate_, cross_=0.5, master_=0.85;
 std::array<double,4> gains_{{1.,1.,1.,1.}};
 std::array<double,4> faders_{{1.,1.,1.,1.}};
 std::array<double,4> filters_{{0.5,0.5,0.5,0.5}};
 std::array<ThreeBandEQ,4> eq_;
 std::array<std::array<double,2>,4> filterLow_{};
 std::array<std::array<double,2>,4> filterHigh_{};
};
}
