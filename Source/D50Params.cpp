#include "D50Params.h"

namespace la91
{
// ------------------------------------------------------------------------------------------------ tabelle
const std::vector<ParamDef>& partialDefs()
{
    static const std::vector<ParamDef> d {
        // WG PITCH
        { "COARSE",   "COARSE",          "WG Pitch Coarse",        0, 72, Fmt::note24,      36 },
        { "FINE",     "FINE",            "WG Pitch Fine",          1,100, Fmt::bip50,       50 },
        { "KF",       "KF",              "WG Pitch Keyfollow",     2, 16, Fmt::keyFollow,   11 },
        { "LFOMODE",  "LFO\nMODE",       "WG LFO Mode",            3,  3, Fmt::lfoMode,      1 },
        { "ENVMODE",  "ENV\nMODE",       "WG P-ENV Mode",          4,  2, Fmt::penvMode,     1 },
        { "BENDMODE", "BEND\nMODE",      "WG Bender Mode",         5,  2, Fmt::bendMode,     2 },
        // WG WAVEFORM
        { "WF",       "WF",              "WG Waveform",            6,  1, Fmt::waveform,     1 },
        { "PCMWAVE",  "PCM\nWAVE",       "WG PCM Wave",            7, 99, Fmt::pcm,          0 },
        { "PW",       "PW",              "WG Pulse Width",         8,100, Fmt::plain,        0 },
        { "PWVELO",   "PW\nVELO",        "WG PW Velocity",         9, 14, Fmt::bip7,         7 },
        { "PWMSEL",   "PWM\nLFO SEL",    "WG PWM LFO Select",     10,  5, Fmt::lfoSel,       0 },
        { "PWMDEPTH", "PWM\nDEPTH",      "WG PWM Depth",          11,100, Fmt::plain,        0 },
        { "PWAFTER",  "PW\nAFTER",       "WG PW Aftertouch",      12, 14, Fmt::bip7,         7 },
        // TVF
        { "CUTOFF",   "CUTOFF\nFREQ",    "TVF Cutoff Freq",       13,100, Fmt::plain,       70 },
        { "RESO",     "RESO",            "TVF Resonance",         14, 30, Fmt::plain,        0 },
        { "TVFKF",    "KF",              "TVF Keyfollow",         15, 14, Fmt::tvfKeyFollow, 7 },
        { "TVFBIAS",  "BIAS\nPOINT",     "TVF Bias Point",        16,127, Fmt::noteBias,    96 },
        { "TVFBLVL",  "BIAS\nLEVEL",     "TVF Bias Level",        17, 14, Fmt::bip7,         7 },
        { "TVFDEPTH", "ENV\nDEPTH",      "TVF ENV Depth",         18,100, Fmt::plain,       30 },
        { "TVFVELO",  "ENV\nVELO",       "TVF ENV Velocity",      19,100, Fmt::plain,       50 },
        { "TVFDKF",   "DEPTH\nKF",       "TVF ENV Depth KF",      20,  4, Fmt::plain,        0 },
        { "TVFTKF",   "TKF",             "TVF ENV Time KF",       21,  4, Fmt::plain,        0 },
        { "TVFT1",    "T1",              "TVF ENV T1",            22,100, Fmt::plain,       10 },
        { "TVFT2",    "T2",              "TVF ENV T2",            23,100, Fmt::plain,       35 },
        { "TVFT3",    "T3",              "TVF ENV T3",            24,100, Fmt::plain,       40 },
        { "TVFT4",    "T4",              "TVF ENV T4",            25,100, Fmt::plain,       40 },
        { "TVFT5",    "T5",              "TVF ENV T5",            26,100, Fmt::plain,       35 },
        { "TVFL1",    "L1",              "TVF ENV L1",            27,100, Fmt::plain,      100 },
        { "TVFL2",    "L2",              "TVF ENV L2",            28,100, Fmt::plain,       70 },
        { "TVFL3",    "L3",              "TVF ENV L3",            29,100, Fmt::plain,       60 },
        { "TVFSUS",   "SUS L",           "TVF ENV Sustain",       30,100, Fmt::plain,       60 },
        { "TVFEND",   "END L",           "TVF ENV End",           31,100, Fmt::plain,        0 },
        { "TVFLSEL",  "LFO\nSELECT",     "TVF LFO Select",        32,  5, Fmt::lfoSel,       0 },
        { "TVFLDEP",  "LFO\nDEPTH",      "TVF LFO Depth",         33,100, Fmt::plain,        0 },
        { "TVFAFTER", "AFTER\nRANGE",    "TVF Aftertouch",        34, 14, Fmt::bip7,         7 },
        // TVA
        { "LEVEL",    "LEVEL",           "TVA Level",             35,100, Fmt::plain,      100 },
        { "TVAVELO",  "VELO",            "TVA Velocity Range",    36,100, Fmt::bip50,       70 },
        { "TVABIAS",  "BIAS\nPOINT",     "TVA Bias Point",        37,127, Fmt::noteBias,    96 },
        { "TVABLVL",  "BIAS\nLEVEL",     "TVA Bias Level",        38, 12, Fmt::neg12,       12 },
        { "TVAT1",    "T1",              "TVA ENV T1",            39,100, Fmt::plain,        0 },
        { "TVAT2",    "T2",              "TVA ENV T2",            40,100, Fmt::plain,       30 },
        { "TVAT3",    "T3",              "TVA ENV T3",            41,100, Fmt::plain,       30 },
        { "TVAT4",    "T4",              "TVA ENV T4",            42,100, Fmt::plain,       30 },
        { "TVAT5",    "T5",              "TVA ENV T5",            43,100, Fmt::plain,       35 },
        { "TVAL1",    "L1",              "TVA ENV L1",            44,100, Fmt::plain,      100 },
        { "TVAL2",    "L2",              "TVA ENV L2",            45,100, Fmt::plain,       90 },
        { "TVAL3",    "L3",              "TVA ENV L3",            46,100, Fmt::plain,       85 },
        { "TVASUS",   "SUS L",           "TVA ENV Sustain",       47,100, Fmt::plain,       85 },
        { "TVAEND",   "END L",           "TVA ENV End",           48,100, Fmt::plain,        0 },
        { "TVAT1V",   "T1\nVELO",        "TVA ENV T1 Velocity",   49,  4, Fmt::plain,        0 },
        { "TVATKF",   "TKF",             "TVA ENV Time KF",       50,  4, Fmt::plain,        0 },
        { "TVALSEL",  "LFO\nSELECT",     "TVA LFO Select",        51,  5, Fmt::lfoSel,       0 },
        { "TVALDEP",  "LFO\nDEPTH",      "TVA LFO Depth",         52,100, Fmt::plain,        0 },
        { "TVAAFTER", "AFTER\nRANGE",    "TVA Aftertouch",        53, 14, Fmt::bip7,         7 },
    };
    return d;
}

const std::vector<ParamDef>& commonDefs()
{
    static const std::vector<ParamDef> d {
        { "STRUCT",   "STRUC-\nTURE",    "Structure",             10,  6, Fmt::structure,    0 },
        { "PEVELO",   "VELO",            "P-ENV Velocity",        11,  2, Fmt::penvVelo,     0 },
        { "PETKF",    "TKF",             "P-ENV Time KF",         12,  4, Fmt::plain,        0 },
        { "PET1",     "T1",              "P-ENV T1",              13, 50, Fmt::plain,        0 },
        { "PET2",     "T2",              "P-ENV T2",              14, 50, Fmt::plain,        0 },
        { "PET3",     "T3",              "P-ENV T3",              15, 50, Fmt::plain,        0 },
        { "PET4",     "T4",              "P-ENV T4",              16, 50, Fmt::plain,        0 },
        { "PEL0",     "L0",              "P-ENV L0",              17,100, Fmt::bip50,       50 },
        { "PEL1",     "L1",              "P-ENV L1",              18,100, Fmt::bip50,       50 },
        { "PEL2",     "L2",              "P-ENV L2",              19,100, Fmt::bip50,       50 },
        { "PESUS",    "SUS L",           "P-ENV Sustain",         20,100, Fmt::bip50,       50 },
        { "PEEND",    "END L",           "P-ENV End",             21,100, Fmt::bip50,       50 },
        { "PMLFOD",   "LFO D",           "Pitch Mod LFO Depth",   22,100, Fmt::plain,        0 },
        { "PMLEVER",  "LEVER",           "Pitch Mod Lever",       23,100, Fmt::plain,       25 },
        { "PMAFTER",  "AFTER",           "Pitch Mod Aftertouch",  24,100, Fmt::plain,        0 },
        { "L1WF",     "WF",              "LFO-1 Waveform",        25,  3, Fmt::lfoWave,      0 },
        { "L1RATE",   "RATE",            "LFO-1 Rate",            26,100, Fmt::plain,       60 },
        { "L1DELAY",  "DELAY",           "LFO-1 Delay",           27,100, Fmt::plain,       30 },
        { "L1SYNC",   "SYNC",            "LFO-1 Sync",            28,  2, Fmt::lfoSync,      0 },
        { "L2WF",     "WF",              "LFO-2 Waveform",        29,  3, Fmt::lfoWave,      0 },
        { "L2RATE",   "RATE",            "LFO-2 Rate",            30,100, Fmt::plain,       40 },
        { "L2DELAY",  "DELAY",           "LFO-2 Delay",           31,100, Fmt::plain,        0 },
        { "L2SYNC",   "SYNC",            "LFO-2 Sync",            32,  2, Fmt::lfoSync,      0 },
        { "L3WF",     "WF",              "LFO-3 Waveform",        33,  3, Fmt::lfoWave,      0 },
        { "L3RATE",   "RATE",            "LFO-3 Rate",            34,100, Fmt::plain,       20 },
        { "L3DELAY",  "DELAY",           "LFO-3 Delay",           35,100, Fmt::plain,        0 },
        { "L3SYNC",   "SYNC",            "LFO-3 Sync",            36,  2, Fmt::lfoSync,      0 },
        { "EQLF",     "FREQ",            "Low EQ Freq",           37, 15, Fmt::eqLow,        4 },
        { "EQLG",     "GAIN",            "Low EQ Gain",           38, 24, Fmt::bip12,       12 },
        { "EQHF",     "FREQ",            "High EQ Freq",          39, 21, Fmt::eqHigh,      14 },
        { "EQHQ",     "Q",               "High EQ Q",             40,  8, Fmt::eqQ,          3 },
        { "EQHG",     "GAIN",            "High EQ Gain",          41, 24, Fmt::bip12,       12 },
        { "CHTYPE",   "TYPE",            "Chorus Type",           42,  7, Fmt::chorusType,   0 },
        { "CHRATE",   "RATE",            "Chorus Rate",           43,100, Fmt::plain,       35 },
        { "CHDEPTH",  "DEPTH",           "Chorus Depth",          44,100, Fmt::plain,       50 },
        { "CHBAL",    "BAL",             "Chorus Balance",        45,100, Fmt::plain,       30 },
        { "PMUTE",    "PARTIAL\nON",     "Partial On",            46,  3, Fmt::partialMute,  3 },
        { "PBAL",     "PART\nBAL",       "Partial Balance",       47,100, Fmt::plain,       50 },
    };
    return d;
}

const std::vector<ParamDef>& patchDefs()
{
    static const std::vector<ParamDef> d {
        { "KEYMODE",  "KEY\nMODE",       "Key Mode",              18,  8, Fmt::keyMode,      1 },
        { "SPLIT",    "SPLIT\nPOINT",    "Split Point",           19, 60, Fmt::note36,      24 },
        { "PORTAMODE","PORTA\nMODE",     "Portamento Mode",       20,  2, Fmt::ulSet,        2 },
        { "HOLDMODE", "HOLD\nMODE",      "Hold Mode",             21,  2, Fmt::ulSet,        2 },
        { "UKEY",     "U-KEY\nSHIFT",    "Upper Key Shift",       22, 48, Fmt::bip24,       24 },
        { "LKEY",     "L-KEY\nSHIFT",    "Lower Key Shift",       23, 48, Fmt::bip24,       24 },
        { "UFINE",    "U-FINE\nTUNE",    "Upper Fine Tune",       24,100, Fmt::bip50,       50 },
        { "LFINE",    "L-FINE\nTUNE",    "Lower Fine Tune",       25,100, Fmt::bip50,       50 },
        { "BENDER",   "BENDER\nRANGE",   "Bender Range",          26, 12, Fmt::plain,        2 },
        { "AFTBEND",  "AFTER\nBEND",     "Aftertouch Bend",       27, 24, Fmt::bip12,       12 },
        { "PORTATIME","PORTA\nTIME",     "Portamento Time",       28,100, Fmt::plain,        0 },
        { "OUTMODE",  "OUTPUT\nMODE",    "Output Mode",           29,  3, Fmt::outMode,      0 },
        { "REVTYPE",  "REV\nTYPE",       "Reverb Type",           30, 31, Fmt::revType,      1 },
        { "REVBAL",   "REV\nBAL",        "Reverb Balance",        31,100, Fmt::plain,       25 },
        { "VOLUME",   "TOTAL\nVOLUME",   "Total Volume",          32,100, Fmt::plain,       80 },
        { "TONEBAL",  "TONE\nBAL",       "Tone Balance",          33,100, Fmt::plain,       50 },
        { "PORTASW",  "PORTA",           "Portamento Switch",     44,  1, Fmt::onOff,        0 },
    };
    return d;
}

juce::String paramID (Block b, const ParamDef& d) { return juce::String (blockPrefix (b)) + "_" + d.key; }

const ParamDef* findDef (const std::vector<ParamDef>& v, const char* key)
{
    for (auto& d : v) if (std::strcmp (d.key, key) == 0) return &d;
    jassertfalse; return nullptr;
}

// ------------------------------------------------------------------------------------------------ formati
static juce::String noteName (int midi)
{
    static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[midi % 12]) + juce::String (midi / 12 - 1);
}
static juce::String signedStr (int v) { return v > 0 ? "+" + juce::String (v) : juce::String (v); }

