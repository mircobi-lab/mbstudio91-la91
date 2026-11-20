#include "PluginProcessor.h"
#include "PluginEditor.h"

LA91Processor::LA91Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "LA91", la91::createLayout())
{
    reader.attach (state);
    setCurrentProgram (0);
}

bool LA91Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void LA91Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare (sampleRate, juce::jmax (samplesPerBlock, 32));
    keyboardState.reset();
}

void LA91Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isProgramChange()) { pendingProgram.store (m.getProgramChangeNumber()); triggerAsyncUpdate(); }
    }
    reader.read (patch);
    buffer.clear();
    synth.process (buffer, midi, patch);
}

void LA91Processor::handleAsyncUpdate()
{
    const int p = pendingProgram.exchange (-1);
    if (p >= 0 && p < getNumPrograms()) setCurrentProgram (p);
}

// ---------------------------------------------------------------------------------------------- programmi
int LA91Processor::getNumPrograms() { return numFactory() + numImported(); }

const la91::RawPatch* LA91Processor::programAt (int index) const
{
    if (index < 0) return nullptr;
    if (index < numFactory()) return &la91::factoryBank()[(size_t) index];
    index -= numFactory();
    if (index < numImported()) return &imported[(size_t) index];
    return nullptr;
}

juce::String LA91Processor::programLabel (int index)
{
    if (index < numFactory()) return "F-" + juce::String (index + 1).paddedLeft ('0', 2);
    const int i = index - numFactory();
    // numerazione stile D-50: banco 1-8, patch 1-8
    return "I-" + juce::String (i / 8 + 1) + juce::String (i % 8 + 1);
}

const juce::String LA91Processor::getProgramName (int index)
{
    if (auto* p = programAt (index)) return programLabel (index) + " " + p->name();
    return {};
}

void LA91Processor::setCurrentProgram (int index)
{
    auto* p = programAt (index);
    if (p == nullptr) return;
    currentProgram = index;
    names = *p;
    la91::applyRawPatch (state, *p);
    if (onProgramChanged) onProgramChanged();
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

bool LA91Processor::importSyx (const juce::File& file, juce::String& message)
{
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb)) { message = "FILE NON LEGGIBILE"; return false; }
    auto patches = la91::parseSyx (mb);
    if (patches.empty()) { message = "NESSUNA PATCH D-50 NEL FILE"; return false; }
    imported = std::move (patches);
    message = juce::String (imported.size()) + " PATCH IMPORTATE";
    setCurrentProgram (numFactory());
    return true;
}

void LA91Processor::clearImported()
{
    imported.clear();
    if (currentProgram >= numFactory()) setCurrentProgram (0);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

// ---------------------------------------------------------------------------------------------- stato
void LA91Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = state.copyState();
    juce::ValueTree extra ("LA91Extra");
    extra.setProperty ("program", currentProgram, nullptr);
    extra.setProperty ("names", juce::Base64::toBase64 (names.bytes.data(), names.bytes.size()), nullptr);
    juce::MemoryBlock bank;
    for (auto& p : imported) bank.append (p.bytes.data(), p.bytes.size());
    extra.setProperty ("imported", juce::Base64::toBase64 (bank.getData(), bank.getSize()), nullptr);
    st.removeChild (st.getChildWithName ("LA91Extra"), nullptr);
    st.appendChild (extra, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void LA91Processor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (! xml || ! xml->hasTagName (state.state.getType())) return;
    auto st = juce::ValueTree::fromXml (*xml);
    auto extra = st.getChildWithName ("LA91Extra");
    if (extra.isValid())
    {
        juce::MemoryOutputStream mo;
        if (juce::Base64::convertFromBase64 (mo, extra["names"].toString()) && mo.getDataSize() == names.bytes.size())
            std::memcpy (names.bytes.data(), mo.getData(), names.bytes.size());
        juce::MemoryOutputStream bo;
        imported.clear();
        if (juce::Base64::convertFromBase64 (bo, extra["imported"].toString()))
        {
            const auto* b = static_cast<const uint8_t*> (bo.getData());
            for (size_t off = 0; off + la91::patchBytes <= bo.getDataSize(); off += la91::patchBytes)
            {
                la91::RawPatch p; std::memcpy (p.bytes.data(), b + off, la91::patchBytes); imported.push_back (p);
            }
        }
        currentProgram = juce::jlimit (0, juce::jmax (0, getNumPrograms() - 1), (int) extra["program"]);
        st.removeChild (extra, nullptr);
    }
    state.replaceState (st);
    if (onProgramChanged) onProgramChanged();
}

juce::AudioProcessorEditor* LA91Processor::createEditor() { return new LA91Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LA91Processor(); }
