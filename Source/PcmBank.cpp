#include "PcmBank.h"

namespace la91
{
namespace
{
struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed) : s (seed ? seed : 0x9e3779b9u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() & 0xffffff) / 16777216.f; }      // 0..1
    float bip() { return uni() * 2.f - 1.f; }                             // -1..1
};

constexpr double SR = PcmBank::oneShotRate;
constexpr double F0 = PcmBank::rootHz;
constexpr double twoPi = juce::MathConstants<double>::twoPi;

// biquad passa-banda (RBJ, guadagno di picco 0 dB)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void bandpass (double f, double q)
    {
        const double w = twoPi * juce::jlimit (20.0, SR * .45, f) / SR, al = std::sin (w) / (2 * q), a0 = 1 + al;
        b0 = al / a0; b1 = 0; b2 = -al / a0; a1 = -2 * std::cos (w) / a0; a2 = (1 - al) / a0;
    }
    void lowpass (double f, double q)
    {
        const double w = twoPi * juce::jlimit (20.0, SR * .45, f) / SR, al = std::sin (w) / (2 * q), c = std::cos (w), a0 = 1 + al;
        b0 = (1 - c) * .5 / a0; b1 = (1 - c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    void highpass (double f, double q)
    {
        const double w = twoPi * juce::jlimit (20.0, SR * .45, f) / SR, al = std::sin (w) / (2 * q), c = std::cos (w), a0 = 1 + al;
        b0 = (1 + c) * .5 / a0; b1 = -(1 + c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y;
        return (float) y;
    }
};

std::vector<float> buffer (double seconds) { return std::vector<float> ((size_t) (seconds * SR), 0.f); }

void normalise (std::vector<float>& v, float peak = .9f)
{
    float m = 1e-9f;
    for (auto x : v) m = std::max (m, std::abs (x));
    for (auto& x : v) x *= peak / m;
}

// dissolvenza finale per evitare clic alla fine del one-shot
void fadeTail (std::vector<float>& v, double seconds = .04)
{
    const int n = std::min ((int) v.size(), (int) (seconds * SR));
    for (int i = 0; i < n; ++i) v[v.size() - 1 - (size_t) i] *= (float) i / (float) n;
}

// modi smorzati (percussioni) + colpo di rumore filtrato
std::vector<float> modal (std::initializer_list<double> ratios, std::initializer_list<double> amps, std::initializer_list<double> decays,
                          double noise, double noiseDecay, double noiseFreq, double dur, uint32_t seed)
{
    auto v = buffer (dur);
    std::vector<double> r (ratios), a (amps), d (decays);
    Rng rng (seed);
    for (size_t k = 0; k < r.size(); ++k)
    {
        const double f = F0 * r[k];
        if (f > SR * .45) continue;
        const double w = twoPi * f / SR, ph = rng.uni() * twoPi;
        const double dk = std::exp (-1.0 / (std::max (.002, d[std::min (k, d.size() - 1)]) * SR));
        double env = a[std::min (k, a.size() - 1)];
        for (size_t i = 0; i < v.size(); ++i) { v[i] += (float) (env * std::sin (w * (double) i + ph)); env *= dk; }
    }
    if (noise > 0)
    {
        Biquad bp; bp.bandpass (noiseFreq, 1.2);
        const double dk = std::exp (-1.0 / (noiseDecay * SR));
        double env = noise;
        for (size_t i = 0; i < v.size() && env > 1e-5; ++i) { v[i] += (float) env * bp.process (rng.bip()) * 2.f; env *= dk; }
    }
    // attacco di 1 ms per non avere scalini
    for (int i = 0; i < 48 && i < (int) v.size(); ++i) v[(size_t) i] *= (float) i / 48.f;
    normalise (v); fadeTail (v);
    return v;
}

// corda pizzicata (Karplus-Strong)
std::vector<float> pluck (double damping, double bright, double pickPos, double dur, uint32_t seed, double drive = 0, double click = 0)
{
    auto v = buffer (dur);
    Rng rng (seed);
    const double period = SR / F0;
    const int len = (int) period + 2;
    std::vector<double> line ((size_t) len * 2, 0.0);
    // eccitazione: rumore filtrato con pettine della posizione del plettro
    std::vector<double> exc ((size_t) len, 0.0);
    double lp = 0;
    for (int i = 0; i < len; ++i) { lp += (rng.bip() - lp) * (.15 + .85 * bright); exc[(size_t) i] = lp; }
    const int pp = std::max (1, (int) (pickPos * len));
    for (int i = len - 1; i >= pp; --i) exc[(size_t) i] -= exc[(size_t) (i - pp)];
    std::vector<double> ring ((size_t) len, 0.0);
    for (int i = 0; i < len; ++i) ring[(size_t) i] = exc[(size_t) i];
    double frac = period - std::floor (period), prev = 0;
    int idx = 0;
    const int ip = (int) std::floor (period);
    std::vector<double> d ((size_t) ip + 1, 0.0);
    for (int i = 0; i <= ip; ++i) d[(size_t) i] = ring[(size_t) (i % len)];
    int w = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const int r0 = (w + 1) % (ip + 1);
        const double s = d[(size_t) w];
        const double nxt = d[(size_t) r0];
        const double out = s * (1 - frac) + nxt * frac;
        const double filt = damping * (bright * out + (1 - bright) * .5 * (out + prev));
        prev = out;
        d[(size_t) w] = filt;
        w = r0;
        v[i] = (float) out;
        (void) idx;
    }
    if (click > 0)
    {
        Biquad bp; bp.bandpass (2500, .8);
        for (int i = 0; i < (int) (.01 * SR); ++i) v[(size_t) i] += (float) (click * (1 - i / (.01 * SR))) * bp.process (rng.bip()) * 3.f;
    }
    if (drive > 0) for (auto& x : v) x = (float) std::tanh (x * (1 + drive * 8)) ;
    normalise (v); fadeTail (v, .08);
    return v;
}

// tono armonico con attacco/decadimento, rumore d'attacco (soffio, archetto) e formanti
struct Harm
{
    std::function<double (int)> amp;     // ampiezza dell'armonica n
    int nMax = 30;
    double attack = .02, decay = .4, dur = .6;
    double noise = 0, noiseFreq = 2000, noiseQ = 1, noiseDecay = .05;
    double brightRise = 0;               // le armoniche alte entrano più tardi (ottoni)
    double bend = 0;                      // semitoni di partenza (scivolata verso l'intonazione)
    double inharm = 0;                    // inarmonicità tipo pianoforte
    double hfDecay = 0;                   // le alte decadono prima (pianoforte)
};

std::vector<float> harm (const Harm& h, uint32_t seed)
{
    auto v = buffer (h.dur);
    Rng rng (seed);
    const size_t n = v.size(), att = (size_t) std::max (1.0, h.attack * SR);
    for (int k = 1; k <= h.nMax; ++k)
    {
        const double ratio = k * std::sqrt (1 + h.inharm * k * k);
        if (F0 * ratio > SR * .45) break;
        const double a = h.amp (k);
        if (a <= 0) continue;
        double ph = rng.uni() * twoPi;
        const double dk = h.hfDecay > 0 ? std::exp (-1.0 / (std::max (.01, h.decay / (1 + h.hfDecay * (k - 1))) * SR)) : 1.0;
        double env = 1;
        const double delay = h.brightRise * (k - 1) / (double) h.nMax;
        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / SR;
            const double bendSemi = h.bend * std::exp (-t / std::max (1e-3, h.attack));
            ph += twoPi * F0 * ratio * std::pow (2.0, bendSemi / 12.0) / SR;
            double g = 1;
            if (h.brightRise > 0) g = juce::jlimit (0.0, 1.0, (t - delay * .5) / std::max (1e-3, delay + 1e-3));
            v[i] += (float) (a * env * g * std::sin (ph));
            env *= dk;
        }
    }
    // inviluppo globale: attacco lineare, decadimento esponenziale
    const double dk = std::exp (-1.0 / (h.decay * SR));
    double env = 1;
    for (size_t i = 0; i < n; ++i)
    {
        const double a = i < att ? (double) i / (double) att : 1.0;
        if (i >= att && h.hfDecay <= 0) env *= dk;
        v[i] *= (float) (a * env);
    }
    if (h.noise > 0)
    {
        Biquad bp; bp.bandpass (h.noiseFreq, h.noiseQ);
        const double nd = std::exp (-1.0 / (h.noiseDecay * SR));
        double e = h.noise;
        for (size_t i = 0; i < n && e > 1e-5; ++i)
        {
            const double a = i < att ? (double) i / (double) att : 1.0;
            v[i] += (float) (e * a) * bp.process (rng.bip()) * 3.f;
            if (i >= att) e *= nd;
        }
    }
    normalise (v); fadeTail (v);
    return v;
}

