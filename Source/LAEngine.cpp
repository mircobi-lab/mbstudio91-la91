#include "LAEngine.h"

namespace la91
{
namespace
{
constexpr float kfTable[17] = { -1.f, -.5f, -.25f, 0.f, .125f, .25f, .375f, .5f, .625f, .75f, .875f, 1.f, 1.25f, 1.5f, 2.f, 1.f, 1.f };

inline float envSec (int v)  { return v <= 0 ? .0008f : .002f * std::pow (10.f, (float) v * .04f); }   // 0..100 -> ~0..20 s
inline float lfoHz (int v)   { return .06f * std::pow (2.f, (float) v * .088f); }                       // 0..100 -> 0.06..27 Hz
inline float dbToGain (float db) { return std::pow (10.f, db * .05f); }
inline float noteHz (double semis) { return (float) (440.0 * std::pow (2.0, (semis - 69.0) / 12.0)); }

// LFO select grezzo 0..5 -> (indice 0..2, segno)
inline void lfoSel (int raw, int& idx, float& sign) { idx = juce::jlimit (0, 2, raw / 2); sign = (raw % 2) ? -1.f : 1.f; }

// key bias: punto (<,> + nota da A1) e livello; restituisce "gradini * ottave" oltre il punto
inline float biasOctaves (int raw, double note)
{
    const double point = 33 + (raw % 64);
    const bool up = raw >= 64;
    return (float) (up ? std::max (0.0, note - point) / 12.0 : std::max (0.0, point - note) / 12.0);
}

inline float blep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
inline float frac1 (float x) { return x - std::floor (x); }
inline float sawBlep (float t, float dt) { return 2.f * t - 1.f - blep (t, dt); }

// fattore dei tempi con il key follow (0..4): note acute = inviluppi più corti
inline float timeKf (int tkf, double note) { return std::pow (2.f, -(float) (note - 60.0) / 12.f * (float) tkf * .25f); }
}

// =================================================================================================== lettura parametri
void ParamReader::attach (juce::AudioProcessorValueTreeState& s)
{
    links.clear();
    const Block parts[] = { Block::U1, Block::U2, Block::L1, Block::L2 };
    for (int i = 0; i < 4; ++i)
        for (auto& d : partialDefs()) links.push_back ({ s.getRawParameterValue (paramID (parts[i], d)), 0, i, d.offset });
    for (int i = 0; i < 2; ++i)
        for (auto& d : commonDefs()) links.push_back ({ s.getRawParameterValue (paramID (i == 0 ? Block::UC : Block::LC, d)), 1, i, d.offset });
    for (auto& d : patchDefs()) links.push_back ({ s.getRawParameterValue (paramID (Block::P, d)), 2, 0, d.offset });
}

void ParamReader::read (PatchState& p) const
{
    for (auto& l : links)
    {
        const int v = (int) std::lround (l.src->load (std::memory_order_relaxed));
        if (l.which == 0) p.part[l.idx][l.off] = v;
        else if (l.which == 1) p.com[l.idx][l.off] = v;
        else p.pat[l.off] = v;
    }
}

// =================================================================================================== inviluppi e LFO
void SegEnv::tick (float dt)
{
    if (finished()) return;
    if (stage < nAttack)
    {
        const float d = dur[(size_t) stage];
        t += dt;
        if (t >= d) { v = from = tgt[(size_t) stage]; t = 0; ++stage; }
        else v = from + (tgt[(size_t) stage] - from) * t / d;
    }
    else if (stage == nAttack) v = tgt[(size_t) (nAttack - 1)];
    else
    {
        t += dt;
        if (t >= relDur) { v = relTgt; ++stage; }
        else v = from + (relTgt - from) * t / relDur;
    }
}

float Lfo::tick (int wave, float hz, float delaySec, float dt)
{
    phase += hz * dt;
    if (phase >= 1.f)
    {
        phase -= std::floor (phase);
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        rnd = (float) (seed & 0xffff) / 32768.f - 1.f;
    }
    age += dt;
    switch (wave)
    {
        case 0:  value = 4.f * std::abs (phase - .5f) - 1.f; break;
        case 1:  value = 1.f - 2.f * phase; break;
        case 2:  value = phase < .5f ? 1.f : -1.f; break;
        default: value = rnd; break;
    }
    const float fade = delaySec > .002f ? std::min (1.f, age / delaySec) : 1.f;
    return value * fade;
}

// =================================================================================================== chorus
void ChorusFx::prepare (double s)
{
    sr = s;
    buf.assign ((size_t) juce::nextPowerOfTwo ((int) (sr * .06) + 4), 0.f);
    w = 0; ph = 0; fbState = 0;
}

void ChorusFx::process (const float* in, float* outL, float* outR, int n, int type, int rate, int depth, int balance)
{
    struct Spec { float baseMs, modMs, fb, trem, stereo; bool delay; };
    static const Spec specs[8] = {
        { 7.f, 3.f, 0.f, 0.f, .25f, true },     // CHORUS 1
        { 12.f, 5.f, .1f, 0.f, .5f, true },     // CHORUS 2
        { 1.5f, 1.2f, .6f, 0.f, .25f, true },   // FLANGER 1
        { 3.f, 2.5f, .75f, 0.f, .5f, true },    // FLANGER 2
        { 9.f, 3.f, .5f, 0.f, .25f, true },     // FB CHORUS
        { 0.f, 0.f, 0.f, 1.f, 0.f, false },     // TREMOLO
        { 7.f, 3.f, 0.f, .6f, .25f, true },     // CHORUS TREM
        { 6.f, 1.f, 0.f, 0.f, .5f, true } };    // DIMENSION
    const auto& s = specs[juce::jlimit (0, 7, type)];
    const float hz = .05f * std::pow (2.f, (float) rate * .075f);
    const float dep = (float) depth / 100.f, wet = (float) balance / 100.f;
    const float inc = hz / (float) sr;
    const int mask = (int) buf.size() - 1;
    const float msToS = (float) sr / 1000.f;
    auto read = [&] (float delaySamples)
    {
        const float rp = (float) w - delaySamples;
        const int i0 = (int) std::floor (rp);
        const float f = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * f;
    };
    for (int i = 0; i < n; ++i)
    {
        const float x = in[i];
        ph += inc; if (ph >= 1.f) ph -= 1.f;
        const float lL = std::sin (juce::MathConstants<float>::twoPi * ph);
        const float lR = std::sin (juce::MathConstants<float>::twoPi * (ph + s.stereo));
        float yl = x, yr = x;
        if (s.delay)
        {
            buf[(size_t) w] = x + fbState * s.fb;
            const float dL = (s.baseMs + s.modMs * dep * (.5f + .5f * lL)) * msToS;
            const float dR = (s.baseMs + s.modMs * dep * (.5f + .5f * lR)) * msToS;
            yl = read (dL); yr = read (dR);
            fbState = juce::jlimit (-2.f, 2.f, yl);
            w = (w + 1) & mask;
        }
        float tl = 1.f, tr = 1.f;
        if (s.trem > 0) { tl = 1.f - s.trem * dep * .5f * (1.f + lL); tr = 1.f - s.trem * dep * .5f * (1.f + (type == 5 ? lL : -lL)); }
        if (type == 5) { outL[i] = x * (1.f - wet + wet * tl); outR[i] = x * (1.f - wet + wet * tr); }
        else
        {
            outL[i] = (x * (1.f - wet * .5f) + yl * wet) * (s.trem > 0 ? tl : 1.f);
            outR[i] = (x * (1.f - wet * .5f) + (type == 7 ? -yr * .6f + yl * .4f : yr) * wet) * (s.trem > 0 ? tr : 1.f);
        }
    }
}

// =================================================================================================== riverbero / delay
void ReverbFx::prepare (double s, int maxBlock)
{
    sr = s;
    verb.setSampleRate (sr);
    dl.assign ((size_t) (sr * 1.2) + 8, 0.f); dr = dl;
    tmpL.assign ((size_t) maxBlock, 0.f); tmpR = tmpL;
    w = 0; lastType = -1; gateEnv = gateGain = gateHold = 0;
}

void ReverbFx::process (float* l, float* r, const float* sendL, const float* sendR, int n, int type, int balance)
{
    struct Spec { float room, damp, width, dlyL, dlyR, fb; bool cross; float gateMs; bool reverse; float revMix, dlyMix; };
    static const Spec specs[32] = {
        { .60f, .40f, 1, 0, 0, 0, false, 0, false, 1, 0 },   { .75f, .40f, 1, 0, 0, 0, false, 0, false, 1, 0 },
        { .86f, .35f, 1, 0, 0, 0, false, 0, false, 1, 0 },   { .90f, .30f, 1, 0, 0, 0, false, 0, false, 1, 0 },
        { .30f, .60f, .6f, 0, 0, 0, false, 0, false, 1, 0 }, { .45f, .05f, .8f, 0, 0, 0, false, 0, false, 1, 0 },
        { .40f, .50f, .8f, 0, 0, 0, false, 0, false, 1, 0 }, { .55f, .50f, .9f, 0, 0, 0, false, 0, false, 1, 0 },
        { .65f, .45f, 1, 0, 0, 0, false, 0, false, 1, 0 },   { .72f, .45f, 1, 0, 0, 0, false, 0, false, 1, 0 },
        { .30f, .50f, 1, 102, 102, .30f, false, 0, false, .2f, 1 },
        { .30f, .50f, 1, 180, 180, .35f, true, 0, false, .2f, 1 },
        { .30f, .50f, 1, 224, 224, .35f, true, 0, false, .2f, 1 },
        { .30f, .50f, 1, 148, 296, .35f, true, 0, false, .2f, 1 },
        { .70f, .30f, 1, 0, 0, 0, false, 200, false, 1, 0 }, { .80f, .30f, 1, 0, 0, 0, false, 480, false, 1, 0 },
        { .80f, .10f, 1, 0, 0, 0, false, 0, false, 1, 0 },   { .95f, .20f, 1, 0, 0, 0, false, 0, false, 1, 0 },
        { .35f, .10f, .7f, 7, 9, .80f, false, 0, false, .5f, .6f },
        { .30f, .50f, 1, 248, 248, .35f, false, 0, false, .2f, 1 },
        { .30f, .50f, 1, 338, 338, .35f, false, 0, false, .2f, 1 },
        { .30f, .50f, 1, 157, 157, .35f, true, 0, false, .2f, 1 },
        { .30f, .50f, 1, 252, 252, .35f, true, 0, false, .2f, 1 },
        { .30f, .50f, 1, 274, 137, .35f, true, 0, false, .2f, 1 },
        { .75f, .30f, 1, 0, 0, 0, false, 300, false, 1, 0 },
        { .80f, .30f, 1, 0, 0, 0, false, 360, true, 1, 0 },  { .85f, .30f, 1, 0, 0, 0, false, 480, true, 1, 0 },
        { .20f, .50f, 1, 60, 62, 0, false, 0, false, .1f, .8f },
        { .20f, .50f, 1, 90, 93, 0, false, 0, false, .1f, .8f },
        { .20f, .50f, 1, 130, 134, 0, false, 0, false, .1f, .8f },
        { .88f, .20f, 1, 90, 120, .30f, true, 0, false, 1, .5f },
        { .93f, .25f, 1, 0, 0, 0, false, 0, false, 1, 0 } };
    type = juce::jlimit (0, 31, type);
    const auto& s = specs[type];
    if (type != lastType)
    {
        juce::Reverb::Parameters p;
        p.roomSize = s.room; p.damping = s.damp; p.width = s.width; p.wetLevel = 1.f; p.dryLevel = 0.f; p.freezeMode = 0;
        verb.setParameters (p);
        lastType = type;
    }
    const float bal = (float) balance / 100.f * 1.2f;
    if (bal <= 0.f) return;
    std::copy (sendL, sendL + n, tmpL.begin());
    std::copy (sendR, sendR + n, tmpR.begin());
    verb.processStereo (tmpL.data(), tmpR.data(), n);

    const int size = (int) dl.size();
    const int dL = juce::jlimit (1, size - 1, (int) (s.dlyL * sr / 1000.0));
    const int dR = juce::jlimit (1, size - 1, (int) (s.dlyR * sr / 1000.0));
    const float relCoef = std::exp (-1.f / (.05f * (float) sr));
    const float gateSamples = s.gateMs * (float) sr / 1000.f;
    for (int i = 0; i < n; ++i)
    {
        float wl = tmpL[(size_t) i] * s.revMix * .35f, wr = tmpR[(size_t) i] * s.revMix * .35f;
        if (s.dlyMix > 0)
        {
            const float yl = dl[(size_t) ((w - dL + size) % size)], yr = dr[(size_t) ((w - dR + size) % size)];
            if (s.cross) { dl[(size_t) w] = sendL[i] + yr * s.fb; dr[(size_t) w] = sendR[i] * .3f + yl * s.fb; }
            else         { dl[(size_t) w] = sendL[i] + yl * s.fb; dr[(size_t) w] = sendR[i] + yr * s.fb; }
            w = (w + 1) % size;
            wl += yl * s.dlyMix; wr += yr * s.dlyMix;
        }
        if (gateSamples > 0)
        {
            const float a = std::abs (sendL[i]) + std::abs (sendR[i]);
            gateEnv = std::max (a, gateEnv * relCoef);
            if (gateEnv > .01f) gateHold = std::min (gateHold + 1.f, gateSamples);
            else gateHold = std::max (0.f, gateHold - 1.f);
            float target = gateHold > 0 ? 1.f : 0.f;
            if (s.reverse) target *= gateHold / gateSamples;
            gateGain += (target - gateGain) * .002f;
            wl *= gateGain; wr *= gateGain;
        }
        l[i] += wl * bal; r[i] += wr * bal;
    }
}

// =================================================================================================== synth
Synth::Synth() {}

void Synth::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    for (int t = 0; t < 2; ++t)
    {
        bus[t].assign ((size_t) maxBlock, 0.f); chL[t] = bus[t]; chR[t] = bus[t];
        chorus[t].prepare (sr);
        eqKey[t] = -1;
        eqLow[t].reset(); eqHigh[t].reset();
    }
    mixL.assign ((size_t) maxBlock, 0.f); mixR = mixL; sendL = mixL; sendR = mixL;
    reverb.prepare (sr, maxBlock);
    volume.reset (sr, .05);
    reset();
}

