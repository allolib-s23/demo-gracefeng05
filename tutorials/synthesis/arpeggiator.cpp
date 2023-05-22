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

using namespace gam;
using namespace al;
using namespace std;

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
          // playArps();
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
        playSquare(frequency, startTime + i * totalDuration / numNotes, noteDuration);
    }
    }


  void chord(int bpm, float startTime, int measures, vector<float> notes, float amp) {
    float chordDuration = ((60.0/bpm) * 3) * measures;
    for (int i = 0; i < notes.size(); i++) {
      playWireBox(notes[i], startTime, chordDuration, amp, 0.0, 0.001);
      playSine(notes[i], startTime, chordDuration, amp, 0.0, 0.001);
    }
  }

  /* void playArps() {
    vector<float> 
    upDownArp(100, 0.0, 4, )
  }
  */
  
};

int main() {
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}