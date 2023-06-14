// Copied from 08_SubSyn.cpp

#include <cstdio>  // for printing to stdout

#include <vector>
#include <random>
#include <iostream>

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/math/al_Random.hpp"

using namespace gam;
using namespace al;
using namespace std;

class Snare : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Amplitude envelope
  gam::Sine<> mOsc; // Main pitch osc (top of drum)
  gam::Sine<> mOsc2; // Secondary pitch osc (bottom of drum)
  gam::Decay<> mDecay; // Pitch decay for oscillators
//   gam::ReverbMS<> reverb;	// Schroeder reverberator
  gam::Burst mBurst; // Noise to simulate rattle/chains

  Mesh snareMesh;


  void init() override {
    // Initialize burst 
    mBurst = gam::Burst(10000, 5000, 0.3);

    // Initialize amplitude envelope
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.01);
    mAmpEnv.amp(1.0);

    // Initialize pitch decay 
    mDecay.decay(0.8);

    addRect(snareMesh, 0.3f, 0.5f);
    // reverb.resize(gam::FREEVERB);
	// 	reverb.decay(0.5); // Set decay length, in seconds
	// 	reverb.damping(0.2); // Set high-frequency damping factor in [0, 1]

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(200);
    mOsc2.freq(150);

    while (io()) {
      
      // Each mDecay() call moves it forward (I think), so we only want
      // to call it once per sample
      float decay = mDecay();
      mOsc.freqMul(decay);
      mOsc2.freqMul(decay);

      float amp = mAmpEnv();
      float s1 = mBurst() + (mOsc() * amp * 0.1)+ (mOsc2() * amp * 0.05);
    //   s1 += reverb(s1) * 0.2;
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    
    if (mAmpEnv.done()) free();
  }

  void onProcess(Graphics&g) override {
    g.pushMatrix();

    g.translate(0, -1, -4);
    g.scale(1,1,1);
    g.color(Color(0.5, 0.5, 0.5)); 
    g.draw(snareMesh);
    g.popMatrix();
  }
  void onTriggerOn() override { mBurst.reset(); mAmpEnv.reset(); mDecay.reset();}
  
  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

