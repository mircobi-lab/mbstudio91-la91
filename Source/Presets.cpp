#include "Presets.h"

namespace la91
{
namespace
{
struct Builder
{
    RawPatch p = initPatch();

    Builder& name (const char* patch, const char* upper, const char* lower)
    {
        asciiToD50 (patch, p.bytes.data() + blockOffset (Block::P), patchNameLen);
        asciiToD50 (upper, p.bytes.data() + blockOffset (Block::UC), toneNameLen);
        asciiToD50 (lower, p.bytes.data() + blockOffset (Block::LC), toneNameLen);
        return *this;
    }
    void put (Block b, const std::vector<ParamDef>& defs, const char* key, int v)
    {
        const auto* d = findDef (defs, key);
        p.bytes[(size_t) (blockOffset (b) + d->offset)] = (uint8_t) juce::jlimit (0, d->max, v);
    }
    Builder& part (Block b, std::initializer_list<std::pair<const char*, int>> kv) { for (auto& x : kv) put (b, partialDefs(), x.first, x.second); return *this; }
    Builder& com (Block b, std::initializer_list<std::pair<const char*, int>> kv)  { for (auto& x : kv) put (b, commonDefs(), x.first, x.second); return *this; }
    Builder& pat (std::initializer_list<std::pair<const char*, int>> kv)           { for (auto& x : kv) put (Block::P, patchDefs(), x.first, x.second); return *this; }

