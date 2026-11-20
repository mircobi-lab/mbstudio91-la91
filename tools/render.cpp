// Render di prova offline: suona ogni preset di fabbrica (accordo + arpeggio) e salva un WAV per preset,
// stampando picco, RMS e controllo NaN. Uso: la91_render <cartella_uscita> [file.syx]
#include <juce_audio_formats/juce_audio_formats.h>
#include "LAEngine.h"
#include "Presets.h"

using namespace la91;

static PatchState fromRaw (const RawPatch& r)
{
    PatchState ps;
    const Block parts[4] = { Block::U1, Block::U2, Block::L1, Block::L2 };
    for (int i = 0; i < 4; ++i) for (int o = 0; o < 64; ++o) ps.part[i][o] = r.bytes[(size_t) (blockOffset (parts[i]) + o)];
    for (int i = 0; i < 2; ++i) for (int o = 0; o < 64; ++o) ps.com[i][o] = r.bytes[(size_t) (blockOffset (i == 0 ? Block::UC : Block::LC) + o)];
    for (int o = 0; o < 64; ++o) ps.pat[o] = r.bytes[(size_t) (blockOffset (Block::P) + o)];
    return ps;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    outDir.createDirectory();
    std::vector<RawPatch> list = factoryBank();
    if (argc > 2)
    {
        juce::MemoryBlock mb; juce::File (argv[2]).loadFileAsData (mb);
        list = parseSyx (mb);
        std::printf ("syx: %d patch\n", (int) list.size());
    }
    const double sr = 48000; const int block = 256;
    const double seconds = 7.0;
    int idx = 0;
    for (auto& raw : list)
    {
        Synth synth; synth.prepare (sr, block);
        auto ps = fromRaw (raw);
        const int total = (int) (seconds * sr);
        juce::AudioBuffer<float> out (2, total), buf (2, block);
        // accordo C-E-G-C tenuto 2.5 s, poi arpeggio, poi una nota bassa
        struct Ev { double t; int note; bool on; };
        std::vector<Ev> evs;
        for (int n : { 48, 60, 64, 67 }) { evs.push_back ({ .05, n, true }); evs.push_back ({ 2.5, n, false }); }
        const int arp[] = { 60, 64, 67, 72, 76, 72, 67, 64 };
        for (int k = 0; k < 8; ++k) { evs.push_back ({ 3.0 + k * .22, arp[k], true }); evs.push_back ({ 3.0 + k * .22 + .18, arp[k], false }); }
        evs.push_back ({ 5.0, 36, true }); evs.push_back ({ 5.8, 36, false });
        double sumSq = 0; float peak = 0; bool bad = false;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            juce::MidiBuffer midi;
            for (auto& e : evs)
            {
                const int at = (int) (e.t * sr) - pos;
                if (at >= 0 && at < n) midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) 100) : juce::MidiMessage::noteOff (1, e.note), at);
            }
            juce::AudioBuffer<float> b (buf.getArrayOfWritePointers(), 2, n);
            b.clear();
            synth.process (b, midi, ps);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                {
                    const float x = b.getSample (c, i);
                    if (! std::isfinite (x)) bad = true;
                    peak = std::max (peak, std::abs (x)); sumSq += (double) x * x;
                    out.setSample (c, pos + i, x);
                }
        }
        const double rms = std::sqrt (sumSq / (2.0 * total));
        const auto name = raw.name().trim();
        std::printf ("%02d %-20s peak %.3f  rms %.4f (%.1f dBFS)%s\n", idx + 1, name.toRawUTF8(), peak, rms, 20 * std::log10 (rms + 1e-12), bad ? "  NAN!" : "");
        auto file = outDir.getChildFile (juce::String (idx + 1).paddedLeft ('0', 2) + "_" + name.replaceCharacter (' ', '_') + ".wav");
        file.deleteFile();
        juce::WavAudioFormat wav;
        if (auto os = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
        {
            auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (24));
            if (writer) writer->writeFromAudioSampleBuffer (out, 0, total);
        }
        ++idx;
    }
    return 0;
}