class Kick : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Decay<> mDecay; // Added decay envelope for pitch
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>

  void init() override {
    // Intialize amplitude envelope
    // - Minimum attack (to make it thump)
    // - Short decay
    // - Maximum amplitude
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.3);
    mAmpEnv.amp(1.0);

    // Initialize pitch decay 
    mDecay.decay(0.3);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(getInternalParameterValue("frequency"));
    mPan.pos(0);
    // (removed parameter control for attack and release)

    while (io()) {
      mOsc.freqMul(mDecay()); // Multiply pitch oscillator by next decay value
      float s1 = mOsc() *  mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }

    if (mAmpEnv.done()) free();
  }

  void onTriggerOn() override { mAmpEnv.reset(); mDecay.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

class Hihat : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>
  
  gam::Burst mBurst; // Resonant noise with exponential decay

  void init() override {
    // Initialize burst - Main freq, filter freq, duration
    mBurst = gam::Burst(20000, 15000, 0.05);

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    while (io()) {
      float s1 = mBurst();
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // Left this in because I'm not sure how to tell when a burst is done
    if (mAmpEnv.done()) free();
  }
  void onTriggerOn() override { mBurst.reset(); }
  //void onTriggerOff() override {  }
};

class Bass : public SynthVoice
{
public:
    // Unit generators
    double a = 0;
    double b = 0;
    double timepose = 0;
    Vec3f note_position;
    Vec3f note_direction;

    gam::Pan<> mPan;
    // const int static MAX_PARTIALS = 10;
    // gam::Sine<> mOsc[MAX_PARTIALS];
    gam::Sine<> mOsc;
    gam::Square<> mSquare1;
    gam::Square<> mSquare2;
    // gam::Saw<> mSaw1;
    // gam::Saw<> mSaw2;
    gam::Env<8> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;

    Mesh mMesh;

    // Initialize voice. This function will only be called once per voice when
    // it is created. Voices will be reused if they are idle.
    void init() override
    {
        // Intialize envelope
        mAmpEnv.curve(0); // make segments lines
        mAmpEnv.levels(0, 0.3, 0.7, 0.3, 0.0);
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

        addSphere(mMesh, 1, 100, 100); // Add a rectangle to the mesh
        mMesh.decompress();
        mMesh.generateNormals();
        // This is a quick way to create parameters for the voice. Trigger
        // parameters are meant to be set only when the voice starts, i.e. they
        // are expected to be constant within a voice instance. (You can actually
        // change them while you are prototyping, but their changes will only be
        // stored and aplied when a note is triggered.)

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
        // createInternalTriggerParameter("partials", 3.0, 1.0, MAX_PARTIALS);
        createInternalTriggerParameter("deltaA", 0.23, -1, 1);
        createInternalTriggerParameter("deltaB", 0.29, -1, 1);
    }

    // The audio processing function
    void onProcess(AudioIOData& io) override
    {
        // Get the values from the parameters and apply them to the corresponding
        // unit generators. You could place these lines in the onTrigger() function,
        // but placing them here allows for realtime prototyping on a running
        // voice, rather than having to trigger a new voice to hear the changes.
        // Parameters will update values once per audio callback because they
        // are outside the sample processing loop.
        double freq = getInternalParameterValue("frequency");
        mSquare1.freq(freq);
        mSquare2.freq(freq * 2.0);
        // mOsc.freq(freq * 2.0);
        // int partials = (int) floor(getInternalParameterValue("partials"));
        // for(int i = 0; i < partials; i++){
        //     mOsc[i].freq(i * freq);
        // }
        
        mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
        mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
        mPan.pos(getInternalParameterValue("pan"));
        while (io())
        {
            float s1 = (mSquare1() + mSquare2() * 0.5) * mAmpEnv() * getInternalParameterValue("amplitude");
            // for(int i = 0; i < partials; i++){
            //     s1 += (1.0/(float)(i+1)) * mOsc[i]() * mAmpEnv() * getInternalParameterValue("amplitude");
            // }
            float s2;
            mPan(s1, s1, s2);
            mEnvFollow(s1);
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
    void onProcess(Graphics& g) override
    {
        // empty if there are no graphics to draw
        a += getInternalParameterValue("deltaA");
        b += getInternalParameterValue("deltaB");
        timepose += 0.02;
        // Get the paramter values on every video frame, to apply changes to the
        // current instance
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");


        g.pushMatrix();
        g.depthTesting(true);
        g.lighting(true);
        g.translate(note_position);

        g.rotate(a, Vec3f(1, 0, 0));
        g.rotate(b, Vec3f(1));
        g.scale(0.3 + mAmpEnv() * 0.2, 0.3 + mAmpEnv() * 0.5, 1);
        g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
        g.draw(mMesh);
        g.popMatrix();
    }

    // The triggering functions just need to tell the envelope to start or release
    // The audio processing function checks when the envelope is done to remove
    // the voice from the processing chain.
    void onTriggerOn() override { 
        float frequency = getInternalParameterValue("frequency");
        float angle = frequency / 200;

        a = al::rnd::uniform();
        b = al::rnd::uniform();
        timepose = 0;
        note_position = {frequency/1000.0f - 0.5f, 0, -15};
        note_direction = {sin(angle), cos(angle), 0};
        mAmpEnv.reset(); 
    }

    void onTriggerOff() override { mAmpEnv.release(); }
};

class SquareWave : public SynthVoice
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
    mAmpEnv.levels(1, 0.5, 0.2, 0.1);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a rectangle
    addRect(mMesh);

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
    g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -8);
    g.scale(frequency / 5000, frequency / 5000, frequency / 5000);
    g.color(frequency / 1000, frequency / 1000, 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();

    addTetrahedron(mMesh, 1.0);
    g.pushMatrix();
    g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -12);
    g.scale(frequency / 5000, 2 * frequency / 5000, frequency / 5000);
    g.color(frequency / 1000, 10, 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();

    g.pushMatrix();
    g.translate(cos(static_cast<double>(frequency)), sin(static_cast<double>(frequency)), -14);
    g.scale(frequency / 5000, 3 * frequency / 5000, frequency / 5000);
    g.color(frequency / 1000, 10, frequency / 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

class WireBox : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;

  gam::Env<3> mAmpEnv;

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
    mAmpEnv.levels(0, 1, 1, 0);
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
    // Get the paramter values on every video frame, to apply changes to the
  // current instance
  float frequency = getInternalParameterValue("frequency");
  float amplitude = getInternalParameterValue("amplitude");
  // Now draw
  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), -1 * cos(static_cast<double>(frequency)), -16);
  g.scale(5 * frequency/1000, 5 * frequency/1000, 1);
  g.color(10, frequency / 1000,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();

  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -16);
  g.scale(3 * frequency/1000, 3 * frequency/1000, 0.4);
  g.color(10, 10,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();

  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -24);
  g.scale(3 * frequency/1000, 3 * frequency/1000, 0.4);
  g.color(10, 10,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

class SineEnv : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0);  // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);  // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addRect(mMesh);

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 1.0, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io()) {
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
  }

  // The graphics processing function
  void onProcess(Graphics& g) override {
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    // Now draw
    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), -1 * cos(static_cast<double>(frequency)), -4);
    g.scale(0.1, 0.1, 0.1);
    g.color(mEnvFollow.value(), frequency / 1000, mEnvFollow.value() * 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

class OscAM : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;

  gam::Env<3> mAmpEnv;

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
    mAmpEnv.levels(0, 1, 1, 0);
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
  // Get the paramter values on every video frame, to apply changes to the
  // current instance
  float frequency = getInternalParameterValue("frequency");
  float amplitude = getInternalParameterValue("amplitude");
  // Now draw
  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), -1 * cos(static_cast<double>(frequency)), -16);
  g.scale(5 * frequency/10000, 5 * frequency/10000, 1);
  g.color(10, frequency / 1000,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();

  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -16);
  g.scale(3 * frequency/10000, 3 * frequency/10000, 0.4);
  g.color(10, 10,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();

  addSurfaceLoop(mMesh, 4, 4, 2);
  g.pushMatrix();
  g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -24);
  g.scale(3 * frequency/10000, 3 * frequency/10000, 0.4);
  g.color(10, 10,  frequency / 1000, 0.4);
  g.draw(mMesh);
  g.popMatrix();
  
  if (frequency > 800) {
    addTetrahedron(mMesh); // high notes have jagged edges
    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -2); // outer edges
    g.scale(frequency/10000, 0.5 * frequency/10000, 0.4);
    g.color(255, 145, 215, 0.4); // pink
    g.draw(mMesh);
    g.popMatrix();

    g.pushMatrix();
    g.translate(sin(static_cast<double>(frequency)), tan(static_cast<double>(frequency)), -3); // outer edges
    g.scale(0.5 * frequency/10000, frequency/10000, 0.4);
    g.color(214, 0, 186, 0.4); // pink
    g.draw(mMesh);
    g.popMatrix();
  }
}

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