juce::String pcmWaveName (int i)
{
    static const char* names[] = {
        "MALLET A", "MALLET B", "WOOD A", "WOOD B", "LOG", "HAMMER", "TAIKO", "TINE", "PLUCK A", "CHINK",
        "AGOGO", "TRIANGLE", "BELLS", "SCRAPE", "PICK", "PIANO LO", "PIANO MID", "PIANO HI", "HARPSI", "HARP",
        "ORG PERC", "STEEL STR", "NYLON STR", "E.GTR A", "E.GTR B", "DIRTY GTR", "PICK BASS", "POP BASS", "THUMP", "UPRIGHT",
        "REED", "BREATH", "STEAM", "FLUTE HI", "FLUTE LO", "GUIRO", "BAMBOO FL", "FL HARM", "LIPS A", "LIPS B",
        "TRUMPET", "TROMBONE", "CONTRA", "CELLO", "VLN BOW", "VIOLIN", "PIZZ",
        "DRAWBAR LP", "ORGAN HI LP", "ORGAN LO LP", "EP LP A", "EP LP B", "CLAVI LP", "HARPSI LP", "BASS LP A", "BASS LP B",
        "GTR LP", "CELLO LP", "VIOLIN LP", "REED LP", "SAX LP A", "SAX LP B", "VOX AH LP", "VOX OO LP", "MALE LP",
        "SPECTRUM 1", "SPECTRUM 2", "SPECTRUM 3", "SPECTRUM 4", "SPECTRUM 5", "SPECTRUM 6",
        "BRIGHT LP", "DARK LP", "METAL LP", "NOISE LP", "PULSE LP" };
    constexpr int named = (int) (sizeof (names) / sizeof (names[0]));   // 76
    if (i >= 0 && i < named) return names[i];
    return "SEQ " + juce::String (i - named + 1).paddedLeft ('0', 2);
}