void Synth::reset()
{
    for (auto& v : voices) v.active = false;
    sustain = false; modWheel = pressure = bend = 0;
    lastNote[0] = lastNote[1] = -1;
}

bool Synth::toneSounds (int tone, int note, const PatchState& ps) const
{
    const int km = ps.pat[18], split = 36 + ps.pat[19];
    switch (km)
    {
        case 0: case 4: return tone == 0;
        case 1: case 5: return true;
        default:        return tone == 0 ? note >= split : note < split;
    }
}

bool Synth::soloFor (int tone, const PatchState& ps) const
{
    switch (ps.pat[18])
    {
        case 4: return tone == 0;
        case 5: case 8: return true;
        case 6: return tone == 0;
        case 7: return tone == 1;
        default: return false;
    }
}

bool Synth::holdCovers (int tone, const PatchState& ps) const
{
    const int hm = ps.pat[21];
    return hm == 2 || (hm == 0 && tone == 0) || (hm == 1 && tone == 1);
}

void Synth::startTone (Voice& v, ToneVoice& tv, int tone, const PatchState& ps)
{
    const int* C = ps.com[tone];
    const int st = C[10];
    const bool pcm1 = st == 2 || st == 3 || st == 5 || st == 6;
    const bool pcm2 = st == 4 || st == 5 || st == 6;
    tv.active = true; tv.released = false; tv.tone = tone;
    const double note = v.note;

    for (int i = 0; i < 2; ++i)
    {
        const int* P = ps.part[tone * 2 + i];
        auto& p = tv.p[i];
        p.pcm = i == 0 ? pcm1 : pcm2;
        p.on = (C[46] >> i) & 1;
        p.phase = 0; p.pcmPos = 0; p.seqTime = 0; p.ic1 = p.ic2 = 0; p.gain = 0;
        const float tkfF = timeKf (P[21], note);
        p.tvf.start = 0; p.tvf.nAttack = 4;
        for (int s = 0; s < 4; ++s) { p.tvf.tgt[(size_t) s] = (float) P[27 + s]; p.tvf.dur[(size_t) s] = envSec (P[22 + s]) * tkfF; }
        p.tvf.relTgt = (float) P[31]; p.tvf.relDur = envSec (P[26]) * tkfF;
        p.tvf.trigger();
        const float tkaF = timeKf (P[50], note);
        const float velT1 = std::pow (2.f, -(v.velocity - .5f) * 2.f * (float) P[49] * .5f);
        p.tva.start = 0; p.tva.nAttack = 4;
        for (int s = 0; s < 4; ++s) { p.tva.tgt[(size_t) s] = (float) P[44 + s]; p.tva.dur[(size_t) s] = envSec (P[39 + s]) * tkaF * (s == 0 ? velT1 : 1.f); }
        p.tva.relTgt = (float) P[48]; p.tva.relDur = envSec (P[43]) * tkaF;
        p.tva.trigger();
    }
    const float ptk = timeKf (C[12], note);
    tv.penv.start = (float) (C[17] - 50); tv.penv.nAttack = 3;
    for (int s = 0; s < 3; ++s) { tv.penv.tgt[(size_t) s] = (float) (C[18 + s] - 50); tv.penv.dur[(size_t) s] = envSec (C[13 + s] * 2) * ptk; }
    tv.penv.relTgt = (float) (C[21] - 50); tv.penv.relDur = envSec (C[16] * 2) * ptk;
    tv.penv.trigger();
    for (int k = 0; k < 3; ++k)
    {
        auto& l = tv.lfo[k];
        l.age = 0; l.seed = (uint32_t) (v.age * 2654435761u + (uint32_t) k + 1);
        l.phase = C[28 + 4 * k] == 2 ? 0.f : globalLfoPhase[tone][k];
    }
    // portamento
    const int pm = ps.pat[20];
    const bool porta = ps.pat[44] != 0 && (pm == 2 || (pm == 0 && tone == 0) || (pm == 1 && tone == 1));
    tv.targetNote = note;
    tv.glideNote = (porta && lastNote[tone] >= 0) ? (double) lastNote[tone] : note;
    tv.glideRate = porta ? std::max (.005f, envSec (ps.pat[28]) * .3f) : 0.f;
    controlTick (v, tv, ps);
    for (auto& p : tv.p) p.gain = p.gainTarget * .0f;
}

