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
 StereoFrame process(StereoFrame a, StereoFrame b);
 void reset();
private:
 double sampleRate_, cross_=0.5, master_=0.85;
 std::array<double,2> gains_{{1.,1.}};
 std::array<ThreeBandEQ,2> eq_;
};
}