juce::String formatValue (Fmt f, int v)
{
    switch (f)
    {
        case Fmt::plain:  return juce::String (v);
        case Fmt::plus1:  return juce::String (v + 1);
        case Fmt::bip7:   return signedStr (v - 7);
        case Fmt::bip12:  return signedStr (v - 12);
        case Fmt::bip24:  return signedStr (v - 24);
        case Fmt::bip50:  return signedStr (v - 50);
        case Fmt::neg12:  return signedStr (v - 12);
        case Fmt::note24: return noteName (24 + v);
        case Fmt::note36: return noteName (36 + v);
        case Fmt::noteBias: return (v < 64 ? "<" : ">") + noteName (33 + (v % 64));
        case Fmt::keyFollow:
        case Fmt::tvfKeyFollow:
        {
            static const char* k[] = { "-1", "-1/2", "-1/4", "0", "1/8", "1/4", "3/8", "1/2", "5/8", "3/4", "7/8", "1", "5/4", "3/2", "2", "S1", "S2" };
            return k[juce::jlimit (0, 16, v)];
        }
        case Fmt::lfoMode:  { static const char* k[] = { "OFF", "(+)", "(-)", "A&L" }; return k[juce::jlimit (0, 3, v)]; }
        case Fmt::penvMode: { static const char* k[] = { "OFF", "(+)", "(-)" }; return k[juce::jlimit (0, 2, v)]; }
        case Fmt::bendMode: { static const char* k[] = { "OFF", "KF", "NORMAL" }; return k[juce::jlimit (0, 2, v)]; }
        case Fmt::waveform: return v == 0 ? "SQU" : "SAW";
        case Fmt::pcm:      return juce::String (v + 1) + " " + pcmWaveName (v);
        case Fmt::lfoSel:   { static const char* k[] = { "+1", "-1", "+2", "-2", "+3", "-3" }; return k[juce::jlimit (0, 5, v)]; }
        case Fmt::structure:{ static const char* k[] = { "1 S+S", "2 S+RM(S)", "3 P+S", "4 P+RM(S)", "5 S+RM(P)", "6 P+P", "7 P+RM(P)" }; return k[juce::jlimit (0, 6, v)]; }
        case Fmt::lfoWave:  { static const char* k[] = { "TRI", "SAW", "SQU", "RND" }; return k[juce::jlimit (0, 3, v)]; }
        case Fmt::lfoSync:  { static const char* k[] = { "OFF", "ON", "KEY" }; return k[juce::jlimit (0, 2, v)]; }
        case Fmt::eqLow:    { static const int k[] = { 63, 75, 88, 105, 125, 150, 175, 210, 250, 300, 350, 420, 500, 600, 700, 840 }; return juce::String (k[juce::jlimit (0, 15, v)]) + " HZ"; }
        case Fmt::eqHigh:   { static const char* k[] = { "250", "300", "350", "420", "500", "600", "700", "840", "1K", "1.2K", "1.4K", "1.7K", "2K", "2.4K", "2.8K", "3.4K", "4K", "4.8K", "5.7K", "6.7K", "8K", "9.5K" }; return juce::String (k[juce::jlimit (0, 21, v)]) + " HZ"; }
        case Fmt::eqQ:      { static const char* k[] = { "0.3", "0.5", "0.7", "1.0", "1.4", "2.0", "3.0", "4.2", "6.0" }; return k[juce::jlimit (0, 8, v)]; }
        case Fmt::chorusType:{ static const char* k[] = { "CHORUS 1", "CHORUS 2", "FLANGER 1", "FLANGER 2", "FB CHORUS", "TREMOLO", "CHORUS TREM", "DIMENSION" }; return k[juce::jlimit (0, 7, v)]; }
        case Fmt::partialMute: { static const char* k[] = { "OFF OFF", "ON OFF", "OFF ON", "ON ON" }; return k[juce::jlimit (0, 3, v)]; }
        case Fmt::keyMode:  { static const char* k[] = { "WHOLE", "DUAL", "SPLIT", "SEPARATE", "WHOLE SOLO", "DUAL SOLO", "SPLIT U-SOLO", "SPLIT L-SOLO", "SEPARATE SOLO" }; return k[juce::jlimit (0, 8, v)]; }
        case Fmt::ulSet:    { static const char* k[] = { "U", "L", "U+L" }; return k[juce::jlimit (0, 2, v)]; }
        case Fmt::outMode:  { static const char* k[] = { "MIX", "SEP", "U REV", "L REV" }; return k[juce::jlimit (0, 3, v)]; }
        case Fmt::revType:
        {
            static const char* k[] = { "SMALL HALL", "MEDIUM HALL", "LARGE HALL", "CHAPEL", "BOX", "SMALL METAL", "SMALL ROOM", "MEDIUM ROOM",
                "MED-LARGE ROOM", "LARGE ROOM", "DELAY 102MS", "X-DELAY 180MS", "X-DELAY 224MS", "X-DELAY 148/296", "SHORT GATE", "LONG GATE",
                "BRIGHT HALL", "LARGE CAVE", "STEEL PAN", "DELAY 248MS", "DELAY 338MS", "X-DELAY 157MS", "X-DELAY 252MS", "X-DELAY 274/137",
                "GATE REVERB", "REV GATE 360", "REV GATE 480", "SLAP SHORT", "SLAP MEDIUM", "SLAP LONG", "TWISTED SPACE", "SPACE" };
            return k[juce::jlimit (0, 31, v)];
        }
        case Fmt::chaseMode: { static const char* k[] = { "U-L", "U-L-L", "U-L-U" }; return k[juce::jlimit (0, 2, v)]; }
        case Fmt::onOff:    return v ? "ON" : "OFF";
        case Fmt::penvVelo: return juce::String (v);
    }
    return juce::String (v);
}

