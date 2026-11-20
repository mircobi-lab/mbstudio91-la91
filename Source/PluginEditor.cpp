#include "PluginEditor.h"

namespace ui = mbstudio::ui;
namespace C = mbstudio::ui::colours;
using la91::Block;

namespace
{
constexpr int pitch = 38, groupGap = 10, rowH = 166, mainX = 192;
constexpr int rowY[4] = { 124, 300, 476, 652 };
}

// ================================================================================================ pannello
LA91Panel::LA91Panel (LA91Processor& p)
    : proc (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    buildGroups();

    const char* toneNames[2] = { "UPPER", "LOWER" };
    for (int t = 0; t < 2; ++t)
    {
        toneBtn[t].setButtonText (toneNames[t]);
        toneBtn[t].getProperties().set ("led", true);
        toneBtn[t].getProperties().set ("ledColour", "ff3d8bff");
        toneBtn[t].onClick = [this, t] { selTone = t; reattach(); updateButtons(); repaint(); };
        addAndMakeVisible (toneBtn[t]);
    }
    const char* partNames[4] = { "LOWER 1", "LOWER 2", "UPPER 1", "UPPER 2" };
    for (int i = 0; i < 4; ++i)
    {
        partBtn[i].setButtonText (partNames[i]);
        partBtn[i].getProperties().set ("led", true);
        partBtn[i].getProperties().set ("ledColour", "ff3d8bff");
        partBtn[i].onClick = [this, i] { selPartial = i; selTone = i >= 2 ? 0 : 1; reattach(); updateButtons(); repaint(); };
        addAndMakeVisible (partBtn[i]);

        onBtn[i].setButtonText (juce::String (i < 2 ? "L" : "U") + juce::String (i % 2 + 1));
        onBtn[i].getProperties().set ("led", true);
        onBtn[i].onClick = [this, i]
        {
            const juce::String id = i < 2 ? "LC_PMUTE" : "UC_PMUTE";
            setParamInt (id, paramInt (id) ^ (1 << (i % 2)));
            updateButtons();
        };
        addAndMakeVisible (onBtn[i]);
    }
    portaBtn.getProperties().set ("led", true);
    portaBtn.onClick = [this] { setParamInt ("P_PORTASW", paramInt ("P_PORTASW") ? 0 : 1); updateButtons(); };
    addAndMakeVisible (portaBtn);

    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); };
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Banco D-50 (.syx)", juce::File(), "*.syx;*.SYX;*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  juce::String msg;
                                  proc.importSyx (f, msg);
                                  status = msg; statusTicks = 120;
                                  refreshPrograms();
                              });
    };
    clearBtn.onClick = [this] { proc.clearImported(); status = "BANCO IMPORTATO SVUOTATO"; statusTicks = 90; refreshPrograms(); };
    for (auto* b : { &prevBtn, &nextBtn, &loadBtn, &clearBtn }) addAndMakeVisible (*b);

    programBox.onChange = [this]
    {
        const int id = programBox.getSelectedId();
        if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (programBox);
    addAndMakeVisible (meter);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffdcdcd8));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0d0d0e));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff3a3c40));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, C::ledBlue.withAlpha (.55f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, C::ledBlue.withAlpha (.18f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (.5f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, C::key);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, C::print);
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    proc.onProgramChanged = [this] { juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<LA91Panel> (this)] { if (sp) sp->refreshPrograms(); }); };
    refreshPrograms();
    reattach();
    updateButtons();
    setSize (baseW, baseH);
    startTimerHz (30);
}

LA91Panel::~LA91Panel()
{
    stopTimer();
    proc.onProgramChanged = nullptr;
    for (auto& f : faders) f->att.reset();
}

void LA91Panel::addGroup (const juce::String& title, Scope scope, const std::vector<la91::ParamDef>& defs, std::initializer_list<const char*> keys)
{
    Group g { title, scope, {}, {} };
    for (auto* k : keys)
    {
        auto f = std::make_unique<Fader>();
        f->def = la91::findDef (defs, k);
        f->scope = scope;
        f->slider.setSliderStyle (juce::Slider::LinearVertical);
        f->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        f->slider.setColour (juce::Slider::trackColourId, C::ledBlue);
        f->slider.setMouseDragSensitivity (220);
        auto* fp = f.get();
        f->slider.onDragStart = [this, fp] { touched = fp->def->longName; };
        f->slider.onValueChange = [this, fp]
        {
            touched = scopeTag (fp->scope) + " " + juce::String (fp->def->longName).toUpperCase();
            touchedValue = la91::formatValue (fp->def->fmt, (int) std::lround (fp->slider.getValue()));
            repaint (displayArea);
        };
        addAndMakeVisible (f->slider);
        g.faders.push_back (f.get());
        faders.push_back (std::move (f));
    }
    groups.push_back (std::move (g));
}

