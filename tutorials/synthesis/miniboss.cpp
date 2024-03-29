// Using Adam Schmieder's sounds and 04_FM.cpp

#include <cstdio> // for printing to stdout
#include <cmath>
#include <vector>
#include <iostream>

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

// using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

float freq_of(int midi) {
    float freq = pow(2, ((midi-69)/12.0)) * 440;
    return freq;
}

// house bass 2
class electricBass : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::Sine<> car, mod;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("freq", 384.868225, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.161, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.01, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.2, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.735, 0.1, 1.0);  // Unused

    // FM index
    createInternalTriggerParameter("idx1", 10.0, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 1.816, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 1.684, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 0.5, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 0.25, 0.0, 20.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      car.freq(carBaseFreq + mod() * mModEnv() * modScale);
      float s1 = car() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    g.pushMatrix();
    g.translate(getInternalParameterValue("freq") / 300 - 2,
                getInternalParameterValue("modAmt") / 25 - 1, -4);
    float scaling = getInternalParameterValue("amplitude") * 1;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[1] = 0.001;
    mModEnv.lengths()[1] = 0.001;

    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));

    //        mModEnv.lengths()[1] = mAmpEnv.lengths()[1];

    mAmpEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
  }
};

// duck bass
class moonBass : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::Sine<> car, mod;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    addSphere(mMesh, 1.0);

    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.161, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.010, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.465, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.735, 0.1, 1.0);  // Unused

    // FM index
    createInternalTriggerParameter("idx1", 8.605, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 0.711, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 0.816, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 0.25, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 1.0, 0.0, 20.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      car.freq(carBaseFreq + mod() * mModEnv() * modScale);
      float s1 = car() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    double frequency = getInternalParameterValue("freq");
    g.pushMatrix();
    g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -4);
    float scaling = getInternalParameterValue("amplitude") * 0.7;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[1] = 0.001;
    mModEnv.lengths()[1] = 0.001;

    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));

    //        mModEnv.lengths()[1] = mAmpEnv.lengths()[1];

    mAmpEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
  }
};

class longPluck : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.385, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.01, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.752, 0.0, 1.0);
        createInternalTriggerParameter("curve", -7.276, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.056, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 0.013, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 3119.154, 10.0, 5000);
        createInternalTriggerParameter("cf2", 662.539, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 1.447, 0.1, 2);
        createInternalTriggerParameter("bw1", 624.154, 10.0, 5000);
        createInternalTriggerParameter("bw2", 3503.000, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.465, 0.1, 2);
        createInternalTriggerParameter("hmnum", 20.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.00, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(amplitude,  amplitude, -4);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * amplitude, scaling * amplitude, scaling * 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

// harp like 2
class piano : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::Sine<> car, mod;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.485, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.01, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.735, 0.1, 1.0);  // Unused

    // FM index
    createInternalTriggerParameter("idx1", 7.923, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 0.0, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 0.0, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 1.0, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 13.077, 0.0, 20.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      car.freq(carBaseFreq + mod() * mModEnv() * modScale);
      float s1 = car() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    g.pushMatrix();
    g.translate(getInternalParameterValue("freq") / 300 - 2,
                getInternalParameterValue("modAmt") / 25 - 1, -4);
    float scaling = getInternalParameterValue("amplitude") * 1;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[1] = 0.001;
    mModEnv.lengths()[1] = 0.001;

    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));

    //        mModEnv.lengths()[1] = mAmpEnv.lengths()[1];

    mAmpEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
  }
};

// SubSyn
class funkyBass : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addPrism(mMesh);

        createInternalTriggerParameter("amplitude", 0.385, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.01, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.695, 0.0, 1.0);
        createInternalTriggerParameter("curve", 2.721, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.012, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 0.569, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 10.0, 10.0, 5000);
        createInternalTriggerParameter("cf2", 4828.875, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.556, 0.1, 2);
        createInternalTriggerParameter("bw1", 10.0, 10.0, 5000);
        createInternalTriggerParameter("bw2", 13124.472, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.520, 0.1, 2);
        createInternalTriggerParameter("hmnum", 16.564, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 0.706, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -6);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