void Synth::controlTick (Voice& v, ToneVoice& tv, const PatchState& ps)
{
    const float dt = (float) ctl / (float) sr;
    const int tone = tv.tone;
    const int* C = ps.com[tone];
    const int* PT = ps.pat;

    for (int k = 0; k < 3; ++k)
        tv.lfoVal[k] = tv.lfo[k].tick (C[25 + 4 * k], lfoHz (C[26 + 4 * k]), envSec (C[27 + 4 * k]) * .5f, dt);
    tv.penv.tick (dt);
    if (tv.glideRate > 0) tv.glideNote += (tv.targetNote - tv.glideNote) * (1.0 - std::exp (-dt / tv.glideRate));
    else tv.glideNote = tv.targetNote;

    const double note = tv.glideNote;
    const float vel = v.velocity;
    const float keyShift = (float) ((tone == 0 ? PT[22] : PT[23]) - 24);
    const float fineTune = (float) ((tone == 0 ? PT[24] : PT[25]) - 50) / 100.f;
    const float bendSemi = bend * (float) PT[26];
    const float afterBend = (float) (PT[27] - 12) * pressure;
    const float vibDepth = (std::pow ((float) C[22] / 100.f, 2.f) + modWheel * std::pow ((float) C[23] / 100.f, 2.f)
                            + pressure * std::pow ((float) C[24] / 100.f, 2.f)) * 12.f;
    const float vibAL = (modWheel * std::pow ((float) C[23] / 100.f, 2.f) + pressure * std::pow ((float) C[24] / 100.f, 2.f)) * 12.f;
    float penvSemi = tv.penv.v / 50.f * 12.f;
    if (C[11] == 1) penvSemi *= .5f + vel; else if (C[11] == 2) penvSemi *= .25f + 1.5f * vel;
    const int st = C[10];
    const bool ring = st == 1 || st == 3 || st == 4 || st == 6;
    const float pb = (float) C[47];
    const float bal[2] = { std::min (1.f, (100.f - pb) / 50.f), std::min (1.f, pb / 50.f) };
    (void) ring;

    for (int i = 0; i < 2; ++i)
    {
        auto& p = tv.p[i];
        const int* P = ps.part[tone * 2 + i];
        p.tvf.tick (dt); p.tva.tick (dt);

        // ---- WG pitch
        const float kf = kfTable[juce::jlimit (0, 16, P[2])];
        double semis = 60.0 + (note - 60.0) * kf + (P[0] - 36) + (P[1] - 50) / 100.0 + keyShift + fineTune + afterBend;
        if (P[4] == 1) semis += penvSemi; else if (P[4] == 2) semis -= penvSemi;
        if (P[3] == 1) semis += vibDepth * tv.lfoVal[0];
        else if (P[3] == 2) semis -= vibDepth * tv.lfoVal[0];
        else if (P[3] == 3) semis += vibAL * tv.lfoVal[0];
        if (P[5] == 1) semis += bendSemi * kf; else if (P[5] == 2) semis += bendSemi;
        p.freq = juce::jlimit (8.0, sr * .45, (double) noteHz (semis));

        int li; float sg;
        if (! p.pcm)
        {
            // ---- PW / PWM
            lfoSel (P[10], li, sg);
            float pw = (float) P[8] + (float) (P[9] - 7) / 7.f * 50.f * vel + sg * tv.lfoVal[li] * (float) P[11] * .5f
                       + (float) (P[12] - 7) / 7.f * 50.f * pressure;
            pw = juce::jlimit (0.f, 100.f, pw);
            p.duty = .5f - pw / 100.f * .46f;
            p.pwMix = P[6] == 1 ? pw / 100.f : -1.f;   // -1 = quadra

            // ---- TVF
            float depth = (float) P[18] / 100.f * 84.f * (1.f - (float) P[19] / 100.f * (1.f - vel));
            depth *= std::pow (2.f, -(float) (note - 60.0) / 12.f * (float) P[20] * .25f);
            float cn = 24.f + (float) P[13] * 1.05f + (float) (note - 60.0) * kfTable[juce::jlimit (0, 14, P[15])]
                       + (float) (P[17] - 7) * 1.5f * biasOctaves (P[16], note) + depth * p.tvf.v / 100.f;
            lfoSel (P[32], li, sg);
            cn += sg * tv.lfoVal[li] * (float) P[33] / 100.f * 24.f + (float) (P[34] - 7) / 7.f * 24.f * pressure;
            const float fc = juce::jlimit (20.f, (float) sr * .45f, noteHz (cn));
            p.k = 2.f - 1.96f * (float) juce::jlimit (0, 30, P[14]) / 30.f;
            const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            p.a1 = 1.f / (1.f + g * (g + p.k)); p.a2 = g * p.a1; p.a3 = g * p.a2;
            p.filterOn = true;
        }
        else p.filterOn = false;

        // ---- TVA
        float db = P[35] == 0 ? -120.f : (float) (P[35] - 100) * .5f;
        const float vr = (float) (P[36] - 50);
        db += vr > 0 ? (vel - 1.f) * vr * .8f : vel * vr * .8f;
        db += (float) (P[38] - 12) * biasOctaves (P[37], note);
        db += (float) (P[53] - 7) / 7.f * 10.f * pressure;
        const float e = p.tva.v / 100.f;
        float amp = e * e;
        lfoSel (P[51], li, sg);
        amp *= std::max (0.f, 1.f + sg * tv.lfoVal[li] * (float) P[52] / 100.f * .5f);
        p.gainTarget = p.on ? dbToGain (db) * amp * bal[i] * .3f : 0.f;
    }
}

