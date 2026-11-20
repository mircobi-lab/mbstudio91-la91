#pragma once
#include "PluginProcessor.h"
#include "MBStudioLookAndFeel.h"
#include <juce_audio_utils/juce_audio_utils.h>

// Pannello in stile programmatore hardware (righe di fader come un PG a cursori), grafica MBStudio91.
class LA91Panel : public juce::Component, private juce::Timer
{
public:
    explicit LA91Panel (LA91Processor&);
    ~LA91Panel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshPrograms();

    static constexpr int baseW = 1360, baseH = 920;

private:
    enum class Scope { partial, common, patch };
    struct Fader
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        const la91::ParamDef* def = nullptr;
        Scope scope = Scope::partial;
    };
    struct Group
    {
        juce::String title;
        Scope scope;
        std::vector<Fader*> faders;
        juce::Rectangle<int> area;
    };

    LA91Processor& proc;
    std::vector<std::unique_ptr<Fader>> faders;
    std::vector<Group> groups;
    int selPartial = 2;        // 0 L1, 1 L2, 2 U1, 3 U2 (ordine del pannello)
    int selTone = 0;           // 0 UPPER, 1 LOWER
    juce::String touched, touchedValue;

    juce::TextButton toneBtn[2], partBtn[4], onBtn[4], prevBtn { "<" }, nextBtn { ">" }, loadBtn { "LOAD SYX" }, clearBtn { "CLEAR" }, portaBtn { "PORTA" };
    juce::ComboBox programBox;
    mbstudio::ui::LedMeter meter { 12 };
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> displayArea, diagramArea, leftArea;
    juce::String status;
    int statusTicks = 0;

    void buildGroups();
    void addGroup (const juce::String& title, Scope, const std::vector<la91::ParamDef>& defs, std::initializer_list<const char*> keys);
    void reattach();
    la91::Block partialBlock() const;
    la91::Block commonBlock() const { return selTone == 0 ? la91::Block::UC : la91::Block::LC; }
    juce::String scopeTag (Scope) const;
    int paramInt (const juce::String& id) const;
    void setParamInt (const juce::String& id, int v);
    void updateButtons();
    void paintDiagram (juce::Graphics&, juce::Rectangle<float>);
    void timerCallback() override;
};

class LA91Editor : public juce::AudioProcessorEditor
{
public:
    explicit LA91Editor (LA91Processor&);
    ~LA91Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    mbstudio::ui::HardwareLookAndFeel lnf;
    LA91Processor& proc;
    LA91Panel panel;
};