void LA91Panel::buildGroups()
{
    const auto& P = la91::partialDefs();
    const auto& Cm = la91::commonDefs();
    const auto& Pt = la91::patchDefs();
    // riga 1
    addGroup ("WG PITCH", Scope::partial, P, { "COARSE", "FINE", "KF", "LFOMODE", "ENVMODE", "BENDMODE" });
    addGroup ("WG WAVEFORM", Scope::partial, P, { "WF", "PCMWAVE", "PW", "PWVELO", "PWAFTER", "PWMSEL", "PWMDEPTH" });
    addGroup ("STRUCTURE", Scope::common, Cm, { "STRUCT", "PBAL" });
    addGroup ("PITCH MOD", Scope::common, Cm, { "PMLFOD", "PMLEVER", "PMAFTER" });
    addGroup ("PITCH ENV", Scope::common, Cm, { "PET1", "PET2", "PET3", "PET4", "PETKF", "PEL0", "PEL1", "PEL2", "PESUS", "PEEND", "PEVELO" });
    // riga 2
    addGroup ("TVF", Scope::partial, P, { "CUTOFF", "RESO", "TVFKF", "TVFBIAS", "TVFBLVL", "TVFDEPTH", "TVFVELO", "TVFDKF", "TVFLSEL", "TVFLDEP", "TVFAFTER" });
    addGroup ("TVF ENV", Scope::partial, P, { "TVFT1", "TVFT2", "TVFT3", "TVFT4", "TVFT5", "TVFTKF" });
    addGroup ("TVA ENV", Scope::partial, P, { "TVAT1", "TVAT2", "TVAT3", "TVAT4", "TVAT5", "TVATKF" });
    addGroup ("LFO-1", Scope::common, Cm, { "L1WF", "L1RATE", "L1DELAY", "L1SYNC" });
    // riga 3
    addGroup ("TVA", Scope::partial, P, { "LEVEL", "TVAVELO", "TVABIAS", "TVABLVL", "TVALSEL", "TVALDEP", "TVAAFTER" });
    addGroup ("TVF ENV", Scope::partial, P, { "TVFL1", "TVFL2", "TVFL3", "TVFSUS", "TVFEND" });
    addGroup ("TVA ENV", Scope::partial, P, { "TVAL1", "TVAL2", "TVAL3", "TVASUS", "TVAEND", "TVAT1V" });
    addGroup ("LFO-2", Scope::common, Cm, { "L2WF", "L2RATE", "L2DELAY", "L2SYNC" });
    // riga 4
    addGroup ("LFO-3", Scope::common, Cm, { "L3WF", "L3RATE", "L3DELAY", "L3SYNC" });
    addGroup ("LOW EQ", Scope::common, Cm, { "EQLF", "EQLG" });
    addGroup ("HIGH EQ", Scope::common, Cm, { "EQHF", "EQHQ", "EQHG" });
    addGroup ("CHORUS", Scope::common, Cm, { "CHTYPE", "CHRATE", "CHDEPTH", "CHBAL" });
    addGroup ("PATCH", Scope::patch, Pt, { "KEYMODE", "SPLIT", "UKEY", "UFINE", "LKEY", "LFINE", "TONEBAL", "BENDER", "AFTBEND",
                                           "PORTAMODE", "PORTATIME", "HOLDMODE", "OUTMODE", "REVTYPE", "REVBAL", "VOLUME" });
}

la91::Block LA91Panel::partialBlock() const
{
    const Block b[4] = { Block::L1, Block::L2, Block::U1, Block::U2 };
    return b[juce::jlimit (0, 3, selPartial)];
}