// rumore modulato (raspa, guiro, lima)
std::vector<float> scrape (double rate, double centre, double q, double dur, uint32_t seed, double pitched = .2)
{
    auto v = buffer (dur);
    Rng rng (seed);
    Biquad bp; bp.bandpass (centre, q);
    double ph = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const double t = (double) i / SR;
        const double gate = .5 + .5 * std::sin (twoPi * rate * t);
        const double env = std::min (1.0, t / .005) * std::exp (-t / (dur * .45));
        ph += twoPi * F0 / SR;
        v[i] = (float) (env * (gate * gate * bp.process (rng.bip()) * 3 + pitched * std::sin (ph)));
    }
    normalise (v); fadeTail (v);
    return v;
}

double formant (double n, std::initializer_list<double> peaks, double width)
{
    double s = .15;
    for (auto p : peaks) s += std::exp (-(n - p) * (n - p) / (2 * width * width));
    return s;
}
}

// =================================================================================================
PcmBank::PcmBank()
{
    waves.resize ((size_t) numWaves);
    makeOneShots();
    makeLoops();
    makeSequences();
}

void PcmBank::makeOneShots()
{
    auto set = [&] (int i, std::vector<float> s, float gain = 1.f) { auto& w = waves[(size_t) i]; w.kind = Kind::oneShot; w.sample = std::move (s); w.gain = gain; };
    uint32_t seed = 1234;
    auto sd = [&] { return seed += 7919; };

    set (0,  modal ({ 1, 3.93, 9.2 }, { 1, .35, .12 }, { .45, .08, .03 }, .2, .004, 3000, .7, sd()));
    set (1,  modal ({ 1, 3.98, 10.1 }, { 1, .3, .1 }, { 1.2, .3, .1 }, .1, .004, 2500, 1.2, sd()));
    set (2,  modal ({ 1, 3.0, 6.2, 9.9 }, { 1, .5, .25, .1 }, { .18, .06, .03, .02 }, .3, .004, 4000, .4, sd()));
    set (3,  modal ({ 1, 2.76, 5.4, 8.9 }, { 1, .6, .3, .15 }, { .12, .05, .03, .02 }, .4, .003, 3500, .35, sd()));
    set (4,  modal ({ 1, 2.01, 3.9 }, { 1, .4, .1 }, { .35, .12, .05 }, .15, .004, 1500, .5, sd()));
    set (5,  modal ({ 1, 2, 3, 4, 5, 6 }, { 1, .5, .33, .25, .2, .17 }, { .25, .15, .1, .08, .06, .05 }, .8, .01, 2000, .4, sd()));
    {   // TAIKO: sinusoide che scende di intonazione + tonfo di rumore
        auto v = buffer (.6); Rng r (sd()); Biquad lp; lp.lowpass (400, .7); double ph = 0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            const double t = (double) i / SR;
            ph += twoPi * F0 * (1 + .6 * std::exp (-t / .03)) / SR;
            v[i] = (float) (std::exp (-t / .18) * std::sin (ph) + .8 * std::exp (-t / .02) * lp.process (r.bip()) * 3);
        }
        normalise (v); fadeTail (v); set (6, v);
    }
    set (7,  modal ({ 1, 7.1, 14.3 }, { 1, .5, .2 }, { 1.5, .12, .05 }, .05, .003, 5000, 1.2, sd()));
    set (8,  pluck (.996, .5, .13, .9, sd()));
    set (9,  modal ({ 1, 2.32, 4.25, 6.63 }, { 1, .8, .6, .4 }, { .06, .05, .04, .03 }, .5, .006, 6000, .25, sd()));
    set (10, modal ({ 1, 2.48, 4.1 }, { 1, .6, .3 }, { .35, .25, .15 }, .2, .004, 4000, .6, sd()));
    set (11, modal ({ 1, 2.76, 5.4, 8.93, 13.3 }, { .6, .8, .7, .5, .3 }, { 1.2, 1.0, .8, .6, .5 }, .1, .003, 8000, 1.2, sd()));
    set (12, modal ({ .5, 1, 1.19, 1.56, 2, 2.51, 2.66, 3.01, 4.1 }, { .5, 1, .7, .5, .6, .4, .35, .3, .2 }, { 2, 1.6, 1.2, 1, .8, .6, .6, .5, .4 }, .05, .003, 6000, 1.5, sd()));
    set (13, scrape (35, 3500, 1.5, .3, sd(), .1));
    {   Harm h; h.amp = [] (int n) { return 1.0 / n; }; h.nMax = 6; h.attack = .001; h.decay = .03; h.dur = .15; h.noise = .9; h.noiseFreq = 2200; h.noiseQ = 1.4; h.noiseDecay = .008;
        set (14, harm (h, sd())); }
    for (int k = 0; k < 3; ++k)
    {   Harm h; const double b = (double) k;
        h.amp = [b] (int n) { return std::pow ((double) n, -(1.1 - .15 * b)) * (.6 + .4 * std::abs (std::sin (juce::MathConstants<double>::pi * n * .12))); };
        h.nMax = 24; h.attack = .002; h.decay = 1.2 - .35 * b; h.hfDecay = .25; h.inharm = .0003 + .0002 * b; h.dur = .9 - .2 * b;
        h.noise = .25; h.noiseFreq = 1500 + 800 * b; h.noiseQ = .8; h.noiseDecay = .01;
        set (15 + k, harm (h, sd())); }
    set (18, pluck (.997, .95, .1, .8, sd()));
    set (19, pluck (.998, .3, .3, 1.2, sd()));
    {   Harm h; h.amp = [] (int n) { return n == 2 ? 1.0 : n == 3 ? .7 : n == 1 ? .3 : 0.0; }; h.nMax = 3; h.attack = .002; h.decay = .2; h.dur = .5;
        set (20, harm (h, sd())); }
    set (21, pluck (.997, .8, .15, 1.0, sd()));
    set (22, pluck (.995, .4, .2, .9, sd()));
    set (23, pluck (.998, .6, .18, 1.0, sd()));
    set (24, pluck (.998, .7, .25, 1.0, sd()));
    set (25, pluck (.998, .7, .2, 1.0, sd(), .6));
    set (26, pluck (.996, .7, .2, .8, sd()));
    set (27, pluck (.995, .9, .1, .7, sd(), 0, .8));
    {   Harm h; h.amp = [] (int n) { return n == 1 ? 1.0 : n == 2 ? .5 : 0.0; }; h.nMax = 2; h.attack = .002; h.decay = .08; h.dur = .3;
        h.noise = .5; h.noiseFreq = 300; h.noiseQ = .8; h.noiseDecay = .02; set (28, harm (h, sd())); }
    set (29, pluck (.994, .25, .2, .9, sd()));
    // fiati
    {   Harm h; h.amp = [] (int n) { return n % 2 ? 1.0 / n : .08 / n; }; h.nMax = 25; h.attack = .015; h.decay = .4; h.dur = .6;
        h.noise = .15; h.noiseFreq = 1800; h.noiseDecay = .06; set (30, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return n == 1 ? .25 : 0.0; }; h.nMax = 1; h.attack = .03; h.decay = .3; h.dur = .6;
        h.noise = 1; h.noiseFreq = F0 * 3; h.noiseQ = 2; h.noiseDecay = .25; set (31, harm (h, sd())); }
    {   Harm h; h.amp = [] (int) { return 0.0; }; h.nMax = 1; h.attack = .02; h.decay = .5; h.dur = .7;
        h.noise = 1; h.noiseFreq = 6000; h.noiseQ = .5; h.noiseDecay = .35; set (32, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return n == 1 ? 1.0 : n == 2 ? .15 : 0.0; }; h.nMax = 2; h.attack = .02; h.decay = .5; h.dur = .6;
        h.noise = .35; h.noiseFreq = F0 * 4; h.noiseQ = 1.5; h.noiseDecay = .04; set (33, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return n == 1 ? 1.0 : n == 3 ? .2 : 0.0; }; h.nMax = 3; h.attack = .03; h.decay = .5; h.dur = .6;
        h.noise = .6; h.noiseFreq = F0 * 3; h.noiseQ = 1; h.noiseDecay = .08; set (34, harm (h, sd())); }
    set (35, scrape (45, 2500, 1.2, .35, sd(), .05));
    {   Harm h; h.amp = [] (int n) { return n == 1 ? 1.0 : n == 2 ? .1 : 0.0; }; h.nMax = 2; h.attack = .04; h.decay = .5; h.dur = .7; h.bend = -1;
        h.noise = .5; h.noiseFreq = 1200; h.noiseQ = .7; h.noiseDecay = .1; set (36, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return n == 2 ? 1.0 : n == 3 ? .6 : 0.0; }; h.nMax = 3; h.attack = .02; h.decay = .4; h.dur = .6;
        h.noise = .4; h.noiseFreq = F0 * 6; h.noiseQ = 1.5; h.noiseDecay = .05; set (37, harm (h, sd())); }
    for (int k = 0; k < 4; ++k)
    {   Harm h; const double e = 1.3 - .17 * k;
        h.amp = [e] (int n) { return std::pow ((double) n, -e); }; h.nMax = 30; h.attack = .025 + .01 * k; h.decay = .5; h.dur = .7;
        h.brightRise = .04 + .02 * k; h.bend = -.4; h.noise = .06; h.noiseFreq = 1500; h.noiseDecay = .03;
        set (38 + k, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return 1.0 / n; }; h.nMax = 30; h.attack = .06; h.decay = .5; h.dur = .7;
        h.noise = .5; h.noiseFreq = 900; h.noiseQ = .8; h.noiseDecay = .08; set (42, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return formant (n, { 3, 8 }, 1.6) / n; }; h.nMax = 40; h.attack = .05; h.decay = .5; h.dur = .7;
        h.noise = .4; h.noiseFreq = 2200; h.noiseQ = 1; h.noiseDecay = .06; set (43, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return .15 / n; }; h.nMax = 20; h.attack = .03; h.decay = .35; h.dur = .5;
        h.noise = 1; h.noiseFreq = 3500; h.noiseQ = 1.2; h.noiseDecay = .1; set (44, harm (h, sd())); }
    {   Harm h; h.amp = [] (int n) { return formant (n, { 5, 12 }, 2.0) / std::pow ((double) n, .8); }; h.nMax = 40; h.attack = .04; h.decay = .5; h.dur = .7;
        h.noise = .35; h.noiseFreq = 3000; h.noiseQ = 1; h.noiseDecay = .05; set (45, harm (h, sd())); }
    set (46, pluck (.99, .5, .2, .4, sd()));
}