// ------------------------------------------------------------------------------------------------ layout
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&] (Block b, const ParamDef& d, const juce::String& namePrefix)
    {
        const auto f = d.fmt;
        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { paramID (b, d), 1 }, namePrefix + d.longName, 0, d.max, d.def,
            juce::AudioParameterIntAttributes().withStringFromValueFunction ([f] (int v, int) { return formatValue (f, v); })));
    };
    for (auto b : { Block::P })  for (auto& d : patchDefs())   add (b, d, "");
    for (auto b : { Block::UC, Block::LC }) for (auto& d : commonDefs()) add (b, d, b == Block::UC ? "Upper " : "Lower ");
    for (auto b : { Block::U1, Block::U2, Block::L1, Block::L2 })
        for (auto& d : partialDefs()) add (b, d, juce::String (blockPrefix (b)) + " ");
    return layout;
}

// ------------------------------------------------------------------------------------------------ nomi D-50
juce::String d50ToAscii (const uint8_t* data, int len)
{
    juce::String s;
    for (int i = 0; i < len; ++i)
    {
        const int c = data[i];
        char a = ' ';
        if (c == 0) a = ' ';
        else if (c < 27) a = (char) ('A' + c - 1);
        else if (c < 53) a = (char) ('a' + c - 27);
        else if (c < 63) a = (char) ('0' + c - 53);
        else if (c == 63) a = '-';
        s += juce::String::charToString (a);
    }
    return s.trimEnd();
}

