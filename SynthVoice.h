#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <atomic>

//==============================================================================
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
// Filtro paso bajo resonante de 4 polos (estilo Moog), version sencilla.
class MoogLadder
{
public:
    void reset()
    {
        x1 = y1 = y2 = y3 = y4 = 0.0f;
        o0 = o1 = o2 = o3 = 0.0f;
    }

    void setParams (float cutoffHz, float resonance, float sampleRate)
    {
        const float fc = juce::jlimit (0.001f, 0.7f, 2.0f * cutoffHz / sampleRate);
        p = fc * (1.8f - 0.8f * fc);
        k = 2.0f * p - 1.0f;
        const float t1 = (1.0f - p) * 1.386249f;
        const float t2 = 12.0f + t1 * t1;
        r = resonance * (t2 + 6.0f * t1) / (t2 - 6.0f * t1);
    }

    float process (float in)
    {
        const float x = in - r * y4;

        y1 = x  * p + o0 * p - k * y1;
        y2 = y1 * p + o1 * p - k * y2;
        y3 = y2 * p + o2 * p - k * y3;
        y4 = y3 * p + o3 * p - k * y4;

        y4 = juce::jlimit (-2.0f, 2.0f, y4);
        y4 = y4 - (y4 * y4 * y4) / 6.0f;

        o0 = x; o1 = y1; o2 = y2; o3 = y3;
        return y4;
    }

private:
    float x1 = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0;
    float o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    float p = 0.1f, k = 0.0f, r = 0.0f;
};

//==============================================================================
class SynthVoice : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice (juce::AudioProcessorValueTreeState& state)
    {
        pOsc1Wave    = state.getRawParameterValue ("osc1Wave");
        pOsc2Wave    = state.getRawParameterValue ("osc2Wave");
        pOsc2Semi    = state.getRawParameterValue ("osc2Semi");
        pOsc2Detune  = state.getRawParameterValue ("osc2Detune");
        pOscMix      = state.getRawParameterValue ("oscMix");
        pSubLevel    = state.getRawParameterValue ("subLevel");
        pCutoff      = state.getRawParameterValue ("cutoff");
        pResonance   = state.getRawParameterValue ("resonance");
        pFilterEnv   = state.getRawParameterValue ("filterEnv");
        pFDecay      = state.getRawParameterValue ("fDecay");
        pAttack      = state.getRawParameterValue ("attack");
        pDecay       = state.getRawParameterValue ("decay");
        pSustain     = state.getRawParameterValue ("sustain");
        pRelease     = state.getRawParameterValue ("release");
        pLfoRate     = state.getRawParameterValue ("lfoRate");
        pLfoPitch    = state.getRawParameterValue ("lfoPitch");
        pLfoCutoff   = state.getRawParameterValue ("lfoCutoff");
    }

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*> (s) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int pitchWheelPos) override
    {
        noteHz    = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        level     = velocity;
        bendSemis = ((float) pitchWheelPos - 8192.0f) / 8192.0f * 2.0f;
        phase1 = phase2 = phaseSub = 0.0f;

        const double sr = getSampleRate();
        ampEnv.setSampleRate (sr);
        filtEnv.setSampleRate (sr);
        updateEnvelopes();

        if (! ampEnv.isActive())
            filter.reset();

        ampEnv.noteOn();
        filtEnv.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            filtEnv.noteOff();
        }
        else
        {
            ampEnv.reset();
            filtEnv.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int newValue) override
    {
        bendSemis = ((float) newValue - 8192.0f) / 8192.0f * 2.0f;
    }

    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! ampEnv.isActive())
        {
            if (isVoiceActive())
                clearCurrentNote();
            return;
        }

        updateEnvelopes();

        const float sr = (float) getSampleRate();
        const int   w1 = (int) pOsc1Wave->load();
        const int   w2 = (int) pOsc2Wave->load();
        const float detuneRatio = std::pow (2.0f, (pOsc2Semi->load() + pOsc2Detune->load() / 100.0f) / 12.0f);
        const float oscMix   = pOscMix->load();
        const float subLevel = pSubLevel->load();
        const float baseCut  = pCutoff->load();
        const float res      = juce::jlimit (0.0f, 0.95f, pResonance->load());
        const float fEnvAmt  = pFilterEnv->load();
        const float lfoRate  = pLfoRate->load();
        const float lfoPitch = pLfoPitch->load();
        const float lfoCut   = pLfoCutoff->load();
        const float maxCut   = juce::jmin (18000.0f, 0.33f * sr);
        const int   numCh    = output.getNumChannels();

        for (int n = 0; n < numSamples; ++n)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * lfoPhase);
            lfoPhase += lfoRate / sr;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            const float bendRatio = std::pow (2.0f, (bendSemis + lfo * lfoPitch) / 12.0f);
            const float f1 = noteHz * bendRatio;
            const float f2 = f1 * detuneRatio;

            const float dt1 = juce::jmin (0.45f, f1 / sr);
            const float dt2 = juce::jmin (0.45f, f2 / sr);
            const float dtS = juce::jmin (0.45f, 0.5f * f1 / sr);

            const float s1  = oscSample (w1, phase1, dt1);
            const float s2  = oscSample (w2, phase2, dt2);
            const float sub = oscSample (1, phaseSub, dtS);

            phase1   += dt1; if (phase1   >= 1.0f) phase1   -= 1.0f;
            phase2   += dt2; if (phase2   >= 1.0f) phase2   -= 1.0f;
            phaseSub += dtS; if (phaseSub >= 1.0f) phaseSub -= 1.0f;

            const float mixed = (s1 * (1.0f - oscMix) + s2 * oscMix + sub * subLevel) * 0.5f;

            const float fenv = filtEnv.getNextSample();
            const float amp  = ampEnv.getNextSample();

            float cutoffHz = baseCut * std::pow (2.0f, fEnvAmt * 6.0f * fenv + lfo * lfoCut * 2.0f);
            cutoffHz = juce::jlimit (20.0f, maxCut, cutoffHz);
            filter.setParams (cutoffHz, res, sr);

            const float y = filter.process (mixed) * amp * level * 0.8f;

            for (int ch = 0; ch < numCh; ++ch)
                output.addSample (ch, startSample + n, y);
        }

        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            filter.reset();
        }
    }