void PcmBank::buildTable (Wave& w, const std::vector<float>& amps, uint32_t seed)
{
    w.kind = Kind::loop;
    Rng rng (seed);
    std::vector<float> phases (amps.size());
    for (auto& p : phases) p = rng.uni() * (float) twoPi;
    juce::dsp::FFT fft (11);   // 2048
    float scale = 0;
    for (int lv = 0; lv < numLevels; ++lv)
    {
        const int maxH = std::min ((int) amps.size() - 1, 512 >> lv);
        std::vector<float> data ((size_t) tableSize * 2, 0.f);
        for (int h = 1; h <= maxH; ++h)
        {
            data[(size_t) h * 2]     = amps[(size_t) h] * std::cos (phases[(size_t) h]);
            data[(size_t) h * 2 + 1] = amps[(size_t) h] * std::sin (phases[(size_t) h]);
        }
        fft.performRealOnlyInverseTransform (data.data());
        if (lv == 0)
        {
            float m = 1e-9f;
            for (int i = 0; i < tableSize; ++i) m = std::max (m, std::abs (data[(size_t) i]));
            scale = .9f / m;
        }
        auto& t = w.levels[(size_t) lv];
        t.assign ((size_t) tableSize + 1, 0.f);
        for (int i = 0; i < tableSize; ++i) t[(size_t) i] = data[(size_t) i] * scale;
        t[(size_t) tableSize] = t[0];   // punto di guardia per l'interpolazione
    }
}