void asciiToD50 (const juce::String& s, uint8_t* data, int len)
{
    for (int i = 0; i < len; ++i)
    {
        const auto ch = i < s.length() ? s[i] : ' ';
        uint8_t c = 0;
        if (ch >= 'A' && ch <= 'Z') c = (uint8_t) (ch - 'A' + 1);
        else if (ch >= 'a' && ch <= 'z') c = (uint8_t) (ch - 'a' + 27);
        else if (ch >= '0' && ch <= '9') c = (uint8_t) (ch - '0' + 53);
        else if (ch == '-') c = 63;
        data[i] = c;
    }
}

juce::String RawPatch::name() const { return d50ToAscii (bytes.data() + blockOffset (Block::P), patchNameLen); }
juce::String RawPatch::toneName (bool upper) const { return d50ToAscii (bytes.data() + blockOffset (upper ? Block::UC : Block::LC), toneNameLen); }

static void forEachParam (const std::function<void (Block, const ParamDef&)>& fn)
{
    for (auto& d : patchDefs()) fn (Block::P, d);
    for (auto b : { Block::UC, Block::LC }) for (auto& d : commonDefs()) fn (b, d);
    for (auto b : { Block::U1, Block::U2, Block::L1, Block::L2 }) for (auto& d : partialDefs()) fn (b, d);
}

