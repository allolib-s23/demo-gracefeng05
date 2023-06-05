// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 09. Plucked String synthesis-Visual (Mesh & Spectrum)
// Press '[' or ']' to turn on & off GUI
// Able to play with MIDI device
// Myungin Lee
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

// #include <json/json.h>
// #include "json.hpp"
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

class Marimba2 : public SynthVoice
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
    mAmpEnv.levels(0.7, 0.5, 0.1, 0.0, 0);
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
    g.scale(5 * frequency/10000, 5 * frequency/10000, 1);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();

    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -16);
    g.scale(3 * frequency/10000, 3 * frequency/10000, 0.4);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();

    addSurfaceLoop(mMesh, 4, 4, 2);
    g.pushMatrix();
    g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -24);
    g.scale(3 * frequency/10000, 3 * frequency/10000, 0.4);
    g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
    g.draw(mMesh);
    g.popMatrix();
  
    if (frequency > 800) {
        addTetrahedron(mMesh); // high notes have jagged edges
        g.pushMatrix();
        g.translate(-1 * sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -2); // outer edges
        g.scale(frequency/10000, 0.5 * frequency/10000, 0.4);
        g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
        g.draw(mMesh);
        g.popMatrix();

        g.pushMatrix();
        g.translate(sin(static_cast<double>(frequency)), tan(static_cast<double>(frequency)), -3); // outer edges
        g.scale(0.5 * frequency/10000, frequency/10000, 0.4);
        g.color(HSV(frequency / 1000, 0.5 + mAmpEnv() * 0.1, 0.3 + 0.5 * mAmpEnv()));
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

class Violin : public SynthVoice
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
    gam::Saw<> mSaw1;
    gam::Saw<> mSaw2;
    gam::Env<5> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;

    Mesh mMesh;

    // Initialize voice. This function will only be called once per voice when
    // it is created. Voices will be reused if they are idle.
    void init() override
    {
        // Intialize envelope
        mAmpEnv.curve(0); // make segments lines
        mAmpEnv.levels(0, 0.3, 0.5, 0.7, 1.0);
        mAmpEnv.sustainPoint(4); // Make point 2 sustain until a release is issued

        addCube(mMesh, 1, 1);
        // addSurfaceLoop(mMesh, 2, 3, 2);
        // addIcosahedron(mMesh);
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

        createInternalTriggerParameter("deltaA", 1, -1, 1);
        createInternalTriggerParameter("deltaB", -1, -1, 1);
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

        double x = ((double) rand() / (RAND_MAX));
        double y = ((double) rand() / (RAND_MAX));
        double z = ((double) rand() / (RAND_MAX));
        g.rotate(a, Vec3f(x, y, z));
        g.rotate(b, Vec3f(z, x, y));
        // g.translate(sin(static_cast<double>(frequency)), cos(static_cast<double>(frequency)), -8);
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

class MyApp : public App, public MIDIMessageHandler
{
public:
    SynthGUIManager<Marimba> synthManager{"bloop"};
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

    void playMarimba(float freq, float time, float duration, float amp = .1, float attack = 0.1, float decay = 0.2)
    {
        auto *voice = synthManager.synth().getVoice<Marimba>();
        // amp, freq, attack, release, pan
        vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
        voice->setTriggerParams(params);
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playMarimba2(float freq, float time, float duration, float amp = .1, float attack = 0.1, float decay = 0.2)
    {
        auto *voice = synthManager.synth().getVoice<Marimba2>();
        // amp, freq, attack, release, pan
        vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
        voice->setTriggerParams(params);
        synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
    }

    void playViolin(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1){
    auto* voice = synthManager.synth().getVoice<Violin>();

    voice->setInternalParameterValue("frequency", freq);
    voice->setInternalParameterValue("amplitude", amp);
    voice->setInternalParameterValue("attackTime", attack);
    voice->setInternalParameterValue("releaseTime", decay);
    voice->setInternalParameterValue("pan", 0.0f);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

    void playQuintet() {
        // read json file
        std::ifstream f("/Users/gracefeng/allolib/demo1-gracefeng05/tutorials/synthesis/organon.json");
        // cout << "FILE" << endl;
        // cout << f.rdbuf();
        json music = json::parse(f);

        json marimba1 = music["tracks"][0]["notes"];
        json marimba2 = music["tracks"][1]["notes"];
        json marimba3 = music["tracks"][2]["notes"];
        json cello = music["tracks"][3]["notes"];

        auto m1 = marimba1.begin();
        auto m2 = marimba2.begin();
        auto m3 = marimba3.begin();
        auto c = cello.begin();


    while(m1 != marimba1.end() || m2 != marimba2.end() || m3 != marimba3.end() || c != cello.end())
    {
        if(m1 != marimba1.end())
        {
            auto note = *m1;
            playMarimba(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++m1;
        }
        if(m2 != marimba2.end())
        {
            auto note = *m2;
            playMarimba2(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++m2;
        }
        if(m3 != marimba3.end())
        {
            auto note = *m3;
            playMarimba(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++m3;
        }
        if(c != cello.end())
        {
            auto note = *c;
            playViolin(freq_of(note["midi"]), note["time"], note["duration"], note["velocity"]);
            ++c;
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
        if (k.key('q'))
        {
            playQuintet();
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