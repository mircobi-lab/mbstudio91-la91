#pragma once
/*  LA-91 — motore di sintesi "Linear Arithmetic" scritto per MBStudio91.
    Ogni voce ha due tone (UPPER, LOWER); ogni tone ha due partial (SYNTH: sega/quadra con PW e filtro
    risonante TVF; PCM: forme d'onda della PcmBank) combinati secondo la STRUCTURE (con ring modulator),
    inviluppi a 5 tempi per TVF e TVA, inviluppo di pitch, 3 LFO, EQ e chorus per tone, riverbero/delay finale. */
#include "D50Params.h"
#include "PcmBank.h"

namespace la91
{
// valori grezzi correnti (letti dall'APVTS una volta per blocco), indicizzati per byte del blocco
struct PatchState
{
    int part[4][64] {};   // 0 U1, 1 U2, 2 L1, 3 L2
    int com[2][64] {};    // 0 UPPER, 1 LOWER
    int pat[64] {};
};

class ParamReader
{
public:
    void attach (juce::AudioProcessorValueTreeState&);
    void read (PatchState&) const;
private:
    struct Link { std::atomic<float>* src; int which; int idx; int off; };   // which: 0 part, 1 com, 2 pat
    std::vector<Link> links;
};

// inviluppo a segmenti lineari: start -> tgt[0..n-1] (attacco), mantiene l'ultimo, al rilascio -> relTgt
struct SegEnv
{
    float start = 0, relTgt = 0, relDur = .1f;
    std::array<float, 4> tgt {}, dur {};
    int nAttack = 4;
    int stage = 0;          // 0..nAttack-1 attacco, nAttack = sustain, nAttack+1 = rilascio, nAttack+2 = finito
    float v = 0, from = 0, t = 0;
    void trigger() { stage = 0; v = from = start; t = 0; }
    void release() { if (stage <= nAttack) { stage = nAttack + 1; from = v; t = 0; } }
    bool finished() const { return stage >= nAttack + 2; }
    void tick (float dt);
};

struct Lfo
{
    float phase = 0, value = 0, rnd = 0, age = 0;
    uint32_t seed = 1;
    float tick (int wave, float hz, float delaySec, float dt);   // restituisce -1..1 con dissolvenza iniziale
};

class Synth;

struct PartialVoice
{
    bool pcm = false, on = true;
    double phase = 0, pcmPos = 0;
    float ic1 = 0, ic2 = 0;                 // stato del filtro SVF
    float a1 = 0, a2 = 0, a3 = 0, k = 2;    // coefficienti
    float gain = 0, gainTarget = 0;         // TVA (interpolato nel blocco di controllo)
    double freq = 440;
    float duty = .5f, pwMix = 0;
    bool filterOn = true;
    SegEnv tvf, tva;
    float seqTime = 0;
};

struct ToneVoice
{
    bool active = false, released = false;
    int tone = 0;                // 0 upper, 1 lower
    PartialVoice p[2];
    SegEnv penv;
    Lfo lfo[3];
    float lfoVal[3] {};
    double glideNote = 60, targetNote = 60;
    float glideRate = 0;         // semitoni al secondo (0 = immediato)
};

struct Voice
{
    bool active = false, keyDown = false, dying = false;
    int note = 60;
    float velocity = 1;
    uint64_t age = 0;
    float killGain = 1;
    ToneVoice tv[2];
};

class ChorusFx
{
public:
    void prepare (double sr);
    void process (const float* in, float* outL, float* outR, int n, int type, int rate, int depth, int balance);
private:
    double sr = 48000;
    std::vector<float> buf;
    int w = 0;
    float ph = 0, fbState = 0;
};

class ReverbFx
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* l, float* r, const float* sendL, const float* sendR, int n, int type, int balance);
private:
    double sr = 48000;
    juce::Reverb verb;
    int lastType = -1;
    std::vector<float> dl, dr, tmpL, tmpR;
    int w = 0;
    float gateEnv = 0, gateGain = 0, gateHold = 0;
};

class Synth
{
public:
    Synth();
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>&, const juce::MidiBuffer&, const PatchState&);
    float getOutputLevel() const { return outLevel.load(); }
    int activeVoices() const { return numActive.load(); }

    static constexpr int maxPoly = 16, numSlots = 24, ctl = 16;

private:
    juce::SharedResourcePointer<PcmBank> bank;
    double sr = 48000;
    std::array<Voice, numSlots> voices;
    uint64_t counter = 0;
    float modWheel = 0, pressure = 0, bend = 0;
    bool sustain = false;
    int lastNote[2] = { -1, -1 };
    float globalLfoPhase[2][3] {};
    std::vector<float> bus[2], chL[2], chR[2], mixL, mixR, sendL, sendR;
    juce::dsp::IIR::Filter<float> eqLow[2], eqHigh[2];
    int eqKey[2] = { -1, -1 };
    ChorusFx chorus[2];
    ReverbFx reverb;
    std::atomic<float> outLevel { 0 };
    std::atomic<int> numActive { 0 };
    juce::SmoothedValue<float> volume;

    void noteOn (int note, float vel, const PatchState&);
    void noteOff (int note, const PatchState&);
    void startTone (Voice&, ToneVoice&, int tone, const PatchState&);
    void controlTick (Voice&, ToneVoice&, const PatchState&);
    void renderTone (Voice&, ToneVoice&, float* out, int n, const PatchState&);
    void updateEq (int tone, const PatchState&);
    bool toneSounds (int tone, int note, const PatchState&) const;
    bool holdCovers (int tone, const PatchState&) const;
    bool soloFor (int tone, const PatchState&) const;
};
}