RawPatch initPatch()
{
    RawPatch p;
    forEachParam ([&] (Block b, const ParamDef& d) { p.bytes[(size_t) (blockOffset (b) + d.offset)] = (uint8_t) d.def; });
    asciiToD50 ("INIT PATCH", p.bytes.data() + blockOffset (Block::P), patchNameLen);
    asciiToD50 ("INIT UPPER", p.bytes.data() + blockOffset (Block::UC), toneNameLen);
    asciiToD50 ("INIT LOWER", p.bytes.data() + blockOffset (Block::LC), toneNameLen);
    return p;
}

void applyRawPatch (juce::AudioProcessorValueTreeState& s, const RawPatch& p)
{
    forEachParam ([&] (Block b, const ParamDef& d)
    {
        if (auto* prm = dynamic_cast<juce::AudioParameterInt*> (s.getParameter (paramID (b, d))))
        {
            const int v = juce::jlimit (0, d.max, (int) p.bytes[(size_t) (blockOffset (b) + d.offset)]);
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v));
        }
    });
}

RawPatch captureRawPatch (const juce::AudioProcessorValueTreeState& s, const RawPatch& namesFrom)
{
    RawPatch p = namesFrom;
    forEachParam ([&] (Block b, const ParamDef& d)
    {
        if (auto* prm = dynamic_cast<juce::AudioParameterInt*> (s.getParameter (paramID (b, d))))
            p.bytes[(size_t) (blockOffset (b) + d.offset)] = (uint8_t) prm->get();
    });
    return p;
}