void PcmBank::makeLoops()
{
    auto spec = [] (int nMax, const std::function<double (int)>& f)
    {
        std::vector<float> a (513, 0.f);
        for (int n = 1; n <= std::min (512, nMax); ++n) a[(size_t) n] = (float) std::max (0.0, f (n));
        return a;
    };
    auto list = [] (std::initializer_list<std::pair<int, double>> l)
    {
        std::vector<float> a (513, 0.f);
        for (auto& p : l) a[(size_t) p.first] = (float) p.second;
        return a;
    };
    const double pi = juce::MathConstants<double>::pi;
    uint32_t seed = 4242;
    auto put = [&] (int i, std::vector<float> a) { buildTable (waves[(size_t) i], a, seed += 104729); };

    put (47, list ({ { 1, 1 }, { 2, .8 }, { 3, .6 }, { 4, .5 }, { 6, .4 }, { 8, .35 }, { 10, .2 }, { 12, .2 }, { 16, .15 } }));
    put (48, list ({ { 2, .6 }, { 4, .8 }, { 6, .4 }, { 8, .6 }, { 12, .3 }, { 16, .3 } }));
    put (49, list ({ { 1, 1 }, { 2, .3 }, { 3, .5 }, { 5, .2 } }));
    put (50, list ({ { 1, 1 }, { 2, .25 }, { 3, .08 }, { 4, .04 }, { 7, .06 } }));
    put (51, list ({ { 1, 1 }, { 2, .4 }, { 3, .2 }, { 4, .1 }, { 5, .05 }, { 6, .05 }, { 7, .12 }, { 8, .04 } }));
    put (52, spec (60, [&] (int n) { return std::abs (std::sin (pi * n * .12)) / n; }));
    put (53, spec (80, [&] (int n) { return std::abs (std::sin (pi * n * .17)) / std::pow ((double) n, .6); }));
    put (54, list ({ { 1, 1 }, { 2, .5 }, { 3, .3 }, { 4, .15 }, { 5, .08 } }));
    put (55, spec (20, [] (int n) { return (n == 2 ? .3 : 1.0) / std::pow ((double) n, 1.3); }));
    put (56, spec (60, [&] (int n) { return std::abs (std::sin (pi * n * .23)) / n; }));
    put (57, spec (60, [] (int n) { return formant (n, { 3, 8 }, 1.6) / n; }));
    put (58, spec (60, [] (int n) { return formant (n, { 5, 12 }, 2.0) / std::pow ((double) n, .8); }));
    put (59, spec (40, [] (int n) { return n % 2 ? 1.0 / n : .1 / n; }));
    put (60, spec (50, [] (int n) { return formant (n, { 4, 9 }, 1.5) / std::pow ((double) n, .7); }));
    put (61, spec (60, [] (int n) { return formant (n, { 6, 14 }, 2.0) / std::pow ((double) n, .5); }));
    put (62, spec (40, [] (int n) { return formant (n, { 2.7, 4.6, 10 }, 1.1) / std::pow ((double) n, .5); }));
    put (63, spec (30, [] (int n) { return formant (n, { 1.2, 3.1, 9 }, .9) / std::pow ((double) n, .7); }));
    put (64, spec (40, [] (int n) { return formant (n, { 2.0, 4.1, 9.5 }, 1.0) / std::pow ((double) n, .4); }));
    put (65, list ({ { 1, 1 }, { 3, .7 }, { 5, .5 }, { 7, .35 }, { 9, .25 } }));
    put (66, list ({ { 1, 1 }, { 4, .6 }, { 7, .45 }, { 10, .3 }, { 13, .2 } }));
    put (67, list ({ { 1, 1 }, { 2, .6 }, { 8, .5 }, { 9, .4 }, { 16, .3 } }));
    put (68, list ({ { 1, 1 }, { 6, .5 }, { 11, .45 }, { 16, .35 }, { 21, .25 }, { 26, .2 } }));
    put (69, list ({ { 1, 1 }, { 11, .5 }, { 12, .45 }, { 13, .4 } }));
    {   Rng r (777); put (70, spec (32, [&] (int) { return .2 + .8 * r.uni(); })); }
    put (71, spec (200, [] (int n) { return 1.0 / std::pow ((double) n, .6); }));
    put (72, spec (40, [] (int n) { return 1.0 / ((double) n * n); }));
    {   Rng r (31337);
        put (73, spec (60, [&] (int n) { const int c[] = { 7, 11, 13, 17, 19, 23 }; double s = n == 1 ? .4 : 0;
                                          for (int k : c) { if (std::abs (n - k) <= 1) s += (.5 + .5 * r.uni()) * (n == k ? 1 : .4); }
                                          return s; })); }
    {   Rng r (99991); put (74, spec (300, [&] (int n) { return r.uni() / std::sqrt ((double) n); })); }
    put (75, spec (200, [&] (int n) { return std::abs (std::sin (pi * n * .08)) / n; }));
}

void PcmBank::makeSequences()
{
    Rng r (2026);
    for (int s = 0; s < numWaves - firstSeq; ++s)
    {
        auto& w = waves[(size_t) (firstSeq + s)];
        w.kind = Kind::sequence;
        const int count = 2 + s % 3;
        for (int k = 0; k < count; ++k) w.seq.push_back (firstLoop + (int) (r.next() % (uint32_t) (firstSeq - firstLoop)));
        w.seqPeriod = .15f + (float) (s % 6) * .18f;
    }
}

float PcmBank::readLoop (int index, double phase, double freq, double sampleRate) const
{
    const auto& w = waves[(size_t) index];
    const double maxH = .48 * sampleRate / std::max (1.0, freq);
    const int lv = juce::jlimit (0, numLevels - 1, (int) std::ceil (std::log2 (512.0 / std::max (1.0, maxH))));
    const auto& t = w.levels[(size_t) lv];
    if (t.empty()) return 0.f;
    const double p = (phase - std::floor (phase)) * tableSize;
    const int i = (int) p;
    const float f = (float) (p - i);
    return t[(size_t) i] + (t[(size_t) i + 1] - t[(size_t) i]) * f;
}
}