private:
    static float polyBlep (float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    // 0=Saw 1=Square 2=Triangle 3=Sine
    static float oscSample (int wave, float phase, float dt)
    {
        switch (wave)
        {
            case 0:
            {
                float v = 2.0f * phase - 1.0f;
                v -= polyBlep (phase, dt);
                return v;
            }
            case 1:
            {
                float v = phase < 0.5f ? 1.0f : -1.0f;
                v += polyBlep (phase, dt);
                float p2 = phase + 0.5f;
                if (p2 >= 1.0f) p2 -= 1.0f;
                v -= polyBlep (p2, dt);
                return v;
            }
            case 2:
                return 4.0f * std::abs (phase - 0.5f) - 1.0f;
            default:
                return std::sin (juce::MathConstants<float>::twoPi * phase);
        }
    }

    void updateEnvelopes()
    {
        juce::ADSR::Parameters a;
        a.attack  = juce::jmax (0.001f, pAttack->load());
        a.decay   = juce::jmax (0.001f, pDecay->load());
        a.sustain = juce::jlimit (0.0f, 1.0f, pSustain->load());
        a.release = juce::jmax (0.005f, pRelease->load());
        ampEnv.setParameters (a);

        juce::ADSR::Parameters f;
        f.attack  = 0.001f;
        f.decay   = juce::jmax (0.005f, pFDecay->load());
        f.sustain = 0.0f;
        f.release = a.release;
        filtEnv.setParameters (f);
    }

    std::atomic<float> *pOsc1Wave = nullptr, *pOsc2Wave = nullptr, *pOsc2Semi = nullptr, *pOsc2Detune = nullptr,
                       *pOscMix = nullptr, *pSubLevel = nullptr, *pCutoff = nullptr, *pResonance = nullptr,
                       *pFilterEnv = nullptr, *pFDecay = nullptr, *pAttack = nullptr, *pDecay = nullptr,
                       *pSustain = nullptr, *pRelease = nullptr, *pLfoRate = nullptr, *pLfoPitch = nullptr,
                       *pLfoCutoff = nullptr;

    juce::ADSR ampEnv, filtEnv;
    MoogLadder filter;

    float noteHz = 440.0f, level = 0.0f, bendSemis = 0.0f;
    float phase1 = 0.0f, phase2 = 0.0f, phaseSub = 0.0f, lfoPhase = 0.0f;
};