// chiptune lead (SubSyn)
class chiptuneLead : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.2, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.752, 0.0, 1.0);
        createInternalTriggerParameter("curve", -7.276, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.056, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 0.105, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 10, 10.0, 5000);
        createInternalTriggerParameter("cf2", 1362.003, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.421, 0.1, 2);
        createInternalTriggerParameter("bw1", 2648.691, 10.0, 5000);
        createInternalTriggerParameter("bw2", 10, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.59, 0.1, 2);
        createInternalTriggerParameter("hmnum", 20.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.0, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -8);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

// videogame chords SubSyn
class videoGame : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addOctahedron(mMesh);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.01, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.569, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.366, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.026, 0.0, 1.0);
        createInternalTriggerParameter("envDur", 0.317, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 10, 587.201, 5000);
        createInternalTriggerParameter("cf2", 2104.683, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.345, 0.1, 2);
        createInternalTriggerParameter("bw1", 633.750, 10.0, 5000);
        createInternalTriggerParameter("bw2", 875.802, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.1, 0.1, 2);
        createInternalTriggerParameter("hmnum", 11.8, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 0.996, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(sin(static_cast<double>(amplitude)), cos(static_cast<double>(amplitude)), -16);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

class Marimba : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;

  gam::Env<5> mAmpEnv;

  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0.7, 0.2, 0.5, 0.1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a rectangle
    addWireBox(mMesh);

    createInternalTriggerParameter("amplitude", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override
  {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    float f = getInternalParameterValue("frequency");
    mOsc1.freq(f);
    mOsc3.freq(f * 3);
    mOsc5.freq(f * 5);

    float a = getInternalParameterValue("amplitude");
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      float s1 = mAmpEnv() * (mOsc1() * a +
                              mOsc3() * (a / 3.0) +
                              mOsc5() * (a / 5.0));

      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done())
      free();
  }

  // The graphics processing function
  void onProcess(Graphics &g) override {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), -1 * cos(static_cast<double>(frequency)), -16);
    g.scale(5 * frequency/1000, 5 * frequency/1000, 1);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();

    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -16);
    g.scale(3 * frequency/1000, 3 * frequency/1000, 0.4);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();

    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -24);
    g.scale(3 * frequency/1000, 3 * frequency/1000, 0.4);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

class Kick : public SynthVoice
{
public:
    // Unit generators
    gam::Pan<> mPan;
    gam::Sine<> mOsc;
    gam::Decay<> mDecay; // Added decay envelope for pitch
    gam::AD<> mAmpEnv;   // Changed amp envelope from Env<3> to AD<>

    void init() override
    {
        // Intialize amplitude envelope
        // - Minimum attack (to make it thump)
        // - Short decay
        // - Maximum amplitude
        mAmpEnv.attack(0.01);
        mAmpEnv.decay(0.3);
        mAmpEnv.amp(1.0);

        // Initialize pitch decay
        mDecay.decay(0.3);

        createInternalTriggerParameter("amplitude", 0.2, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 150, 20, 5000);
    }

    // The audio processing function
    void onProcess(AudioIOData &io) override
    {
        mOsc.freq(getInternalParameterValue("frequency"));
        mPan.pos(0);
        // (removed parameter control for attack and release)

        while (io())
        {
            mOsc.freqMul(mDecay()); // Multiply pitch oscillator by next decay value
            float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
            float s2;
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }

        if (mAmpEnv.done())
        {
            free();
        }
    }

    void onTriggerOn() override
    {
        mAmpEnv.reset();
        mDecay.reset();
    }

    void onTriggerOff() override
    {
        mAmpEnv.release();
        mDecay.finish();
    }
};