void Synth::renderTone (Voice& v, ToneVoice& tv, float* out, int n, const PatchState& ps)
{
    const int* C = ps.com[tv.tone];
    const int st = C[10];
    const bool ring = st == 1 || st == 3 || st == 4 || st == 6;
    const float invSr = 1.f / (float) sr;
    const float killStep = 1.f / (.004f * (float) sr);
    float s0[ctl], s1[ctl];

    int pos = 0;
    while (pos < n)
    {
        const int len = std::min (ctl, n - pos);
        controlTick (v, tv, ps);
        for (int i = 0; i < 2; ++i)
        {
            auto& p = tv.p[i];
            float* s = i == 0 ? s0 : s1;
            const int* P = ps.part[tv.tone * 2 + i];
            if (! p.pcm)
            {
                const float dt = (float) p.freq * invSr;
                float ph = (float) p.phase;
                for (int j = 0; j < len; ++j)
                {
                    float o;
                    if (p.pwMix < 0)
                    {
                        const float d = p.duty;
                        o = (ph < d ? 1.f : -1.f) + blep (ph, dt) - blep (frac1 (ph + 1.f - d), dt) - (2.f * d - 1.f);
                    }
                    else
                    {
                        const float saw = sawBlep (ph, dt);
                        const float off = .5f + .45f * p.pwMix;
                        o = saw * (1.f - p.pwMix) + (saw - sawBlep (frac1 (ph + off), dt)) * p.pwMix * .7f;
                    }
                    ph += dt; if (ph >= 1.f) ph -= 1.f;
                    // TVF: SVF passa-basso a 2 poli (TPT)
                    const float v3 = o - p.ic2;
                    const float v1 = p.a1 * p.ic1 + p.a2 * v3;
                    const float v2 = p.ic2 + p.a2 * p.ic1 + p.a3 * v3;
                    p.ic1 = 2.f * v1 - p.ic1; p.ic2 = 2.f * v2 - p.ic2;
                    s[j] = v2;
                }
                p.phase = ph;
                if (! std::isfinite (p.ic1) || ! std::isfinite (p.ic2)) { p.ic1 = p.ic2 = 0.f; std::fill (s, s + len, 0.f); }
            }
            else
            {
                const int wi = juce::jlimit (0, PcmBank::numWaves - 1, P[7]);
                const auto& w = bank->wave (wi);
                if (w.kind == PcmBank::Kind::oneShot)
                {
                    const double rate = p.freq / PcmBank::rootHz * PcmBank::oneShotRate / sr;
                    const int size = (int) w.sample.size();
                    for (int j = 0; j < len; ++j)
                    {
                        const int i0 = (int) p.pcmPos;
                        if (i0 + 1 < size)
                        {
                            const float f = (float) (p.pcmPos - i0);
                            s[j] = (w.sample[(size_t) i0] + (w.sample[(size_t) i0 + 1] - w.sample[(size_t) i0]) * f) * w.gain;
                            p.pcmPos += rate;
                        }
                        else s[j] = 0.f;
                    }
                }
                else if (w.kind == PcmBank::Kind::loop)
                {
                    const double inc = p.freq / sr;
                    for (int j = 0; j < len; ++j) { s[j] = bank->readLoop (wi, p.phase, p.freq, sr) * w.gain; p.phase += inc; if (p.phase >= 1) p.phase -= 1; }
                }
                else
                {
                    const double inc = p.freq / sr;
                    const int cnt = (int) w.seq.size();
                    for (int j = 0; j < len; ++j)
                    {
                        const float x = p.seqTime / w.seqPeriod;
                        const int seg = (int) x;
                        const float loc = x - (float) seg, xf = loc > .7f ? (loc - .7f) / .3f : 0.f;
                        const int a = w.seq[(size_t) (seg % cnt)], b = w.seq[(size_t) ((seg + 1) % cnt)];
                        float o = bank->readLoop (a, p.phase, p.freq, sr) * (1.f - xf);
                        if (xf > 0) o += bank->readLoop (b, p.phase, p.freq, sr) * xf;
                        s[j] = o;
                        p.phase += inc; if (p.phase >= 1) p.phase -= 1;
                        p.seqTime += invSr;
                    }
                }
            }
        }
        // TVA, ring modulator e somma
        auto& p0 = tv.p[0]; auto& p1 = tv.p[1];
        const float st0 = (p0.gainTarget - p0.gain) / (float) len, st1 = (p1.gainTarget - p1.gain) / (float) len;
        for (int j = 0; j < len; ++j)
        {
            p0.gain += st0; p1.gain += st1;
            const float a = s0[j] * p0.gain;
            const float b = ring ? s0[j] * s1[j] * 1.6f * p1.gain : s1[j] * p1.gain;
            float k = 1.f;
            if (v.dying) { v.killGain = std::max (0.f, v.killGain - killStep); k = v.killGain; }
            out[pos + j] += (a + b) * k;
        }
        pos += len;
    }
    // fine del tone: TVA finiti (o partial spenti), con coda forzata se END L > 0
    bool done = true;
    for (auto& p : tv.p)
    {
        if (! p.on) continue;
        if (! p.tva.finished()) { done = false; continue; }
        if (p.tva.v > .5f) { p.tva.v *= std::exp (-(float) n / (.3f * (float) sr)); done = false; }
    }
    if (done || (v.dying && v.killGain <= 0.f)) tv.active = false;
}

