#pragma once
#include "MBStudioMidi.h"
#include "LAEngine.h"
#include "Presets.h"

class LA91Processor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    LA91Processor();
    ~LA91Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- banchi
    int numFactory() const { return (int) la91::factoryBank().size(); }
    int numImported() const { return (int) imported.size(); }
    juce::String programLabel (int index);           // "F-03" / "I-12"
    bool importSyx (const juce::File&, juce::String& message);
    void clearImported();
    juce::String currentPatchName() const { return names.name(); }
    juce::String currentToneName (bool upper) const { return names.toneName (upper); }

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;
    mbstudio::midi::MidiMapper midiMap;               // MIDI Learn e profili controller
    la91::Synth synth;
    std::function<void()> onProgramChanged;           // per l'editor (thread dei messaggi)
    void requestPanic() noexcept { panicRequested = true; }   // MIDI PANIC, eseguito all'inizio del blocco audio

private:
    la91::ParamReader reader;
    la91::PatchState patch;
    la91::RawPatch names = la91::initPatch();
    std::vector<la91::RawPatch> imported;
    int currentProgram = 0;
    std::atomic<int> pendingProgram { -1 };
    std::atomic<bool> panicRequested { false };

    const la91::RawPatch* programAt (int index) const;
    void handleAsyncUpdate() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA91Processor)
};
