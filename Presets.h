#pragma once
#include <vector>

// Presets de estilo "synth analogico de los 80" (secuencias, cuerdas, leads).
// Son aproximaciones propias, no copias de ningun sonido grabado.
// Para parametros de tipo lista (formas de onda): 0=Saw 1=Square 2=Triangle 3=Sine.

struct PresetParam { const char* id; float value; };
struct Preset { const char* name; std::vector<PresetParam> params; };

inline const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets =
    {
        { "Init", {} },

        { "Secuencia Moog",
          { {"osc1Wave",0}, {"osc2Wave",1}, {"osc2Semi",-12}, {"osc2Detune",4}, {"oscMix",0.4f}, {"subLevel",0.3f},
            {"cutoff",650}, {"resonance",0.55f}, {"filterEnv",0.55f}, {"fDecay",0.22f},
            {"attack",0.002f}, {"decay",0.2f}, {"sustain",0.35f}, {"release",0.12f},
            {"delayTime",375}, {"delayFeedback",0.4f}, {"delayMix",0.25f}, {"reverbSize",0.4f}, {"reverbMix",0.1f} } },

        { "Pad Cuerdas",
          { {"osc1Wave",0}, {"osc2Wave",0}, {"osc2Semi",0}, {"osc2Detune",14}, {"oscMix",0.5f}, {"subLevel",0},
            {"cutoff",5200}, {"resonance",0.1f}, {"filterEnv",0.1f}, {"fDecay",1.0f},
            {"attack",0.7f}, {"decay",1.0f}, {"sustain",0.85f}, {"release",1.4f},
            {"lfoRate",5.8f}, {"lfoPitch",0.08f},
            {"delayTime",480}, {"delayFeedback",0.35f}, {"delayMix",0.15f}, {"reverbSize",0.8f}, {"reverbMix",0.35f} } },

        { "Lead CS-80",
          { {"osc1Wave",0}, {"osc2Wave",0}, {"osc2Semi",0}, {"osc2Detune",9}, {"oscMix",0.5f}, {"subLevel",0},
            {"cutoff",3200}, {"resonance",0.25f}, {"filterEnv",0.35f}, {"fDecay",0.6f},
            {"attack",0.06f}, {"decay",0.5f}, {"sustain",0.8f}, {"release",0.35f},
            {"lfoRate",5.2f}, {"lfoPitch",0.15f}, {"lfoCutoff",0.1f},
            {"delayTime",400}, {"delayFeedback",0.35f}, {"delayMix",0.25f}, {"reverbSize",0.5f}, {"reverbMix",0.2f} } },

        { "Bajo Profundo",
          { {"osc1Wave",1}, {"osc2Wave",0}, {"osc2Semi",-12}, {"osc2Detune",3}, {"oscMix",0.5f}, {"subLevel",0.6f},
            {"cutoff",420}, {"resonance",0.35f}, {"filterEnv",0.45f}, {"fDecay",0.3f},
            {"attack",0.003f}, {"decay",0.25f}, {"sustain",0.6f}, {"release",0.15f},
            {"delayMix",0}, {"reverbMix",0} } },

        { "Pluck Eco",
          { {"osc1Wave",0}, {"osc2Wave",1}, {"osc2Semi",12}, {"osc2Detune",6}, {"oscMix",0.35f}, {"subLevel",0},
            {"cutoff",1800}, {"resonance",0.3f}, {"filterEnv",0.7f}, {"fDecay",0.18f},
            {"attack",0.001f}, {"decay",0.18f}, {"sustain",0}, {"release",0.2f},
            {"delayTime",375}, {"delayFeedback",0.5f}, {"delayMix",0.35f}, {"reverbSize",0.6f}, {"reverbMix",0.2f} } },

        { "Arpegio Cristal",
          { {"osc1Wave",2}, {"osc2Wave",3}, {"osc2Semi",12}, {"osc2Detune",0}, {"oscMix",0.45f}, {"subLevel",0},
            {"cutoff",9000}, {"resonance",0.1f}, {"filterEnv",0.2f}, {"fDecay",0.4f},
            {"attack",0.002f}, {"decay",0.35f}, {"sustain",0.1f}, {"release",0.5f},
            {"delayTime",300}, {"delayFeedback",0.45f}, {"delayMix",0.3f}, {"reverbSize",0.7f}, {"reverbMix",0.3f} } },

        { "Organo Etereo",
          { {"osc1Wave",1}, {"osc2Wave",1}, {"osc2Semi",12}, {"osc2Detune",2}, {"oscMix",0.5f}, {"subLevel",0},
            {"cutoff",6000}, {"resonance",0.05f}, {"filterEnv",0}, {"fDecay",0.5f},
            {"attack",0.05f}, {"decay",0.3f}, {"sustain",1.0f}, {"release",0.5f},
            {"lfoRate",6.0f}, {"lfoPitch",0.05f},
            {"delayMix",0.1f}, {"reverbSize",0.7f}, {"reverbMix",0.4f} } }
    };
    return presets;
}