juce::String LA91Panel::scopeTag (Scope s) const
{
    switch (s)
    {
        case Scope::partial: return juce::String (selPartial >= 2 ? "UPPER " : "LOWER ") + juce::String (selPartial % 2 + 1);
        case Scope::common:  return selTone == 0 ? "UPPER" : "LOWER";
        case Scope::patch:   return "PATCH";
    }
    return {};
}

void LA91Panel::reattach()
{
    for (auto& f : faders)
    {
        f->att.reset();
        const Block b = f->scope == Scope::partial ? partialBlock() : f->scope == Scope::common ? commonBlock() : Block::P;
        const auto id = la91::paramID (b, *f->def);
        f->slider.setDoubleClickReturnValue (true, f->def->def);
        f->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.state, id, f->slider);
        f->slider.setTooltip (juce::String (f->def->longName));
    }
    touched.clear();
}

int LA91Panel::paramInt (const juce::String& id) const
{
    if (auto* p = dynamic_cast<juce::AudioParameterInt*> (proc.state.getParameter (id))) return p->get();
    return 0;
}

void LA91Panel::setParamInt (const juce::String& id, int v)
{
    if (auto* p = dynamic_cast<juce::AudioParameterInt*> (proc.state.getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
        p->endChangeGesture();
    }
}

void LA91Panel::updateButtons()
{
    for (int t = 0; t < 2; ++t) toneBtn[t].setToggleState (selTone == t, juce::dontSendNotification);
    for (int i = 0; i < 4; ++i)
    {
        partBtn[i].setToggleState (selPartial == i, juce::dontSendNotification);
        const int mute = paramInt (i < 2 ? "LC_PMUTE" : "UC_PMUTE");
        onBtn[i].setToggleState (((mute >> (i % 2)) & 1) != 0, juce::dontSendNotification);
    }
    portaBtn.setToggleState (paramInt ("P_PORTASW") != 0, juce::dontSendNotification);
}

void LA91Panel::refreshPrograms()
{
    programBox.clear (juce::dontSendNotification);
    const int nf = proc.numFactory();
    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        if (i == 0) programBox.addSectionHeading ("FACTORY MBSTUDIO91");
        if (i == nf) programBox.addSectionHeading ("BANCO IMPORTATO (.SYX)");
        programBox.addItem (proc.getProgramName (i), i + 1);
    }
    programBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    touched.clear();
    updateButtons();
    repaint();
}

void LA91Panel::timerCallback()
{
    meter.setLevel (std::pow (proc.synth.getOutputLevel(), .5f));
    updateButtons();
    if (statusTicks > 0 && --statusTicks == 0) status.clear();
    repaint (displayArea);
    repaint (diagramArea);
}

// ------------------------------------------------------------------------------------------------ layout
void LA91Panel::resized()
{
    // intestazione
    displayArea = { 330, 14, 470, 86 };
    programBox.setBounds (816, 16, 316, 28);
    prevBtn.setBounds (816, 56, 46, 30);
    nextBtn.setBounds (868, 56, 46, 30);
    loadBtn.setBounds (930, 56, 110, 30);
    clearBtn.setBounds (1046, 56, 86, 30);
    meter.setBounds (1310, 18, 14, 82);

    // colonna di sinistra (sezione di selezione)
    leftArea = { 14, rowY[0], 164, rowY[3] + rowH - rowY[0] };
    int y = rowY[0] + 24;
    for (int t = 0; t < 2; ++t) toneBtn[t].setBounds (14 + t * 84, y, 80, 30);
    y += 64;
    for (int i = 0; i < 4; ++i) partBtn[i].setBounds (14 + (i % 2) * 84, y + (i / 2) * 36, 80, 30);
    y += 100;
    for (int i = 0; i < 4; ++i) onBtn[i].setBounds (14 + i * 41, y, 38, 30);
    y += 60;
    portaBtn.setBounds (14, y, 80, 30);
    diagramArea = { 14, y + 52, 164, rowY[3] + rowH - (y + 52) };

    // righe dei fader
    const int rows[4][6] = { { 0, 1, 2, 3, 4, -1 }, { 5, 6, 7, 8, -1, -1 }, { 9, 10, 11, 12, -1, -1 }, { 13, 14, 15, 16, 17, -1 } };
    for (int r = 0; r < 4; ++r)
    {
        int x = mainX;
        for (int k = 0; k < 6 && rows[r][k] >= 0; ++k)
        {
            auto& g = groups[(size_t) rows[r][k]];
            const int w = (int) g.faders.size() * pitch;
            g.area = { x, rowY[r], w, rowH };
            for (size_t i = 0; i < g.faders.size(); ++i)
                g.faders[i]->slider.setBounds (x + (int) i * pitch, rowY[r] + 48, pitch, rowH - 54);
            x += w + groupGap;
        }
    }
    keyboard.setBounds (14, 848, baseW - 28, 60);
    keyboard.setKeyWidth ((float) (baseW - 28) / 49.f);
}