class Snare : public SynthVoice
{
public:
    // Unit generators
    gam::Pan<> mPan;
    gam::AD<> mAmpEnv;   // Amplitude envelope
    gam::Sine<> mOsc;    // Main pitch osc (top of drum)
    gam::Sine<> mOsc2;   // Secondary pitch osc (bottom of drum)
    gam::Decay<> mDecay; // Pitch decay for oscillators
    // gam::ReverbMS<> reverb;  // Schroeder reverberator
    gam::Burst mBurst; // Noise to simulate rattle/chains

    void init() override
    {
        // Initialize burst
        mBurst = gam::Burst(10000, 5000, 0.1);
        // editing last number of burst shortens/makes sound snappier

        // Initialize amplitude envelope
        mAmpEnv.attack(0.01);
        mAmpEnv.decay(0.01);
        mAmpEnv.amp(0.005);

        // Initialize pitch decay
        mDecay.decay(0.1);

        // reverb.resize(gam::FREEVERB);
        // reverb.decay(0.5); // Set decay length, in seconds
        // reverb.damping(0.2); // Set high-frequency damping factor in [0, 1]
    }

    // The audio processing function
    void onProcess(AudioIOData &io) override
    {
        mOsc.freq(200);
        mOsc2.freq(150);

        while (io())
        {

            // Each mDecay() call moves it forward (I think), so we only want
            // to call it once per sample
            float decay = mDecay();
            mOsc.freqMul(decay);
            mOsc2.freqMul(decay);

            float amp = mAmpEnv();
            float s1 = mBurst() + (mOsc() * amp * 0.1) + (mOsc2() * amp * 0.05);
            // s1 += reverb(s1) * 0.2;
            float s2;
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }

        if (mAmpEnv.done())
            free();
    }
    void onTriggerOn() override
    {
        mBurst.reset();
        mAmpEnv.reset();
        mDecay.reset();
    }

    void onTriggerOff() override
    {
        mAmpEnv.release();
        mDecay.finish();
    }
};

class Hihat : public SynthVoice
{
public:
    // Unit generators
    gam::Pan<> mPan;
    gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>

    gam::Burst mBurst; // Resonant noise with exponential decay

    void init() override
    {
        // Initialize burst - Main freq, filter freq, duration
        mBurst = gam::Burst(20000, 15000, 0.05);
    }

    // The audio processing function
    void onProcess(AudioIOData &io) override
    {
        while (io())
        {
            float s1 = mBurst();
            float s2;
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        // Left this in because I'm not sure how to tell when a burst is done
        if (mAmpEnv.done())
            free();
    }
    void onTriggerOn() override { mBurst.reset(); }
    // void onTriggerOff() override {  }
};

class MyApp : public App, public MIDIMessageHandler
{
public:
    SynthGUIManager<moonBass> synthManager{"plunk"};
    //    ParameterMIDI parameterMIDI;
    RtMidiIn midiIn; // MIDI input carrier
    Mesh mSpectrogram;
    vector<float> spectrum;
    bool showGUI = true;
    bool showSpectro = true;
    bool navi = false;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

    virtual void onInit() override
    {
        imguiInit();
        navControl().active(false); // Disable navigation via keyboard, since we
                                    // will be using keyboard for note triggering
        // Set sampling rate for Gamma objects from app's audio
        gam::sampleRate(audioIO().framesPerSecond());
        // Check for connected MIDI devices
        if (midiIn.getPortCount() > 0)
        {
            // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
            // callback called whenever a MIDI message is received
            MIDIMessageHandler::bindTo(midiIn);

            // Open the last device found
            unsigned int port = midiIn.getPortCount() - 1;
            midiIn.openPort(port);
            printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
        }
        else
        {
            printf("Error: No MIDI devices found.\n");
        }
        // Declare the size of the spectrum
        spectrum.resize(FFT_SIZE / 2 + 1);
    }

