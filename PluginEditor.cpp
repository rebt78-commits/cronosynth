#include "PluginEditor.h"
#include "Presets.h"

ChronoSynthAudioProcessorEditor::ChronoSynthAudioProcessorEditor (ChronoSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), chronoProcessor (p)
{
    const juce::StringArray waves { "Saw", "Square", "Triangle", "Sine" };

    // Fila 0: osciladores
    addChoice ("osc1Wave",   "Osc 1", waves, 0, 0);
    addChoice ("osc2Wave",   "Osc 2", waves, 0, 1);
    addKnob   ("osc2Semi",   "Osc2 Semitonos", 0, 2);
    addKnob   ("osc2Detune", "Osc2 Detune",    0, 3);
    addKnob   ("oscMix",     "Mezcla",         0, 4);
    addKnob   ("subLevel",   "Sub",            0, 5);

    // Fila 1: filtro y LFO
    addKnob ("cutoff",    "Cutoff",     1, 0);
    addKnob ("resonance", "Resonancia", 1, 1);
    addKnob ("filterEnv", "Env Filtro", 1, 2);
    addKnob ("fDecay",    "Decay Filtro", 1, 3);
    addKnob ("lfoRate",   "LFO Vel",    1, 4);
    addKnob ("lfoPitch",  "LFO Pitch",  1, 5);
    addKnob ("lfoCutoff", "LFO Cutoff", 1, 6);

    // Fila 2: amplificador
    addKnob ("attack",  "Attack",  2, 0);
    addKnob ("decay",   "Decay",   2, 1);
    addKnob ("sustain", "Sustain", 2, 2);
    addKnob ("release", "Release", 2, 3);
    addKnob ("volume",  "Volumen", 2, 4);

    // Fila 3: efectos
    addKnob ("delayTime",     "Delay ms",   3, 0);
    addKnob ("delayFeedback", "Feedback",   3, 1);
    addKnob ("delayMix",      "Delay Mezcla", 3, 2);
    addKnob ("reverbSize",    "Reverb Tamano", 3, 3);
    addKnob ("reverbMix",     "Reverb Mezcla", 3, 4);

    // Selector de presets
    const auto& presets = getPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox.addItem (presets[i].name, (int) i + 1);

    presetBox.setTextWhenNothingSelected ("Presets...");
    presetBox.onChange = [this]
    {
        chronoProcessor.loadPreset (presetBox.getSelectedItemIndex());
    };
    addAndMakeVisible (presetBox);

    setSize (820, 580);
}

void ChronoSynthAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& text, int row, int col)
{
    auto k = std::make_unique<KnobControl>();
    k->row = row;
    k->col = col;

    k->slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);

    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);

    k->attachment = std::make_unique<SliderAttachment> (chronoProcessor.apvts, id, k->slider);
    knobs.push_back (std::move (k));
}

void ChronoSynthAudioProcessorEditor::addChoice (const juce::String& id, const juce::String& text,
                                                 const juce::StringArray& items, int row, int col)
{
    auto c = std::make_unique<ChoiceControl>();
    c->row = row;
    c->col = col;

    c->box.addItemList (items, 1);
    c->label.setText (text, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (c->box);
    addAndMakeVisible (c->label);

    c->attachment = std::make_unique<ComboAttachment> (chronoProcessor.apvts, id, c->box);
    choices.push_back (std::move (c));
}

void ChronoSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12141a));

    g.setColour (juce::Colour (0xff4fd1ff));
    g.setFont (26.0f);
    g.drawText ("ChronoSynth", 16, 12, 300, 36, juce::Justification::centredLeft);

    const char* names[numRows] = { "OSCILADORES", "FILTRO / LFO", "AMPLIFICADOR", "EFECTOS" };
    const int rowH = (getHeight() - topMargin - 10) / numRows;

    g.setFont (13.0f);
    for (int r = 0; r < numRows; ++r)
    {
        const int y = topMargin + r * rowH;
        g.setColour (juce::Colour (0xff2a2e3a));
        g.drawHorizontalLine (y, 10.0f, (float) getWidth() - 10.0f);
        g.setColour (juce::Colour (0xffb8c0d0));
        g.drawText (names[r], 12, y + 6, leftMargin - 16, 20, juce::Justification::centredLeft);
    }
}

void ChronoSynthAudioProcessorEditor::resized()
{
    presetBox.setBounds (getWidth() - 240, 14, 220, 28);

    const int cellW = (getWidth() - leftMargin - 10) / numCols;
    const int rowH  = (getHeight() - topMargin - 10) / numRows;

    for (auto& k : knobs)
    {
        const int x = leftMargin + k->col * cellW;
        const int y = topMargin + k->row * rowH;
        k->label.setBounds (x, y + 4, cellW, 18);
        k->slider.setBounds (x + 4, y + 22, cellW - 8, rowH - 26);
    }

    for (auto& c : choices)
    {
        const int x = leftMargin + c->col * cellW;
        const int y = topMargin + c->row * rowH;
        c->label.setBounds (x, y + 4, cellW, 18);
        c->box.setBounds (x + 8, y + 34, cellW - 16, 26);
    }
}