void Synth::noteOn (int note, float vel, const PatchState& ps)
{
    bool sounds[2] = { toneSounds (0, note, ps), toneSounds (1, note, ps) };
    if (! sounds[0] && ! sounds[1]) return;
    // modalità solo: chiude le voci che usano lo stesso tone
    for (int t = 0; t < 2; ++t)
        if (sounds[t] && soloFor (t, ps))
            for (auto& v : voices) if (v.active && ! v.dying && v.tv[t].active) { v.dying = true; }
    int busy = 0;
    for (auto& v : voices) if (v.active && ! v.dying) ++busy;
    if (busy >= maxPoly)
    {
        Voice* steal = nullptr;
        for (auto& v : voices)
            if (v.active && ! v.dying && (steal == nullptr || (! v.keyDown && steal->keyDown) || (v.keyDown == steal->keyDown && v.age < steal->age)))
                steal = &v;
        if (steal) steal->dying = true;
    }
    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (! slot) for (auto& v : voices) if (v.dying && (slot == nullptr || v.killGain < slot->killGain)) slot = &v;
    if (! slot) return;
    auto& v = *slot;
    v = Voice();
    v.active = true; v.keyDown = true; v.note = note; v.velocity = vel; v.age = ++counter;
    for (int t = 0; t < 2; ++t)
    {
        v.tv[t].active = false;
        if (sounds[t]) { startTone (v, v.tv[t], t, ps); lastNote[t] = note; }
    }
}