// ------------------------------------------------------------------------------------------------ disegno
void LA91Panel::paint (juce::Graphics& g)
{
    ui::paintChassis (g, getLocalBounds());

    // marchio e sottotitolo
    ui::paintBrand (g, { 18.f, 16.f, 300.f, 34.f }, "LA-91");
    ui::paintPrinted (g, { 18.f, 56.f, 300.f, 16.f }, "LINEAR SYNTHESIZER  -  PROGRAMMER", 14.f, juce::Justification::centredLeft, C::printDim);
    ui::paintPrinted (g, { 18.f, 76.f, 300.f, 14.f }, "ENGINE BASED ON DSYNKANT PARAMETER MAP", 11.f, juce::Justification::centredLeft, C::printDim.withAlpha (.7f));

    // display
    const auto d = displayArea.toFloat();
    ui::paintRecess (g, d);
    {
        const auto in = d.reduced (12.f, 8.f);
        const int cur = proc.getCurrentProgram();
        g.setColour (C::print);
        g.setFont (ui::Fonts::display (28.f));
        g.drawText (proc.programLabel (cur) + "  " + proc.currentPatchName(), in.withHeight (30.f), juce::Justification::centredLeft, true);
        g.setFont (ui::Fonts::label (14.f));
        g.setColour (C::printDim);
        g.drawText ("U: " + proc.currentToneName (true) + "     L: " + proc.currentToneName (false), in.withY (in.getY() + 32.f).withHeight (18.f), juce::Justification::centredLeft, true);
        const juce::String line3 = status.isNotEmpty() ? status : touched.isNotEmpty() ? touched + "  =  " + touchedValue : "VOICES " + juce::String (proc.synth.activeVoices()) + " / 16";
        g.setColour (status.isNotEmpty() ? C::ledAmber : C::print);
        g.setFont (ui::Fonts::label (14.f));
        g.drawText (line3.toUpperCase(), in.withY (in.getY() + 52.f).withHeight (18.f), juce::Justification::centredLeft, true);
        ui::paintLed (g, { d.getRight() - 14.f, d.getY() + 12.f }, 3.5f, C::ledGreen, juce::jmin (1.f, proc.synth.getOutputLevel() * 3.f));
    }
    ui::paintPrinted (g, { 1140.f, 16.f, 160.f, 28.f }, "PROGRAM", 16.f, juce::Justification::centredLeft, C::printDim);
    ui::paintPrinted (g, { 1140.f, 56.f, 160.f, 30.f }, "BANK  .SYX", 16.f, juce::Justification::centredLeft, C::printDim);
    ui::paintPrinted (g, { 1270.f, 86.f, 34.f, 14.f }, "OUT", 11.f, juce::Justification::centredRight, C::printDim);

    // separatore sotto l'intestazione
    ui::paintEngravedLine (g, { 14.f, 112.f }, { (float) baseW - 14.f, 112.f });

    // colonna di sinistra
    auto chipAt = [&] (float x, float y, const juce::String& t) { ui::paintChip (g, { x, y, ui::chipWidth (t, 16.f), 16.f }, t, 14.f); };
    chipAt (14.f, (float) rowY[0], "COMMON SELECT");
    chipAt (14.f, (float) rowY[0] + 64.f, "PARTIAL SELECT");
    chipAt (14.f, (float) rowY[0] + 164.f, "PARTIAL ON");
    chipAt (14.f, (float) rowY[0] + 224.f, "PORTAMENTO");
    ui::paintPrinted (g, { 100.f, (float) rowY[0] + 252.f, 80.f, 30.f }, "SWITCH", 11.f, juce::Justification::centredLeft, C::printDim);
    chipAt (14.f, (float) diagramArea.getY() - 22.f, "STRUCTURE");
    paintDiagram (g, diagramArea.toFloat());
    ui::paintEngravedLine (g, { 184.f, (float) rowY[0] }, { 184.f, (float) (rowY[3] + rowH) });

    // gruppi di fader
    for (auto& gr : groups)
    {
        const auto a = gr.area.toFloat();
        const float cw = ui::chipWidth (gr.title, 16.f);
        ui::paintChip (g, { a.getX(), a.getY(), cw, 16.f }, gr.title, 14.f);
        const auto tag = scopeTag (gr.scope);
        g.setFont (ui::Fonts::label (11.f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), tag) + 6.f;
        const float lineEnd = a.getRight() - 4.f - (gr.scope == Scope::patch ? 0.f : tw + 12.f);
        if (lineEnd > a.getX() + cw + 6.f) ui::paintEngravedLine (g, { a.getX() + cw + 4.f, a.getY() + 8.f }, { lineEnd, a.getY() + 8.f });
        if (gr.scope != Scope::patch && a.getRight() - tw > a.getX() + cw)
        {
            g.setColour (C::printDim);
            g.drawText (tag, juce::Rectangle<float> (a.getRight() - tw, a.getY() + 1.f, tw, 14.f), juce::Justification::centredRight, false);
            // LED di ambito: blu = partial selezionato, verde = tone (common)
            ui::paintLed (g, { a.getRight() - tw - 6.f, a.getY() + 8.f }, 2.5f, gr.scope == Scope::partial ? C::ledBlue : C::ledGreen, 1.f);
        }
        for (size_t i = 0; i < gr.faders.size(); ++i)
        {
            const auto lab = juce::StringArray::fromLines (gr.faders[i]->def->label);
            const juce::Rectangle<float> col (a.getX() + (float) i * pitch, a.getY() + 20.f, (float) pitch, 26.f);
            g.setColour (C::print);
            g.setFont (ui::Fonts::label (11.5f));
            if (lab.size() == 1) g.drawText (lab[0], col.withTrimmedTop (13.f), juce::Justification::centredTop, false);
            else for (int l = 0; l < juce::jmin (2, lab.size()); ++l) g.drawText (lab[l], col.withY (col.getY() + 13.f * (float) l).withHeight (13.f), juce::Justification::centredTop, false);
        }
    }
    // logo impresso in basso a destra (sopra la tastiera)
    ui::paintLogo (g, { (float) baseW - 260.f, 824.f, 246.f, 16.f }, false, ui::Imprint::debossed, juce::Justification::centredRight);
}

