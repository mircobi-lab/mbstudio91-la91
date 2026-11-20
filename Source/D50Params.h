#pragma once
/*  LA-91 — mappa dei parametri in stile D-50.
    I parametri usano gli stessi valori grezzi (0..max) e lo stesso ordine dei byte del dump SysEx
    descritto nel modello di DSynkant (Nil Geisweiller, GPL v2 o successiva): così un banco .syx
    si carica copiando i byte direttamente nei parametri.

    Blocchi di una patch (448 byte):  U1 0x000  U2 0x040  UC 0x080  L1 0x0C0  L2 0x100  LC 0x140  P 0x180   */
#include <juce_audio_processors/juce_audio_processors.h>

namespace la91
{
enum class Fmt
{
    plain, plus1, bip7, bip12, bip24, bip50, neg12, note24, noteBias, note36,
    keyFollow, tvfKeyFollow, lfoMode, penvMode, bendMode, waveform, pcm, lfoSel,
    structure, lfoWave, lfoSync, eqLow, eqHigh, eqQ, chorusType, partialMute,
    keyMode, ulSet, outMode, revType, chaseMode, onOff, penvVelo
};

struct ParamDef
{
    const char* key;      // suffisso dell'ID (es. "CUTOFF")
    const char* label;    // serigrafia sotto il chip, "\n" per andare a capo
    const char* longName; // nome per il display e l'automazione
    int offset;           // byte nel blocco
    int max;              // valore grezzo massimo (minimo sempre 0)
    Fmt fmt;
    int def;              // valore della patch iniziale
};

enum class Block { U1, U2, UC, L1, L2, LC, P };
constexpr int blockOffset (Block b)
{
    switch (b) { case Block::U1: return 0x000; case Block::U2: return 0x040; case Block::UC: return 0x080;
                 case Block::L1: return 0x0C0; case Block::L2: return 0x100; case Block::LC: return 0x140;
                 case Block::P:  return 0x180; }
    return 0;
}
inline const char* blockPrefix (Block b)
{
    switch (b) { case Block::U1: return "U1"; case Block::U2: return "U2"; case Block::UC: return "UC";
                 case Block::L1: return "L1"; case Block::L2: return "L2"; case Block::LC: return "LC";
                 case Block::P:  return "P"; }
    return "";
}

constexpr int patchBytes = 448;
constexpr int toneNameLen = 10, patchNameLen = 18;

// ---- tabelle (definite in D50Params.cpp)
const std::vector<ParamDef>& partialDefs();   // 54 parametri, blocchi U1 U2 L1 L2
const std::vector<ParamDef>& commonDefs();    // 38 parametri, blocchi UC LC (dal byte 10)
const std::vector<ParamDef>& patchDefs();     // blocco P (dal byte 18)

juce::String paramID (Block, const ParamDef&);
const ParamDef* findDef (const std::vector<ParamDef>&, const char* key);
juce::String formatValue (Fmt, int v);
juce::String pcmWaveName (int index0);        // 0..99

// raccoglie tutti i parametri per l'APVTS
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// ---- patch grezza (448 byte) <-> parametri
struct RawPatch
{
    std::array<uint8_t, patchBytes> bytes {};
    juce::String name() const;            // nome patch (18 caratteri D-50)
    juce::String toneName (bool upper) const;
};
RawPatch initPatch();
void applyRawPatch (juce::AudioProcessorValueTreeState&, const RawPatch&);
RawPatch captureRawPatch (const juce::AudioProcessorValueTreeState&, const RawPatch& namesFrom);

juce::String d50ToAscii (const uint8_t* data, int len);
void asciiToD50 (const juce::String& s, uint8_t* data, int len);

// Legge un file .syx (dump DT1 Roland, modello 0x14). Restituisce le patch trovate nell'area
// di memoria (02 00 00, fino a 64) o, in mancanza, nell'area temporanea (00 00 00).
std::vector<RawPatch> parseSyx (const juce::MemoryBlock&);
}