void Synth::noteOff (int note, const PatchState& ps)
{
    for (auto& v : voices)
    {
        if (! v.active || ! v.keyDown || v.note != note || v.dying) continue;
        v.keyDown = false;
        for (int t = 0; t < 2; ++t)
        {
            auto& tv = v.tv[t];
            if (! tv.active || tv.released) continue;
            if (sustain && holdCovers (t, ps)) continue;
            tv.released = true;
            for (auto& p : tv.p) { p.tvf.release(); p.tva.release(); }
            tv.penv.release();
        }
    }
}

void Synth::updateEq (int tone, const PatchState& ps)
{
    const int* C = ps.com[tone];
    const int key = C[37] | (C[38] << 5) | (C[39] << 10) | (C[40] << 15) | (C[41] << 20);
    if (key == eqKey[tone]) return;
    eqKey[tone] = key;
    static const float lowF[16] = { 63, 75, 88, 105, 125, 150, 175, 210, 250, 300, 350, 420, 500, 600, 700, 840 };
    static const float highF[22] = { 250, 300, 350, 420, 500, 600, 700, 840, 1000, 1200, 1400, 1700, 2000, 2400, 2800, 3400, 4000, 4800, 5700, 6700, 8000, 9500 };
    static const float qs[9] = { .3f, .5f, .7f, 1.f, 1.4f, 2.f, 3.f, 4.2f, 6.f };
    eqLow[tone].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, lowF[juce::jlimit (0, 15, C[37])], .707f,
                                                                                   dbToGain ((float) (C[38] - 12)));
    eqHigh[tone].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, std::min ((float) sr * .45f, highF[juce::jlimit (0, 21, C[39])]),
                                                                                      qs[juce::jlimit (0, 8, C[40])], dbToGain ((float) (C[41] - 12)));
}

