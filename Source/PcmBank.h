#pragma once
/*  LA-91 — 100 forme d'onda "PCM" generate in proprio al primo avvio (nessun campione esterno).
    0..46   attacchi one-shot (percussioni, pizzichi, soffi, archetti) a C4, 48 kHz
    47..75  loop a ciclo singolo con mip-map anti-aliasing
    76..99  sequenze: 3-4 loop che si alternano con dissolvenza                                   */
#include <juce_dsp/juce_dsp.h>

namespace la91
{
class PcmBank
{
public:
    static constexpr int numWaves = 100, firstLoop = 47, firstSeq = 76;
    static constexpr int tableSize = 2048, numLevels = 10;          // livello k: al massimo 512 >> k armoniche
    static constexpr double oneShotRate = 48000.0, rootHz = 261.6255653;

    PcmBank();

    enum class Kind { oneShot, loop, sequence };
    struct Wave
    {
        Kind kind = Kind::loop;
        std::vector<float> sample;                                    // one-shot
        std::array<std::vector<float>, numLevels> levels;             // loop
        std::vector<int> seq;                                         // sequenza: indici di loop
        float seqPeriod = 1.f;                                        // secondi per passo
        float gain = 1.f;
    };
    const Wave& wave (int i) const { return waves[(size_t) juce::jlimit (0, numWaves - 1, i)]; }

    // lettura di un loop: phase 0..1, freq in Hz (sceglie il livello senza aliasing)
    float readLoop (int index, double phase, double freq, double sampleRate) const;

private:
    std::vector<Wave> waves;
    void makeOneShots();
    void makeLoops();
    void makeSequences();
    void buildTable (Wave&, const std::vector<float>& harmonicAmps, uint32_t seed);
};
}