class MyApp : public App
{
public:

  SynthGUIManager<SineEnv> synthManager {"synth8"};
  //    ParameterMIDI parameterMIDI;

  virtual void onInit( ) override {
    imguiInit();
    navControl().active(false);  // Disable navigation via keyboard, since we
                              // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

    void onCreate() override {
        // Play example sequence. Comment this line to start from scratch
        //    synthManager.synthSequencer().playSequence("synth8.synthSequence");

        // bossa(0.0, 150, 64);

        synthManager.synthRecorder().verbose(true);
    }

    void onSound(AudioIOData& io) override {
        synthManager.render(io);  // Render audio
    }

    void onAnimate(double dt) override {
        imguiBeginFrame();
        synthManager.drawSynthControlPanel();
        imguiEndFrame();
    }

    void onDraw(Graphics& g) override {
        g.clear();
        synthManager.render(g);

        // Draw GUI
        imguiDraw();
    }

    bool onKeyDown(Keyboard const& k) override {
        if (ParameterGUI::usingKeyboard()) {  // Ignore keys if GUI is using them
        return true;
        }
        if (k.shift()) {
        // If shift pressed then keyboard sets preset
        int presetNumber = asciiToIndex(k.key());
        synthManager.recallPreset(presetNumber);
        }
        if (k.key() == 'a') {
          playArps();
        }
        /* else {
        // Otherwise trigger note for polyphonic synth
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0) {
            synthManager.voice()->setInternalParameterValue(
                "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
            synthManager.triggerOn(midiNote);
        }
        } */
        return true;
    }

    bool onKeyUp(Keyboard const& k) override {
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0) {
        synthManager.triggerOff(midiNote);
        }
        return true;
    }