void Synth::process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const PatchState& ps)
{
    const int n = out.getNumSamples();
    if (n > (int) mixL.size()) prepare (sr, n);
    for (int t = 0; t < 2; ++t) std::fill (bus[t].begin(), bus[t].begin() + n, 0.f);

    auto renderRange = [&] (int start, int end)
    {
        if (end <= start) return;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int t = 0; t < 2; ++t)
                if (v.tv[t].active) renderTone (v, v.tv[t], bus[t].data() + start, end - start, ps);
            if (! v.tv[0].active && ! v.tv[1].active) v.active = false;
        }
    };

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int at = juce::jlimit (0, n, meta.samplePosition);
        renderRange (pos, at); pos = at;
        if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), ps);
        else if (m.isNoteOff()) noteOff (m.getNoteNumber(), ps);
        else if (m.isPitchWheel()) bend = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
        else if (m.isChannelPressure()) pressure = (float) m.getChannelPressureValue() / 127.f;
        else if (m.isAftertouch()) pressure = (float) m.getAfterTouchValue() / 127.f;
        else if (m.isController())
        {
            const int cc = m.getControllerNumber();
            if (cc == 1) modWheel = (float) m.getControllerValue() / 127.f;
            else if (cc == 64)
            {
                const bool down = m.getControllerValue() >= 64;
                if (sustain && ! down)
                {
                    sustain = false;
                    for (auto& v : voices)
                        if (v.active && ! v.keyDown)
                            for (auto& tv : v.tv)
                                if (tv.active && ! tv.released)
                                {
                                    tv.released = true;
                                    for (auto& p : tv.p) { p.tvf.release(); p.tva.release(); }
                                    tv.penv.release();
                                }
                }
                sustain = down;
            }
            else if (cc == 120 || cc == 123) for (auto& v : voices) if (v.active) v.dying = true;
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) for (auto& v : voices) if (v.active) v.dying = true;
    }
    renderRange (pos, n);

    // LFO "liberi" condivisi (per sync OFF/ON)
    for (int t = 0; t < 2; ++t)
        for (int k = 0; k < 3; ++k)
        {
            float& ph = globalLfoPhase[t][k];
            ph += lfoHz (ps.com[t][26 + 4 * k]) * (float) n / (float) sr;
            ph -= std::floor (ph);
        }

    // EQ e chorus per tone
    for (int t = 0; t < 2; ++t)
    {
        updateEq (t, ps);
        float* b = bus[t].data();
        for (int i = 0; i < n; ++i) b[i] = eqHigh[t].processSample (eqLow[t].processSample (b[i]));
        const int* C = ps.com[t];
        chorus[t].process (b, chL[t].data(), chR[t].data(), n, C[42], C[43], C[44], C[45]);
    }
    const float tb = (float) ps.pat[33];
    const float gU = std::min (1.f, tb / 50.f), gL = std::min (1.f, (100.f - tb) / 50.f);
    const int om = ps.pat[29];
    const float sU = om == 3 ? 0.f : 1.f, sL = om == 2 ? 0.f : 1.f;
    for (int i = 0; i < n; ++i)
    {
        const float uL = chL[0][(size_t) i] * gU, uR = chR[0][(size_t) i] * gU, lL = chL[1][(size_t) i] * gL, lR = chR[1][(size_t) i] * gL;
        mixL[(size_t) i] = uL + lL; mixR[(size_t) i] = uR + lR;
        sendL[(size_t) i] = uL * sU + lL * sL; sendR[(size_t) i] = uR * sU + lR * sL;
    }
    reverb.process (mixL.data(), mixR.data(), sendL.data(), sendR.data(), n, ps.pat[30], ps.pat[31]);

    volume.setTargetValue (std::pow ((float) ps.pat[32] / 100.f, 1.5f));
    float peak = 0;
    const int chans = out.getNumChannels();
    auto* L = out.getWritePointer (0);
    auto* R = chans > 1 ? out.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float g = volume.getNextValue();
        const float l = std::tanh (mixL[(size_t) i] * g), r = std::tanh (mixR[(size_t) i] * g);
        if (R) { L[i] = l; R[i] = r; } else L[i] = .5f * (l + r);
        peak = std::max (peak, std::max (std::abs (l), std::abs (r)));
    }
    for (int c = 2; c < chans; ++c) out.clear (c, 0, n);
    outLevel.store (std::max (peak, outLevel.load() * .9f));
    int act = 0;
    for (auto& v : voices) if (v.active) ++act;
    numActive.store (act);
}
}