    void playMoonBass(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<moonBass>();

        voice->setInternalParameterValue("freq", freq);
        voice->setInternalParameterValue("amplitude", amp/2);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playElectricBass(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<electricBass>();

        voice->setInternalParameterValue("freq", freq);
        voice->setInternalParameterValue("amplitude", amp/2);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playPiano(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<piano>();

        voice->setInternalParameterValue("freq", freq);
        voice->setInternalParameterValue("amplitude", amp/3);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playChiptune(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<chiptuneLead>();

        voice->setInternalParameterValue("frequency", freq);
        voice->setInternalParameterValue("amplitude", amp/2);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playFunkyBass(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<funkyBass>();

        voice->setInternalParameterValue("frequency", freq/2);
        voice->setInternalParameterValue("amplitude", amp/2);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playLongPluck(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<longPluck>();

        voice->setInternalParameterValue("frequency", freq);
        voice->setInternalParameterValue("amplitude", amp/2);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playVideoGame(float freq, float time, float duration, float amp = 0.4)
    {
        auto *voice = synthManager.synth().getVoice<videoGame>();

        voice->setInternalParameterValue("frequency", freq);
        voice->setInternalParameterValue("amplitude", amp/3);
        // voice->setInternalParameterValue("sustain", sus);

        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playMarimba(float freq, float time, float duration, float amp = .1, float attack = 0.1, float decay = 0.2)
    {
        auto *voice = synthManager.synth().getVoice<Marimba>();
        // amp, freq, attack, release, pan
        vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
        voice->setTriggerParams(params);
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playKick(float time, float duration = 0.3)
    {
        auto *voice = synthManager.synth().getVoice<Kick>();
        // amp, freq, attack, release, pan
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }
    void playSnare(float time, float duration = 0.3)
    {
        auto *voice = synthManager.synth().getVoice<Snare>();
        // amp, freq, attack, release, pan
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }
    void playHihat(float time, float duration = 0.3)
    {
        auto *voice = synthManager.synth().getVoice<Hihat>();
        // amp, freq, attack, release, pan
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playMiniboss() {
        // read json file
        std::ifstream f("/Users/gracefeng/allolib/demo1-gracefeng05/tutorials/synthesis/miniboss.json");
        // cout << "FILE" << endl;
        // cout << f.rdbuf();
        json music = json::parse(f);

        json piano1 = music["tracks"][1]["notes"];
        json piano2 = music["tracks"][2]["notes"];
        json piano3 = music["tracks"][3]["notes"];
        json pluck = music["tracks"][4]["notes"];
        json deepBass = music["tracks"][5]["notes"];
        json electricBass = music["tracks"][6]["notes"];
        json kick = music["tracks"][7]["notes"];
        json snare = music["tracks"][8]["notes"];
        json hihat = music["tracks"][9]["notes"];

        auto p1 = piano1.begin();
        auto p2 = piano2.begin();
        auto p3 = piano3.begin();
        auto pl = pluck.begin();
        auto db = deepBass.begin();
        auto eb = electricBass.begin();
        auto k = kick.begin();
        auto s = snare.begin();
        auto hh = hihat.begin();


    while(p1 != piano1.end() || p2 != piano2.end() || p3 != piano3.end() || pl != pluck.end() || 
    db != deepBass.end() || eb != electricBass.end() || k != kick.end() || s != snare.end() || hh != hihat.end())
    {
        if(p1 != piano1.end())
        {
            auto note = *p1;
            playMarimba(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++p1;
        }
        if(p2 != piano2.end())
        {
            auto note = *p2;
            playVideoGame(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++p2;
        }
        if(p3 != piano3.end())
        {
            auto note = *p3;
            playChiptune(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++p3;
        }
        if(pl != pluck.end())
        {
            auto note = *pl;
            playLongPluck(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++pl;
        }
        if(db != deepBass.end())
        {
            auto note = *db;
            playMoonBass(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++db;
        }
        if(eb != electricBass.end())
        {
            auto note = *eb;
            playFunkyBass(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++eb;
        }
        if(k != kick.end())
        {
            auto note = *k;
            playKick(note["time"], note["duration"]);
            ++k;
        }
        if(s != snare.end())
        {
            auto note = *s;
            playSnare(note["time"], note["duration"]);
            ++s;
        }
        if(hh != hihat.end())
        {
            auto note = *hh;
            playHihat(note["time"], note["duration"]);
            ++hh;
        }
    }

    }

    void onCreate() override
    {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth8.synthSequence");
        synthManager.synthRecorder().verbose(true);
    }

    void onSound(AudioIOData &io) override
    {
        synthManager.render(io); // Render audio
        // STFT
        while (io())
        {
            if (stft(io.out(0)))
            { // Loop through all the frequency bins
                for (unsigned k = 0; k < stft.numBins(); ++k)
                {
                    // Here we simply scale the complex sample
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                    // spectrum[k] = stft.bin(k).real();
                }
            }
        }        
    }

    void onAnimate(double dt) override
    {
        navControl().active(navi); // Disable navigation via keyboard, since we
        imguiBeginFrame();
        synthManager.drawSynthControlPanel();
        imguiEndFrame();
    }

    void onDraw(Graphics &g) override
    {
        g.clear();
        synthManager.render(g);
        // // Draw Spectrum
        mSpectrogram.reset();
        mSpectrogram.primitive(Mesh::LINE_STRIP);
        if (showSpectro)
        {
            for (int i = 0; i < FFT_SIZE / 2; i++)
            {
                mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
                mSpectrogram.vertex(i, spectrum[i], 0.0);
            }
            g.meshColor(); // Use the color in the mesh
            g.pushMatrix();
            g.translate(-3, -3, 0);
            g.scale(20.0 / FFT_SIZE, 100, 1.0);
            g.draw(mSpectrogram);
            g.popMatrix();
        }
        // Draw GUI
        imguiDraw();
    }
  // This gets called whenever a MIDI message is received on the port
  void onMIDIMessage(const MIDIMessage &m)
  {
    switch (m.type())
    {
    case MIDIByte::NOTE_ON:
    {
      int midiNote = m.noteNumber();
      if (midiNote > 0 && m.velocity() > 0.001)
      {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.voice()->setInternalParameterValue(
            "attackTime", 0.01 / m.velocity());
        synthManager.triggerOn(midiNote);
      }
      else
      {
        synthManager.triggerOff(midiNote);
      }
      break;
    }
    case MIDIByte::NOTE_OFF:
    {
      int midiNote = m.noteNumber();
      printf("Note OFF %u, Vel %f", m.noteNumber(), m.velocity());
      synthManager.triggerOff(midiNote);
      break;
    }
    default:;
    }
  }
    bool onKeyDown(Keyboard const &k) override
    {
        if (ParameterGUI::usingKeyboard())
        { // Ignore keys if GUI is using them
            return true;
        }
        if (k.shift())
        {
            // If shift pressed then keyboard sets preset
            int presetNumber = asciiToIndex(k.key());
            synthManager.recallPreset(presetNumber);
        }
        if (k.key('m'))
        {
            playMiniboss();
        }
        else
        {
            // Otherwise trigger note for polyphonic synth
            int midiNote = asciiToMIDI(k.key());
            if (midiNote > 0)
            {
                synthManager.voice()->setInternalParameterValue(
                    "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
                synthManager.triggerOn(midiNote);
            }
        }
        switch (k.key())
        {
        case ']':
        showGUI = !showGUI;
        break;
        case '[':
        showSpectro = !showSpectro;
        break;
        case '=':
        navi = !navi;
        break;
        }
        return true;
    }

    bool onKeyUp(Keyboard const &k) override
    {
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0)
        {
            synthManager.triggerOff(midiNote);
        }
        return true;
    }

    void onExit() override { imguiShutdown(); }
};

int main()
{
    MyApp app;

    // Set up audio
    app.configureAudio(48000., 512, 2, 0);

    app.start();
}