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
#include "Gamma/Spatial.h"
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

class Spectrogram : public SynthVoice {
  public:
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE/4, 0, gam::HANN, gam::MAG_FREQ);

  Mesh mSpectrogram;
  vector<float> spectrum;
  Mesh mMesh;

  void init() override {
    mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0,1,1,0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // Declare the size of the spectrum
    spectrum.resize(FFT_SIZE);
    mSpectrogram.primitive(Mesh::LINE_LOOP);
    // mSpectrogram.primitive(Mesh::POINTS);

    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  void onProcess(AudioIOData& io) override {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    float f = getInternalParameterValue("frequency");
    mOsc.freq(f);
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while(io()){
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      mEnvFollow(s1);
      io.out(0) += s1;
      io.out(1) += s2;

      if(stft(s1)){
        // loop through all frequency bins and scale the complex sample
        for(unsigned k = 0; k < stft.numBins(); ++k){
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
        }
      }
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if(mAmpEnv.done()) free();
  }

  void onProcess(Graphics& g) override {
    // Figure out graphics for chords later

    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");

    mSpectrogram.reset();

    for(int i = 0; i < FFT_SIZE / 90; i++){
      mSpectrogram.color(HSV(frequency/500 - spectrum[i] * 50));
      mSpectrogram.vertex(i, spectrum[i], 0.0);
    }

    for(int i = -5; i <= 4; i++) {
      g.meshColor();
      g.pushMatrix();
      // g.translate(-0.5f, 1, -10);
      // g.translate(cos(static_cast<double>(frequency)), sin(static_cast<double>(frequency)), -4);
      g.translate(i, -2.7, -10);
      g.scale(50.0/FFT_SIZE, 250, 1.0);
      // g.pointSize(1 + 5 * mEnvFollow.value() * 10);
      g.lineWidth(1 + 5 * mEnvFollow.value() * 100);
      g.draw(mSpectrogram);
      g.popMatrix();
    }
  }

  void onTriggerOn() override {
    mAmpEnv.reset();
  }

  void onTriggerOff() override {
    mAmpEnv.release();
  }
};

class SquareWave : public SynthVoice
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
    addTorus(mMesh);

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
    g.translate(-1 * sin(static_cast<double>(frequency)), -1 * cos(static_cast<double>(frequency)), -10);
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
  // SynthGUIManager<SineEnv> synthManager {"synth8"};
  //    ParameterMIDI parameterMIDI;

  SynthGUIManager<Spectrogram> synthManager {"Spectrogram"};

    Mesh mSpectrogram;
    vector<float> spectrum;
    
    // Time constants
    const float beat = 0.5;
    const float measure = beat * 4.0f;
    const float sixteenth = beat / 4.0f;
    const float eighth = beat / 2.0f;
    const float half = beat * 2.0f;
    const float whole = beat * 4.0f;

    // Pitch constants
    const float Bb4 = 466.16;
    const float C5 = 523.25;
    const float Db5 = 554.37;
    const float D5 = 587.33;
    const float Eb5 = 622.25;
    const float F5 = 698.46;
    const float G5 = 783.99;
    const float A5 = 880.00;
    const float Ab5 = 830.61;

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
        if (k.key() == 'i') {
          playComp();
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

    void playSpectrogram(float freq, float offset, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    auto* voice = synthManager.synth().getVoice<Spectrogram>();

    voice->setInternalParameterValue("frequency", freq * offset);
    voice->setInternalParameterValue("amplitude", amp);
    voice->setInternalParameterValue("attackTime", attack);
    voice->setInternalParameterValue("releaseTime", decay);
    voice->setInternalParameterValue("pan", 0.0f);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

  void arpeggiator(int bpm, float startTime, int measures, const std::vector<float>& frequencies, int octaves) {
    // calculate duration of single note based on the number of frequencies.
    float noteDuration;
    switch (frequencies.size()) {
      case 1: noteDuration = 60.0 / bpm * 2.0; break; // 
      case 2: noteDuration = 60.0 / bpm; break; // 1/8 note
      case 3: noteDuration = 60.0 / bpm * (2.0 / 3.0); break; // 1/4 note triplet
      case 4: noteDuration = 60.0 / bpm * 0.5; break; // 1/8 note
      case 6: noteDuration = 60.0/ bpm * (1.0 / 3.0);
    }

    int numNotes = measures * 4 / noteDuration;
    float totalDuration = numNotes * noteDuration * 60.0 / bpm;

    // arpeggiate up and down specified number of octaves.
    int numSteps = frequencies.size() * octaves * 2; // double steps for ascending and descending.
    for (int i = 0; i < numNotes; i++) {
      // calculate index of current step in arpeggio.
      int stepIndex = i % numSteps;

      // calculate frequency of current note.
      float curFreq;
      if (stepIndex < numSteps / 2) { // ascending
                curFreq = frequencies[stepIndex % frequencies.size()] * pow(2.0, stepIndex / frequencies.size());
      }
      else { // descending
        int descendingIndex = numSteps - stepIndex - 1;
        curFreq = frequencies[descendingIndex % frequencies.size()] * pow(2.0, descendingIndex / frequencies.size());
      }

      playSquare(curFreq, startTime + i * totalDuration / numNotes, noteDuration, 0.05, 0.0, 0.001);
    }
  }

  void arpSquare(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float noteDuration = ((60.0/bpm) * 3)/notes.size(); // total length of 1 measure divided by number of notes
    int totalNotes = notes.size() * measures;
    int curNote = 0;

    for (int i = 0; i < measures; i++) {
      for (int j = 0; j < notes.size(); j++) {
        playSquare(notes[j], startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        playSpectrogram(notes[j], 0.5f, startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        curNote++;
      }
    }
  }

  void arpWireBox(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float noteDuration = ((60.0/bpm) * 3)/notes.size(); // total length of 1 measure divided by number of notes
    int totalNotes = notes.size() * measures;
    int curNote = 0;

    for (int i = 0; i < measures; i++) {
      for (int j = 0; j < notes.size(); j++) {
        playWireBox(notes[j], startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        playSpectrogram(notes[j], 0.5f, startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        curNote++;
      }
    }
  }

  void arpSine(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float noteDuration = ((60.0/bpm) * 3)/notes.size(); // total length of 1 measure divided by number of notes
    int totalNotes = notes.size() * measures;
    int curNote = 0;

    for (int i = 0; i < measures; i++) {
      for (int j = 0; j < notes.size(); j++) {
        playSine(notes[j], startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        playSpectrogram(notes[j], 0.5f, startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        curNote++;
      }
    }
  }

  void arpSpectrogram(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float noteDuration = ((60.0/bpm) * 3)/notes.size(); // total length of 1 measure divided by number of notes
    int totalNotes = notes.size() * measures;
    int curNote = 0;

    for (int i = 0; i < measures; i++) {
      for (int j = 0; j < notes.size(); j++) {
        playSpectrogram(notes[j], 0.5f, startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        curNote++;
      }
    }
  }

  void arpOsc(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float noteDuration = ((60.0/bpm) * 3)/notes.size(); // total length of 1 measure divided by number of notes
    int totalNotes = notes.size() * measures;
    int curNote = 0;

    for (int i = 0; i < measures; i++) {
      for (int j = 0; j < notes.size(); j++) {
        playOsc(notes[j], startTime + (curNote * noteDuration), noteDuration - 0.05, amp, 0.0, 0.05);
        curNote++;
      }
    }
  }

  void chord(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float chordDuration = ((60.0/bpm) * 3) * measures;
    for (int i = 0; i < notes.size(); i++) {
      playWireBox(notes[i], startTime, chordDuration, amp, 0.0, 0.001);
      playSine(notes[i], startTime, chordDuration, amp, 0.0, 0.001);
    }
  }

    void mainTheme(int bpm, float startTime, int reps) {
      float addedTime = 0.0;

      vector<float> AEA = {880.00/2, 1318.51/2, 880.00/2};
      vector<float> BEB = {987.77/2, 1318.51/2, 987.77/2};
      vector<float> CEC = {1046.50/2, 1318.51/2, 1046.50/2};
      vector<float> DED = {1174.66/2, 1318.51/2, 1174.66/2};
      vector<float> DEB = {1174.66/2, 1318.51/2, 987.77/2};

      for (int i = 0; i < reps; i++) {
        /* arpSquare(bpm, startTime + addedTime, 2, AEA, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, BEB, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, CEC, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 18), 1, DED, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 21), 1, DEB, 0.1);
        */

        /* arpSine(bpm, startTime + addedTime, 2, AEA, 0.1);
        arpSine(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, BEB, 0.05);
        arpSine(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, CEC, 0.05);
        arpSine(bpm, startTime + addedTime + timeElapsed(bpm, 18), 1, DED, 0.05);
        arpSine(bpm, startTime + addedTime + timeElapsed(bpm, 21), 1, DEB, 0.05);
        */ 
       
        arpSpectrogram(bpm, startTime + addedTime, 2, AEA, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, BEB, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, CEC, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 18), 1, DED, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 21), 1, DEB, 0.1);
        
        addedTime += timeElapsed(bpm, 24);
      }
    }

    void layer1(int bpm, float startTime, int reps) {
      float addedTime = 0.0;

      vector<float> CBABCEABCECB = {261.63, 246.94, 220.00, 246.94, 261.63, 329.63, 440.00, 493.88, 523.25, 659.25, 523.25, 493.88};
      vector<float> BAGABEBGBAGA = {493.88, 440.00, 392.00, 440.00, 493.88, 659.25, 493.88, 392.00, 493.88, 440.00, 392.00, 440.00};
      vector<float> BAGABEBAGFED = {493.88, 440.00, 392.00, 440.00, 493.88, 659.25, 493.88, 440.00, 392.00, 349.23, 329.63, 293.66};
      vector<float> CBABCEABCECBup = {261.63, 246.94, 220.00, 246.94, 261.63, 329.63, 440.00, 493.88, 523.25, 659.25, 523.25*2, 493.88*2};
      vector<float> CBAGAGFEFEDC = {1046.50, 987.77, 880.00, 783.99, 880.00, 783.99, 698.46, 659.25, 698.46, 659.25, 587.33, 523.25};
      vector<float> DEBCDEDEBCDE = {587.33, 659.25, 493.88, 523.25, 587.33, 659.25, 587.33, 659.25, 493.88, 523.25, 587.33, 659.25};

      for (int i = 0; i < reps; i++) {
        arpOsc(bpm, startTime + addedTime, 2, CBABCEABCECB, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 6), 1, BAGABEBGBAGA, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 9), 1, BAGABEBAGFED, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 12), 1, CBABCEABCECBup, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 15), 1, CBAGAGFEFEDC, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 18), 2, DEBCDEDEBCDE, 0.1);

        arpSpectrogram(bpm, startTime + addedTime, 2, CBABCEABCECB, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 6), 1, BAGABEBGBAGA, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 9), 1, BAGABEBAGFED, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 12), 1, CBABCEABCECBup, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 15), 1, CBAGAGFEFEDC, 0.1);
        arpSpectrogram(bpm, startTime + addedTime + timeElapsed(bpm, 18), 2, DEBCDEDEBCDE, 0.1);

        addedTime += timeElapsed(bpm, 24);
      }
    }

    void ostinato1(int bpm, float startTime, int reps) {
      float addedTime = 0.0;

      vector<float> BC = {987.77, 1046.50, 987.77, 1046.50, 987.77, 1046.50, 987.77, 1046.50, 987.77, 1046.50, 987.77, 1046.50};

      for (int i = 0; i < reps; i++) {
        // arpSquare(bpm, startTime + addedTime, 1, BC, 0.1);
        arpWireBox(bpm, startTime + addedTime, 1, BC, 0.1);

        addedTime += timeElapsed(bpm, 3);
      }
    }
    
    void layer2(int bpm, float startTime, int reps) {
      float addedTime = 0.0;
      
      vector<float> ACEA = {440.00, 523.25, 659.25, 880.00, 440.00, 523.25, 659.25, 880.00, 440.00, 523.25, 659.25, 880.00};
      vector<float> BDEB = {493.88, 587.33, 659.25, 987.77, 493.88, 587.33, 659.25, 987.77, 493.88, 587.33, 659.25, 987.77};
      vector<float> ACEAGBEGACEA = {440.00, 523.25, 659.25, 880.00, 392.00, 493.88, 659.25, 783.99, 440.00, 523.25, 659.25, 880.00};
      vector<float> BDEBACEAGBEG = {493.88, 587.33, 659.25, 987.77, 440.00, 523.25, 659.25, 880.00, 392.00, 493.88, 659.25, 783.99};

      for (int i = 0; i < reps; i++) {
        arpSquare(bpm, startTime + addedTime, 1, ACEA, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 3), 2, BDEB, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 9), 1, ACEAGBEGACEA, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 12), 1, BDEBACEAGBEG, 0.1);

        addedTime += timeElapsed(bpm, 15);
      }
    }

    void layer3(int bpm, float startTime, int reps) {
      float addedTime = 0.0;
      
      vector<float> FCEAE = {87.31, 130.81, 164.81, 220.00, 329.63, 220.00, 164.81, 130.81, 87.31, 130.81, 164.81, 220.00};
      vector<float> GDEBE = {98.00, 146.83, 164.81, 246.94, 329.63, 246.94, 164.81, 146.83, 98.00, 146.83, 164.81, 246.94};
      vector<float> ACECE = {110.00, 130.81, 164.81, 261.63, 329.63, 261.63, 164.81, 130.81, 110.00, 130.81, 164.81, 261.63};

      for (int i = 0; i < reps; i++) {
        arpSquare(bpm, startTime + addedTime, 2, FCEAE, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, GDEBE, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, ACECE, 0.1);
        arpSquare(bpm, startTime + addedTime + timeElapsed(bpm, 18), 2, GDEBE, 0.1);

        addedTime += timeElapsed(bpm, 24);
      }
    }

    void layer4(int bpm, float startTime, int reps) {
      float addedTime = 0.0;
      
      vector<float> EAECF = {329.63*4, 220.00*4, 164.81*4, 130.81*4, 87.31*4, 130.81*4, 164.81*4, 220.00*4, 329.63*4, 220.00*4, 164.81*4, 130.81*4};
      vector<float> EBEDG = {329.63*4, 246.94*4, 164.81*4, 146.83*4, 98.00*4, 146.83*4, 164.81*4, 246.94*4, 329.63*4, 246.94*4, 164.81*4, 146.83*4};
      vector<float> ECECA = {329.63*4, 261.63*4, 164.81*4, 130.81*4, 110.00*4, 130.81*4, 164.81*4, 261.63*4, 329.63*4, 261.63*4, 164.81*4, 130.81*4};

      for (int i = 0; i < reps; i++) {
        arpOsc(bpm, startTime + addedTime, 2, EAECF, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, EBEDG, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, ECECA, 0.1);
        arpOsc(bpm, startTime + addedTime + timeElapsed(bpm, 18), 2, EBEDG, 0.1);

        addedTime += timeElapsed(bpm, 24);
      }
    }

    void bass(int bpm, float startTime, int reps) {
      float addedTime = 0.0;

      vector<float> FF = {87.31, 87.31*2};
      vector<float> GG = {98.00, 98.00*2};
      vector<float> AA = {110.00, 110.00*2};

      for (int i = 0; i < reps; i++) {
        chord(bpm, startTime + addedTime, 2, FF, 0.2);
        chord(bpm, startTime + addedTime + timeElapsed(bpm, 6), 2, GG, 0.2);
        chord(bpm, startTime + addedTime + timeElapsed(bpm, 12), 2, AA, 0.2);
        chord(bpm, startTime + addedTime + timeElapsed(bpm, 18), 2, GG, 0.2);
        
        addedTime += timeElapsed(bpm, 24);
      }
    }

    void playComp() {
      int bpm = 120;
      vector<float> E = {329.63*2, 329.63*2, 329.63*2};
      vector<float> downTriplets = {2637.02, 2349.32, 1975.53, 1567.98, 1318.51, 1174.66, 2637.02/2, 2349.32/2, 1975.53/2, 1567.98/2, 1318.51/2, 1174.66/2, 2637.02/4, 2349.32/4, 1975.53/4, 1567.98/4, 1318.51/4, 1174.66/4};
      vector<float> upTriplets = {2637.02/8, 2349.32/8, 1975.53/8, 1567.98/8, 1318.51/8, 1174.66/8, 2637.02/4, 2349.32/4, 1975.53/4, 1567.98/4, 1318.51/4, 1174.66/4, 2637.02/2, 2349.32/2, 1975.53/2, 1567.98/2, 1318.51/2, 1174.66/2};
      vector<float> glitter1 = {2349.32, 2093.00, 1975.53, 1567.98, 1396.91, 1318.51, 2349.32, 2093.00, 1975.53, 1567.98, 1396.91, 1318.51, 2349.32, 2093.00, 1975.53, 1567.98, 1396.91, 1318.51, 2349.32, 2093.00, 1975.53, 1567.98, 1396.91, 1318.51};
      vector<float> glitter2 = {293.66, 329.63, 392.00, 493.88, 523.25, 587.33, 293.66*2, 329.63*2, 392.00*2, 493.88*2, 523.25*2, 587.33*2, 293.66*4, 329.63*4, 392.00*4, 493.88*4, 523.25*4, 587.33*4};

      arpSpectrogram(bpm, 0.0, 10, E, 0.1);
      mainTheme(bpm, timeElapsed(bpm, 6), 2);
      bass(bpm, timeElapsed(bpm, 6), 2);
      layer1(bpm, timeElapsed(bpm, 30), 1);

      // interlude
      ostinato1(bpm, timeElapsed(bpm, 54), 5);
      layer2(bpm, timeElapsed(bpm, 54), 1);

      mainTheme(bpm, timeElapsed(bpm, 69), 2);
      layer1(bpm, timeElapsed(bpm, 69), 2);
      layer3(bpm, timeElapsed(bpm, 69), 2);
      arpSquare(bpm, timeElapsed(bpm, 90), 1, downTriplets, 0.2);
      arpOsc(bpm, timeElapsed(bpm, 105), 1, downTriplets, 0.2);
      arpOsc(bpm, timeElapsed(bpm, 108), 1, upTriplets, 0.2);
      arpWireBox(bpm, timeElapsed(bpm, 105), 1, downTriplets, 0.2);
      arpWireBox(bpm, timeElapsed(bpm, 108), 1, upTriplets, 0.2);
      arpWireBox(bpm, timeElapsed(bpm, 111), 1, glitter1, 0.2);
      arpWireBox(bpm, timeElapsed(bpm, 114), 1, glitter2, 0.2);
      arpOsc(bpm, timeElapsed(bpm, 111), 1, glitter1, 0.2);
      arpSquare(bpm, timeElapsed(bpm, 114), 1, glitter2, 0.2);

      layer4(bpm, timeElapsed(bpm, 93), 1);
      bass(bpm, timeElapsed(bpm, 93), 1);

      // interlude(bpm, timeElapsed(bpm, 48));
    }
  
};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}