    Builder& tva (Block b, int t1, int t2, int t3, int t4, int t5, int l1, int l2, int l3, int sus)
    {
        return part (b, { { "TVAT1", t1 }, { "TVAT2", t2 }, { "TVAT3", t3 }, { "TVAT4", t4 }, { "TVAT5", t5 },
                          { "TVAL1", l1 }, { "TVAL2", l2 }, { "TVAL3", l3 }, { "TVASUS", sus }, { "TVAEND", 0 } });
    }
    Builder& tvf (Block b, int t1, int t2, int t3, int t4, int t5, int l1, int l2, int l3, int sus, int end = 0)
    {
        return part (b, { { "TVFT1", t1 }, { "TVFT2", t2 }, { "TVFT3", t3 }, { "TVFT4", t4 }, { "TVFT5", t5 },
                          { "TVFL1", l1 }, { "TVFL2", l2 }, { "TVFL3", l3 }, { "TVFSUS", sus }, { "TVFEND", end } });
    }
    // PCM d'attacco breve: suona per intero e poi tace
    Builder& pcmHit (Block b, int wave1based, int level = 100)
    {
        part (b, { { "PCMWAVE", wave1based - 1 }, { "LEVEL", level } });
        return tva (b, 0, 40, 50, 50, 30, 100, 90, 0, 0);
    }
};

std::vector<RawPatch> build()
{
    using B = Block;
    std::vector<RawPatch> bank;
    auto add = [&] (Builder& b) { bank.push_back (b.p); };

    {   Builder b; b.name ("MB DIGITAL DREAM", "BELL PAD", "SAW CLOUD");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 2 }, { "REVBAL", 45 }, { "TONEBAL", 55 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 45 }, { "CHTYPE", 0 }, { "CHBAL", 50 }, { "PMLFOD", 8 } });
        b.pcmHit (B::U1, 13, 92);
        b.part (B::U2, { { "WF", 1 }, { "CUTOFF", 55 }, { "RESO", 8 }, { "TVFDEPTH", 25 }, { "COARSE", 48 }, { "FINE", 54 } });
        b.tvf (B::U2, 40, 60, 60, 60, 60, 80, 60, 50, 50); b.tva (B::U2, 45, 50, 50, 50, 62, 100, 95, 90, 90);
        b.com (B::LC, { { "STRUCT", 0 }, { "CHTYPE", 1 }, { "CHBAL", 60 }, { "CHRATE", 25 }, { "PMLFOD", 6 } });
        b.part (B::L1, { { "WF", 1 }, { "CUTOFF", 45 }, { "TVFDEPTH", 20 }, { "FINE", 46 } });
        b.part (B::L2, { { "WF", 1 }, { "CUTOFF", 48 }, { "TVFDEPTH", 20 }, { "FINE", 55 }, { "COARSE", 24 } });
        for (auto bb : { B::L1, B::L2 }) { b.tva (bb, 55, 50, 50, 50, 65, 100, 95, 90, 90); b.tvf (bb, 55, 60, 60, 60, 60, 90, 70, 60, 60); }
        add (b); }

    {   Builder b; b.name ("CUNEO BRASS", "BRASS U", "BRASS L");
        b.pat ({ { "KEYMODE", 0 }, { "REVTYPE", 7 }, { "REVBAL", 25 } });
        b.com (B::UC, { { "STRUCT", 0 }, { "CHTYPE", 0 }, { "CHBAL", 35 }, { "PMLFOD", 4 }, { "PEL0", 46 }, { "PET1", 12 } });
        for (auto bb : { B::U1, B::U2 })
        {
            b.part (bb, { { "WF", 1 }, { "CUTOFF", 38 }, { "RESO", 4 }, { "TVFDEPTH", 55 }, { "TVFVELO", 60 }, { "TVFKF", 9 } });
            b.tvf (bb, 30, 45, 50, 50, 40, 100, 75, 70, 70); b.tva (bb, 20, 40, 45, 45, 38, 100, 92, 90, 90);
        }
        b.part (B::U2, { { "FINE", 57 } });
        add (b); }

    {   Builder b; b.name ("GLASS PIANO", "GLASS", "PIANO BODY");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 1 }, { "REVBAL", 30 }, { "TONEBAL", 45 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 40 }, { "CHTYPE", 0 }, { "CHBAL", 30 } });
        b.pcmHit (B::U1, 17);
        b.part (B::U2, { { "WF", 0 }, { "PW", 30 }, { "CUTOFF", 60 }, { "TVFDEPTH", 30 }, { "COARSE", 48 } });
        b.tva (B::U2, 0, 60, 70, 75, 45, 100, 60, 35, 0); b.tvf (B::U2, 0, 50, 60, 60, 40, 100, 50, 30, 20);
        b.com (B::LC, { { "STRUCT", 5 }, { "PBAL", 35 } });
        b.pcmHit (B::L1, 16, 95);
        b.part (B::L2, { { "PCMWAVE", 50 }, { "LEVEL", 80 } });
        b.tva (B::L2, 0, 65, 70, 75, 45, 100, 55, 30, 0);
        add (b); }

    {   Builder b; b.name ("BREATH OF PIEMONTE", "BREATH PAD", "VOX AIR");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 3 }, { "REVBAL", 50 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 50 }, { "CHTYPE", 1 }, { "CHBAL", 55 }, { "PMLFOD", 10 }, { "L1DELAY", 50 } });
        b.pcmHit (B::U1, 32, 90);
        b.part (B::U2, { { "WF", 1 }, { "CUTOFF", 42 }, { "RESO", 10 }, { "TVFDEPTH", 18 }, { "TVFLSEL", 2 }, { "TVFLDEP", 15 } });
        b.tva (B::U2, 60, 55, 55, 55, 70, 100, 95, 92, 92); b.tvf (B::U2, 60, 65, 65, 65, 65, 100, 70, 60, 55);
        b.com (B::LC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 7 }, { "CHBAL", 55 } });
        b.part (B::L1, { { "PCMWAVE", 62 }, { "LEVEL", 90 } }); b.tva (B::L1, 62, 55, 55, 55, 70, 100, 95, 92, 92);
        b.part (B::L2, { { "PCMWAVE", 63 }, { "LEVEL", 85 }, { "FINE", 54 } }); b.tva (B::L2, 65, 55, 55, 55, 70, 100, 95, 92, 92);
        add (b); }

    {   Builder b; b.name ("RING BELLS", "RING", "SHIMMER");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 16 }, { "REVBAL", 45 }, { "TONEBAL", 60 } });
        b.com (B::UC, { { "STRUCT", 1 }, { "PBAL", 70 } });
        b.part (B::U1, { { "WF", 0 }, { "CUTOFF", 85 }, { "TVFDEPTH", 0 } });
        b.part (B::U2, { { "WF", 0 }, { "CUTOFF", 85 }, { "TVFDEPTH", 0 }, { "COARSE", 55 }, { "FINE", 62 } });
        for (auto bb : { B::U1, B::U2 }) b.tva (bb, 0, 55, 72, 80, 60, 100, 55, 25, 0);
        b.com (B::LC, { { "STRUCT", 0 }, { "CHTYPE", 0 }, { "CHBAL", 50 } });
        b.part (B::L1, { { "WF", 0 }, { "PW", 60 }, { "PWMSEL", 2 }, { "PWMDEPTH", 40 }, { "CUTOFF", 62 } });
        b.part (B::L2, { { "WF", 0 }, { "LEVEL", 0 } });
        b.tva (B::L1, 30, 60, 70, 70, 60, 85, 70, 55, 0);
        add (b); }

    {   Builder b; b.name ("PLUCK SPACE", "PLUCK", "ECHO SAW");
        b.pat ({ { "KEYMODE", 0 }, { "REVTYPE", 12 }, { "REVBAL", 40 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 45 }, { "CHTYPE", 0 }, { "CHBAL", 40 } });
        b.pcmHit (B::U1, 9);
        b.part (B::U2, { { "WF", 1 }, { "CUTOFF", 40 }, { "RESO", 12 }, { "TVFDEPTH", 45 } });
        b.tvf (B::U2, 0, 45, 60, 60, 40, 100, 40, 20, 10); b.tva (B::U2, 0, 50, 62, 70, 40, 100, 70, 40, 0);
        add (b); }

    {   Builder b; b.name ("SYNTH BASS 91", "BASS", "SUB");
        b.pat ({ { "KEYMODE", 4 }, { "REVBAL", 0 }, { "VOLUME", 85 } });
        b.com (B::UC, { { "STRUCT", 0 }, { "PBAL", 50 }, { "CHBAL", 0 } });
        b.part (B::U1, { { "WF", 0 }, { "PW", 20 }, { "COARSE", 24 }, { "CUTOFF", 30 }, { "RESO", 18 }, { "TVFDEPTH", 50 }, { "TVFVELO", 70 } });
        b.part (B::U2, { { "WF", 1 }, { "COARSE", 24 }, { "FINE", 53 }, { "CUTOFF", 34 }, { "RESO", 10 }, { "TVFDEPTH", 45 } });
        for (auto bb : { B::U1, B::U2 }) { b.tvf (bb, 0, 38, 45, 45, 25, 100, 35, 25, 20); b.tva (bb, 0, 40, 50, 50, 20, 100, 92, 88, 88); }
        add (b); }

    {   Builder b; b.name ("STRING MACHINE", "PWM STR", "BOW STR");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 2 }, { "REVBAL", 40 } });
        b.com (B::UC, { { "STRUCT", 0 }, { "PBAL", 50 }, { "CHTYPE", 1 }, { "CHBAL", 70 }, { "CHRATE", 20 }, { "L2RATE", 45 } });
        b.part (B::U1, { { "WF", 0 }, { "PW", 35 }, { "PWMSEL", 2 }, { "PWMDEPTH", 45 }, { "CUTOFF", 58 }, { "TVFDEPTH", 10 } });
        b.part (B::U2, { { "WF", 1 }, { "FINE", 56 }, { "CUTOFF", 55 }, { "TVFDEPTH", 10 } });
        for (auto bb : { B::U1, B::U2 }) b.tva (bb, 50, 50, 50, 50, 62, 100, 95, 92, 92);
        b.com (B::LC, { { "STRUCT", 5 }, { "PBAL", 55 } });
        b.part (B::L1, { { "PCMWAVE", 44 }, { "LEVEL", 80 } }); b.tva (B::L1, 10, 45, 60, 60, 45, 100, 60, 0, 0);
        b.part (B::L2, { { "PCMWAVE", 58 }, { "LEVEL", 95 } }); b.tva (B::L2, 45, 50, 50, 50, 62, 100, 95, 92, 92);
        add (b); }

    {   Builder b; b.name ("VOX SPECTRUM", "SPECTRA", "VOX");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 31 }, { "REVBAL", 45 } });
        b.com (B::UC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 4 }, { "CHBAL", 40 } });
        b.part (B::U1, { { "PCMWAVE", 66 } }); b.tva (B::U1, 40, 60, 60, 60, 60, 100, 85, 80, 80);
        b.part (B::U2, { { "PCMWAVE", 68 }, { "COARSE", 48 } }); b.tva (B::U2, 55, 60, 60, 60, 60, 80, 75, 70, 70);
        b.com (B::LC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 7 }, { "CHBAL", 50 } });
        b.part (B::L1, { { "PCMWAVE", 62 } }); b.tva (B::L1, 50, 55, 55, 55, 62, 100, 95, 90, 90);
        b.part (B::L2, { { "PCMWAVE", 64 }, { "FINE", 53 } }); b.tva (B::L2, 50, 55, 55, 55, 62, 100, 95, 90, 90);
        add (b); }

    {   Builder b; b.name ("MALLET SPLIT", "MALLET", "PICK BASS");
        b.pat ({ { "KEYMODE", 2 }, { "SPLIT", 18 }, { "REVTYPE", 7 }, { "REVBAL", 30 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 45 } });
        b.pcmHit (B::U1, 1);
        b.part (B::U2, { { "WF", 0 }, { "CUTOFF", 48 }, { "TVFDEPTH", 20 } });
        b.tva (B::U2, 0, 50, 65, 70, 45, 90, 50, 20, 0);
        b.com (B::LC, { { "STRUCT", 2 }, { "PBAL", 50 } });
        b.pcmHit (B::L1, 27);
        b.part (B::L2, { { "WF", 1 }, { "CUTOFF", 30 }, { "TVFDEPTH", 40 }, { "RESO", 6 } });
        b.tvf (B::L2, 0, 40, 50, 50, 30, 100, 40, 30, 25); b.tva (B::L2, 0, 45, 55, 55, 30, 100, 85, 75, 75);
        add (b); }

    {   Builder b; b.name ("ALPINE PIPES", "REED", "DRONE");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 3 }, { "REVBAL", 40 }, { "TONEBAL", 60 } });
        b.com (B::UC, { { "STRUCT", 5 }, { "PBAL", 40 }, { "PMLFOD", 10 }, { "L1DELAY", 40 } });
        b.pcmHit (B::U1, 31);
        b.part (B::U2, { { "PCMWAVE", 59 } }); b.tva (B::U2, 25, 50, 50, 50, 50, 100, 92, 90, 90);
        b.com (B::LC, { { "STRUCT", 0 } });
        b.part (B::L1, { { "WF", 0 }, { "PW", 50 }, { "CUTOFF", 40 }, { "COARSE", 24 }, { "KF", 3 } });
        b.part (B::L2, { { "WF", 1 }, { "CUTOFF", 35 }, { "COARSE", 31 }, { "KF", 3 } });
        for (auto bb : { B::L1, B::L2 }) b.tva (bb, 55, 50, 50, 50, 65, 70, 70, 70, 70);
        add (b); }

    {   Builder b; b.name ("SEQ MOTION", "SEQ A", "SEQ B");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 13 }, { "REVBAL", 35 } });
        b.com (B::UC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 0 }, { "CHBAL", 40 } });
        b.part (B::U1, { { "PCMWAVE", 81 } }); b.part (B::U2, { { "PCMWAVE", 88 }, { "COARSE", 48 } });
        for (auto bb : { B::U1, B::U2 }) b.tva (bb, 20, 50, 50, 50, 50, 100, 90, 88, 88);
        b.com (B::LC, { { "STRUCT", 0 } });
        b.part (B::L1, { { "WF", 1 }, { "CUTOFF", 35 }, { "RESO", 20 }, { "TVFLSEL", 4 }, { "TVFLDEP", 35 } });
        b.com (B::LC, { { "L3RATE", 55 } });
        b.part (B::L2, { { "LEVEL", 0 } });
        b.tva (B::L1, 40, 50, 50, 50, 55, 85, 80, 80, 80);
        add (b); }

    {   Builder b; b.name ("SOFT TINE EP", "TINE", "BODY");
        b.pat ({ { "KEYMODE", 0 }, { "REVTYPE", 7 }, { "REVBAL", 25 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 55 }, { "CHTYPE", 5 }, { "CHRATE", 45 }, { "CHDEPTH", 40 }, { "CHBAL", 60 } });
        b.pcmHit (B::U1, 8, 85);
        b.part (B::U2, { { "WF", 0 }, { "CUTOFF", 40 }, { "TVFDEPTH", 25 }, { "TVFVELO", 80 } });
        b.tvf (B::U2, 0, 40, 60, 60, 40, 100, 50, 30, 20); b.tva (B::U2, 0, 60, 72, 78, 45, 100, 75, 50, 0);
        add (b); }

    {   Builder b; b.name ("SOLO SYNC LEAD", "LEAD", "LEAD L");
        b.pat ({ { "KEYMODE", 4 }, { "PORTASW", 1 }, { "PORTATIME", 35 }, { "REVTYPE", 11 }, { "REVBAL", 30 } });
        b.com (B::UC, { { "STRUCT", 1 }, { "PBAL", 55 }, { "PMLFOD", 6 }, { "PMLEVER", 40 }, { "L1DELAY", 55 } });
        b.part (B::U1, { { "WF", 1 }, { "CUTOFF", 62 }, { "RESO", 12 }, { "TVFDEPTH", 25 } });
        b.part (B::U2, { { "WF", 1 }, { "COARSE", 43 }, { "CUTOFF", 70 }, { "TVFDEPTH", 0 } });
        for (auto bb : { B::U1, B::U2 }) b.tva (bb, 5, 40, 50, 50, 35, 100, 95, 92, 92);
        add (b); }

    {   Builder b; b.name ("NIGHT CHOIR", "CHOIR HI", "CHOIR LO");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 2 }, { "REVBAL", 55 } });
        b.com (B::UC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 7 }, { "CHBAL", 60 }, { "PMLFOD", 6 } });
        b.part (B::U1, { { "PCMWAVE", 62 } }); b.part (B::U2, { { "PCMWAVE", 63 }, { "FINE", 56 } });
        b.com (B::LC, { { "STRUCT", 5 }, { "PBAL", 50 }, { "CHTYPE", 1 }, { "CHBAL", 60 } });
        b.part (B::L1, { { "PCMWAVE", 64 }, { "COARSE", 24 } }); b.part (B::L2, { { "PCMWAVE", 62 }, { "FINE", 45 } });
        for (auto bb : { B::U1, B::U2, B::L1, B::L2 }) b.tva (bb, 62, 55, 55, 55, 68, 100, 95, 92, 92);
        add (b); }

    {   Builder b; b.name ("STEEL SKY", "STEEL", "PAD");
        b.pat ({ { "KEYMODE", 1 }, { "REVTYPE", 18 }, { "REVBAL", 40 } });
        b.com (B::UC, { { "STRUCT", 2 }, { "PBAL", 45 } });
        b.pcmHit (B::U1, 22);
        b.part (B::U2, { { "WF", 0 }, { "PW", 45 }, { "CUTOFF", 52 }, { "TVFDEPTH", 25 } });
        b.tva (B::U2, 0, 55, 70, 75, 50, 100, 60, 35, 0);
        b.com (B::LC, { { "STRUCT", 0 }, { "CHTYPE", 2 }, { "CHBAL", 50 }, { "CHRATE", 15 } });
        b.part (B::L1, { { "WF", 1 }, { "CUTOFF", 40 }, { "TVFDEPTH", 15 } }); b.part (B::L2, { { "LEVEL", 0 } });
        b.tva (B::L1, 60, 55, 55, 55, 65, 85, 85, 85, 85);
        add (b); }

    {   Builder b; b.name ("INIT PATCH", "INIT UPPER", "INIT LOWER");
        b.pat ({ { "KEYMODE", 0 } });
        add (b); }
    return bank;
}
}

const std::vector<RawPatch>& factoryBank()
{
    static const std::vector<RawPatch> bank = build();
    return bank;
}
}
