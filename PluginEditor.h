#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class ChronoSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChronoSynthAudioProcessorEditor (ChronoSynthAudioProcessor&);
    ~ChronoSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct KnobControl
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
        int row = 0, col = 0;
    };

    struct ChoiceControl
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<ComboAttachment> attachment;
        int row = 0, col = 0;
    };

    void addKnob (const juce::String& id, const juce::String& text, int row, int col);
    void addChoice (const juce::String& id, const juce::String& text, const juce::StringArray& items, int row, int col);

    ChronoSynthAudioProcessor& chronoProcessor;
    juce::ComboBox presetBox;
    std::vector<std::unique_ptr<KnobControl>> knobs;
    std::vector<std::unique_ptr<ChoiceControl>> choices;

    static constexpr int leftMargin = 100;
    static constexpr int topMargin  = 64;
    static constexpr int numRows    = 4;
    static constexpr int numCols    = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChronoSynthAudioProcessorEditor)
};