    void onExit() override { imguiShutdown(); }

    // From Esme's code
    float timeElapsed(int bpm, float beatsElapsed){
    return (60 * beatsElapsed) / (bpm);
    }
  
    void playSquare(float freq, float time, float duration, float amp = .2, float attack = 0.01, float decay = 0.01) {
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    vector<VariantValue> params = vector<VariantValue>({amp, freq, 0.0, 0.0, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playBass(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    auto* voice = synthManager.synth().getVoice<Bass>();

    voice->setInternalParameterValue("frequency", freq);
    voice->setInternalParameterValue("amplitude", amp);
    voice->setInternalParameterValue("attackTime", attack);
    voice->setInternalParameterValue("releaseTime", decay);
    voice->setInternalParameterValue("pan", 0.0f);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playWireBox(float freq, float time, float duration, float amp = .2, float attack = 0.01, float decay = 0.01) {
    auto *voice = synthManager.synth().getVoice<WireBox>();
    vector<VariantValue> params = vector<VariantValue>({amp, freq, 0.0, 0.0, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playSine(float freq, float time, float duration, float amp = .2, float attack = 0.01, float decay = 0.01) {
    auto *voice = synthManager.synth().getVoice<SineEnv>();
    vector<VariantValue> params = vector<VariantValue>({amp, freq, 0.0, 0.0, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playOsc(float freq, float time, float duration, float amp = .2, float attack = 0.01, float decay = 0.01) {
    auto *voice = synthManager.synth().getVoice<OscAM>();
    vector<VariantValue> params = vector<VariantValue>({amp, freq, 0.0, 0.0, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playKick(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.01, float decay = 0.1)
  {
      auto *voice = synthManager.synth().getVoice<Kick>();
      // amp, freq, attack, release, pan
      voice->setInternalParameterValue("amp", amp);
      voice->setInternalParameterValue("attack", attack);
      voice->setInternalParameterValue("decay", decay);
      voice->setInternalParameterValue("freq", freq);
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playHihat(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Hihat>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playSnare(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Snare>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

    void upDownArp(int bpm, float startTime, int measures, const std::vector<float>& frequencies, int octaves) {
    float noteDuration = 0.5; // default to half notes.
    switch (frequencies.size())
    {
        case 1: noteDuration = 2.0; break; // 1/2
        case 2: noteDuration = 1.0; break; // 1/4
        case 3: noteDuration = 2.0 / 3.0; break; // 1/4 triplet
        case 4: noteDuration = 0.5; break; // 1/8
    }

    // total number of notes in arpeggio.
    int numNotes = measures * 4 / noteDuration;

    // total duration of arpeggio.
    float totalDuration = numNotes * noteDuration * 60.0 / bpm;

    // arpeggiate up and down specified number of octaves.
    int numSteps = frequencies.size() * octaves * 2; // double steps for ascending and descending.
    for (int i = 0; i < numNotes; i++)
    {
        // index of the current step in the arpeggio.
        int stepIndex = i % numSteps;

        // frequency of the current note.
        float frequency;
        if (stepIndex < numSteps / 2) // ascending
        {
            frequency = frequencies[stepIndex % frequencies.size()] * pow(2.0, stepIndex / frequencies.size());
        }
        else // descending
        {
            int descendingIndex = numSteps - stepIndex - 1;
            frequency = frequencies[descendingIndex % frequencies.size()] * pow(2.0, descendingIndex / frequencies.size());
        }
        playSine(frequency, startTime + i * totalDuration / numNotes, noteDuration - 0.1);
        playSquare(frequency, startTime + i * totalDuration / numNotes, noteDuration - 0.1);
    }
    }


  void chord(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float chordDuration = ((60.0/bpm) * 3) * measures;
    for (int i = 0; i < notes.size(); i++) {
      playBass(notes[i]/4, startTime, chordDuration, amp / 2, 0.0, 0.001);
      playSine(notes[i]/8, startTime, chordDuration, amp * 2, 0.0, 0.001);
    }
  }

  void playChordAndArpeggio(int bpm, float startTime, int measures, vector<float>& frequencies, int octaves)
{
  // calculate time per beat and total number of beats
  float timePerBeat = 60.0 / bpm;
  int numBeats = measures * 4;

  // calculate note duration based on number of frequencies
  float noteDuration;
  switch (frequencies.size())
  {
    case 1:
      noteDuration = 2.0; // half note
      break;
    case 2:
      noteDuration = 1.0; // quarter note
      break;
    case 3:
      noteDuration = 2.0 / 3.0; // quarter note triplet
      break;
    case 4:
      noteDuration = 0.5; // eighth note
      break;
    default:
      noteDuration = 0.5; // eighth note by default
      break;
  }

  // play chord
  for (auto freq : frequencies)
  {
    playSine(freq, startTime, noteDuration);
  }

  // arpeggiate up one octave
  for (int i = 0; i < octaves; i++)
  {
    for (int j = 0; j < frequencies.size(); j++)
    {
      float freq = frequencies[j] * pow(2.0, i + 1);
      float time = startTime + j * noteDuration / frequencies.size() + i * noteDuration * frequencies.size();
      playSine(freq, time, noteDuration / frequencies.size());
    }
  }
}

  void playArps() {
    int bpm = 130;
    vector<float> AbM7 = {440/2, 523.25/2, 783.99/2, 830.61/2};
    vector<float> Fm9 = {349.23/2, 523.25/2, 932.33/2, 783.99/2};
    vector<float> CM = {261.63/2, 392.00/2, 659.25/2, 523.25/2};
    vector<float> EbM7 = {311.13/2, 392.00/2, 523.25/2, 587.33/2};
    
    vector<float> AE = {440.00, 659.25};
    vector<float> FC = {349.23, 523.25};
    vector<float> FF = {349.23/2, 349.23};
    vector<float> Ab = {415.30, 415.30/2};

    upDownArp(bpm, 0.0, 2, AbM7, 3);
    upDownArp(bpm, timeElapsed(bpm, 8), 2, Fm9, 3);
    upDownArp(bpm, timeElapsed(bpm, 16), 2, CM, 3);
    upDownArp(bpm, timeElapsed(bpm, 24), 2, EbM7, 3);

    chord(bpm, 0.0, 2, AE, 0.1);
    chord(bpm, timeElapsed(bpm, 8), 2, FC, 0.1);
    chord(bpm, timeElapsed(bpm, 16), 2, FF, 0.1);
    chord(bpm, timeElapsed(bpm, 24), 2, Ab, 0.1);

    playChordAndArpeggio(bpm, timeElapsed(bpm, 32), 2, Fm9, 3);
  }
  
};

int main() {
  MyApp app;
  

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}