// ------------------------------------------------------------------------------------------------ .syx
std::vector<RawPatch> parseSyx (const juce::MemoryBlock& mb)
{
    // memoria lineare indicizzata dall'indirizzo a 21 bit (3 byte da 7 bit)
    std::map<int, uint8_t> mem;
    const auto* d = static_cast<const uint8_t*> (mb.getData());
    const int n = (int) mb.getSize();
    for (int i = 0; i < n; ++i)
    {
        if (d[i] != 0xF0) continue;
        int end = i + 1;
        while (end < n && d[end] != 0xF7) ++end;
        if (end >= n) break;
        // F0 41 dev 14 12 a1 a2 a3 data... sum F7
        if (end - i >= 10 && d[i + 1] == 0x41 && d[i + 3] == 0x14 && d[i + 4] == 0x12)
        {
            int addr = (d[i + 5] << 14) | (d[i + 6] << 7) | d[i + 7];
            for (int k = i + 8; k < end - 1; ++k) mem[addr++] = d[k];
        }
        i = end;
    }
    std::vector<RawPatch> out;
    auto extract = [&] (int base) -> bool
    {
        for (int k = 0; k < patchBytes; ++k) if (mem.find (base + k) == mem.end()) return false;
        RawPatch p;
        for (int k = 0; k < patchBytes; ++k) p.bytes[(size_t) k] = mem[base + k];
        out.push_back (p);
        return true;
    };
    const int bankBase = 2 << 14;   // 02 00 00
    for (int k = 0; k < 64; ++k) if (! extract (bankBase + k * patchBytes)) break;
    if (out.empty()) extract (0);   // area temporanea
    // valori fuori scala (file danneggiati o di altri modelli) riportati nei limiti dei parametri
    for (auto& p : out)
        forEachParam ([&] (Block b, const ParamDef& d)
        {
            auto& v = p.bytes[(size_t) (blockOffset (b) + d.offset)];
            v = (uint8_t) juce::jlimit (0, d.max, (int) v);
        });
    return out;
}
}