void LA91Panel::paintDiagram (juce::Graphics& g, juce::Rectangle<float> r)
{
    ui::paintRecess (g, r);
    auto in = r.reduced (8.f, 8.f);
    const float rowHh = 96.f;
    for (int t = 0; t < 2; ++t)
    {
        // t = 0 UPPER in alto
        const int tone = t;
        const auto area = juce::Rectangle<float> (in.getX(), in.getY() + rowHh * (float) t, in.getWidth(), rowHh).reduced (0.f, 4.f);
        const int st = paramInt (tone == 0 ? "UC_STRUCT" : "LC_STRUCT");
        const int mute = paramInt (tone == 0 ? "UC_PMUTE" : "LC_PMUTE");
        const bool pcm1 = st == 2 || st == 3 || st == 5 || st == 6, pcm2 = st == 4 || st == 5 || st == 6;
        const bool ring = st == 1 || st == 3 || st == 4 || st == 6;
        const bool sounds = proc.synth.activeVoices() >= 0;
        (void) sounds;
        g.setFont (ui::Fonts::display (15.f));
        g.setColour (selTone == tone ? C::print : C::printDim);
        g.drawText (tone == 0 ? "UPPER" : "LOWER", area.withHeight (16.f), juce::Justification::centredLeft, false);
        g.setFont (ui::Fonts::label (11.f));
        g.setColour (C::printDim);
        g.drawText ("STRUCT " + juce::String (st + 1), area.withHeight (16.f), juce::Justification::centredRight, false);
        const float bw = 50.f, bh = 22.f;
        for (int i = 0; i < 2; ++i)
        {
            const int panelIdx = (tone == 0 ? 2 : 0) + i;
            const juce::Rectangle<float> box (area.getX(), area.getY() + 22.f + (float) i * (bh + 8.f), bw, bh);
            const bool sel = selPartial == panelIdx;
            const bool on = (mute >> i) & 1;
            const juce::String txt = juce::String ("P") + juce::String (i + 1) + " " + ((i == 0 ? pcm1 : pcm2) ? "PCM" : "SYN");
            if (sel) ui::paintChip (g, box, txt, 14.f);
            else
            {
                g.setColour (C::printDim); g.drawRect (box, 1.f);
                g.setFont (ui::Fonts::display (14.f));
                g.drawText (txt, box, juce::Justification::centred, false);
            }
            ui::paintLed (g, { box.getRight() + 8.f, box.getCentreY() }, 3.f, C::ledGreen, on ? 1.f : 0.f);
            g.setColour (C::tick);
            g.drawLine (box.getRight() + 14.f, box.getCentreY(), area.getX() + 92.f, box.getCentreY(), 1.f);
        }
        const juce::Point<float> sum (area.getX() + 104.f, area.getY() + 22.f + bh + 4.f);
        g.setColour (C::tick);
        g.drawLine (area.getX() + 92.f, area.getY() + 22.f + bh * .5f, area.getX() + 92.f, area.getY() + 30.f + bh * 1.5f, 1.f);
        g.drawLine (area.getX() + 92.f, sum.y, sum.x - 9.f, sum.y, 1.f);
        g.drawEllipse (juce::Rectangle<float> (18.f, 18.f).withCentre (sum), 1.2f);
        g.setFont (ui::Fonts::display (13.f));
        g.setColour (ring ? C::ledAmber : C::print);
        g.drawText (ring ? "RM" : "+", juce::Rectangle<float> (18.f, 18.f).withCentre (sum).translated (0, 1.f), juce::Justification::centred, false);
        g.setColour (C::tick);
        g.drawLine (sum.x + 9.f, sum.y, area.getRight() - 2.f, sum.y, 1.f);
        g.drawArrow ({ sum.x + 9.f, sum.y, area.getRight() - 2.f, sum.y }, 1.f, 6.f, 6.f);
    }
    // riepilogo della patch sotto lo schema
    auto info = juce::Rectangle<float> (in.getX(), in.getY() + rowHh * 2.f + 6.f, in.getWidth(), in.getBottom() - (in.getY() + rowHh * 2.f + 6.f));
    ui::paintEngravedLine (g, { info.getX(), info.getY() }, { info.getRight(), info.getY() });
    info.removeFromTop (8.f);
    const auto* P = &la91::patchDefs();
    auto line = [&] (const char* label, const char* key)
    {
        const auto* d = la91::findDef (*P, key);
        auto row = info.removeFromTop (17.f);
        g.setFont (ui::Fonts::label (11.5f));
        g.setColour (C::printDim);
        g.drawText (label, row, juce::Justification::centredLeft, false);
        g.setColour (C::print);
        g.drawText (la91::formatValue (d->fmt, paramInt ("P_" + juce::String (key))), row, juce::Justification::centredRight, false);
    };
    line ("KEY MODE", "KEYMODE");
    line ("SPLIT", "SPLIT");
    line ("REVERB", "REVTYPE");
    line ("OUTPUT", "OUTMODE");
}

// ================================================================================================ editor
LA91Editor::LA91Editor (LA91Processor& p) : AudioProcessorEditor (p), proc (p), panel (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (panel);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) LA91Panel::baseW / LA91Panel::baseH);
    setResizeLimits (LA91Panel::baseW / 2, LA91Panel::baseH / 2, LA91Panel::baseW * 2, LA91Panel::baseH * 2);
    setSize ((int) (LA91Panel::baseW * .9), (int) (LA91Panel::baseH * .9));
}

LA91Editor::~LA91Editor() { setLookAndFeel (nullptr); }

void LA91Editor::paint (juce::Graphics& g) { ui::paintChassis (g, getLocalBounds()); }

void LA91Editor::resized()
{
    const float s = (float) getWidth() / (float) LA91Panel::baseW;
    panel.setBounds (0, 0, LA91Panel::baseW, LA91Panel::baseH);
    panel.setTransform (juce::AffineTransform::scale (s));
}
