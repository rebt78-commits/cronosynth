#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ChronoSynthAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&layout] (const juce::String& id, const juce::String& name,
                               float minV, float maxV, float def, float skew = 1.0f)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (minV, maxV, 0.0f, skew), def));
    };

    const juce::StringArray waves { "Saw", "Square", "Triangle", "Sine" };

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "osc1Wave", 1 }, "Osc 1", waves, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "osc2Wave", 1 }, "Osc 2", waves, 0));
    layout.add (std::make_unique<juce::AudioParameterInt>    (juce::ParameterID { "osc2Semi", 1 }, "Osc 2 Semitonos", -24, 24, 0));

    addFloat ("osc2Detune",    "Osc 2 Detune",  -50.0f,  50.0f,   7.0f);
    addFloat ("oscMix",        "Mezcla Osc",      0.0f,   1.0f,   0.5f);
    addFloat ("subLevel",      "Sub",             0.0f,   1.0f,   0.0f);

    addFloat ("cutoff",        "Cutoff",         20.0f, 20000.0f, 2000.0f, 0.3f);
    addFloat ("resonance",     "Resonancia",      0.0f,   0.95f,  0.2f);
    addFloat ("filterEnv",     "Env Filtro",      0.0f,   1.0f,   0.4f);
    addFloat ("fDecay",        "Decay Filtro",    0.01f,  4.0f,   0.4f, 0.4f);

    addFloat ("lfoRate",       "LFO Vel",         0.1f,  20.0f,   5.0f, 0.5f);
    addFloat ("lfoPitch",      "LFO Pitch",       0.0f,   2.0f,   0.0f);
    addFloat ("lfoCutoff",     "LFO Cutoff",      0.0f,   1.0f,   0.0f);

    addFloat ("attack",        "Attack",          0.001f, 5.0f,   0.01f, 0.3f);
    addFloat ("decay",         "Decay",           0.01f,  5.0f,   0.3f,  0.4f);
    addFloat ("sustain",       "Sustain",         0.0f,   1.0f,   0.7f);
    addFloat ("release",       "Release",         0.01f,  8.0f,   0.3f,  0.4f);

    addFloat ("delayTime",     "Delay Tiempo",   10.0f, 1000.0f, 375.0f);
    addFloat ("delayFeedback", "Delay Feedback",  0.0f,   0.9f,   0.35f);
    addFloat ("delayMix",      "Delay Mezcla",    0.0f,   1.0f,   0.2f);

    addFloat ("reverbSize",    "Reverb Tamano",   0.0f,   1.0f,   0.4f);
    addFloat ("reverbMix",     "Reverb Mezcla",   0.0f,   1.0f,   0.15f);

    addFloat ("volume",        "Volumen",         0.0f,   1.0f,   0.7f);

    return layout;
}

//==============================================================================
ChronoSynthAudioProcessor::ChronoSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new SynthVoice (apvts));

    synth.addSound (new SynthSound());

    pDelayTime     = apvts.getRawParameterValue ("delayTime");
    pDelayFeedback = apvts.getRawParameterValue ("delayFeedback");
    pDelayMix      = apvts.getRawParameterValue ("delayMix");
    pReverbSize    = apvts.getRawParameterValue ("reverbSize");
    pReverbMix     = apvts.getRawParameterValue ("reverbMix");
    pVolume        = apvts.getRawParameterValue ("volume");
}

void ChronoSynthAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);

    delayLength = (int) (sampleRate * 1.2) + 8;
    for (auto& b : delayBuffer)
        b.assign ((size_t) delayLength, 0.0f);

    delayWrite = 0;
    delaySmoothed = pDelayTime->load() * 0.001f * (float) sampleRate;
    lastGain = pVolume->load();

    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

bool ChronoSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChronoSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, numSamples);

    // ---- Delay ----
    if (delayLength > 0 && numChannels > 0)
    {
        const float fb   = pDelayFeedback->load();
        const float dMix = pDelayMix->load();
        const float target = juce::jlimit (1.0f, (float) (delayLength - 4),
                                           pDelayTime->load() * 0.001f * (float) currentSampleRate);

        for (int n = 0; n < numSamples; ++n)
        {
            delaySmoothed += 0.0005f * (target - delaySmoothed);

            float readPos = (float) delayWrite - delaySmoothed;
            if (readPos < 0.0f) readPos += (float) delayLength;

            int i0 = (int) readPos;
            if (i0 >= delayLength) i0 -= delayLength;
            const int i1 = (i0 + 1) % delayLength;
            const float frac = readPos - (float) i0;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                auto& buf = delayBuffer[ch];

                const float in = data[n];
                const float d  = buf[(size_t) i0] + frac * (buf[(size_t) i1] - buf[(size_t) i0]);

                buf[(size_t) delayWrite] = in + d * fb;
                data[n] = in + d * dMix;
            }

            delayWrite = (delayWrite + 1) % delayLength;
        }
    }

    // ---- Reverb ----
    if (buffer.getNumChannels() >= 2)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = pReverbSize->load();
        rp.damping    = 0.5f;
        rp.wetLevel   = pReverbMix->load() * 0.5f;
        rp.dryLevel   = 1.0f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
    }

    // ---- Volumen y limitador suave ----
    const float gain = pVolume->load();
    buffer.applyGainRamp (0, numSamples, lastGain, gain);
    lastGain = gain;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int n = 0; n < numSamples; ++n)
            data[n] = std::tanh (data[n]);
    }
}

//==============================================================================
void ChronoSynthAudioProcessor::loadPreset (int index)
{
    const auto& presets = getPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    for (const auto& pp : presets[(size_t) index].params)
        if (auto* p = apvts.getParameter (pp.id))
            p->setValueNotifyingHost (p->convertTo0to1 (pp.value));
}

juce::AudioProcessorEditor* ChronoSynthAudioProcessor::createEditor()
{
    return new ChronoSynthAudioProcessorEditor (*this);
}

void ChronoSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChronoSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChronoSynthAudioProcessor();
}
