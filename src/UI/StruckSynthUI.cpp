// StruckSynthUI.cpp — the modal-synthesis front end. Five grouped panels (Global, Strike, Object,
// Decay, Timbre/Output) of reusable Knob/Dropdown controls, plus a live ModalSpectrum visual that
// drives the real framework-free ModalBank core (linked via pm_framework) to draw the current
// mode layout and per-mode decay. No presets / IFS pads / mod-matrix — this is the v1 modal voice.

#include "DistrhoUI.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Dropdown.hpp"
#include "GroupBox.hpp"
#include "Knob.hpp"
#include "PopupMenu.hpp"

#include "ModalBank.hpp"        // framework-free core, for the spectrum visual
#include "ModalGenerators.hpp"  // procedural mode series
#include "ModalMaterials.hpp"   // static material tables + kModalStretchValues
#include "pm_params.h"          // ModalBankMaterial / MODAL_BANK_FIRST_GENERATOR
#include "StruckSynthParams.h"

START_NAMESPACE_DISTRHO

using namespace ifsynth;  // params (kParams, ParamIndex, kMaterialNames)

using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::Dropdown;
using DGL_NAMESPACE::GroupBox;
using DGL_NAMESPACE::Knob;
using DGL_NAMESPACE::PopupMenu;

// Category accent palette (struck-instrument scheme): global=steel, strike=amber, object=green,
// decay=violet, output=copper. Reused for panels, knobs, and the spectrum.
static inline Color colGlobal() { return Color(150, 166, 182); }
static inline Color colStrike() { return Color(214, 168, 92);  }
static inline Color colObject() { return Color(120, 190, 140); }
static inline Color colDecay()  { return Color(168, 140, 214); }
static inline Color colOutput() { return Color(214, 132, 96);  }

// --------------------------------------------------------------------------------------------
// ModalSpectrum — draws the current bank as vertical bars: x = mode frequency (log), height =
// relative decay time (T60), so damp-tilt and the damp-bell visibly reshape the profile. It owns
// a ModalBank core and reconfigures it from the modal params on every relevant change.
class ModalSpectrum : public NanoSubWidget
{
public:
    explicit ModalSpectrum(Widget* parent) : NanoSubWidget(parent)
    {
        fBank.setSampleRate(48000.0);
        recompute();
    }

    // Push the modal params the visual depends on, then recompute the mode layout.
    void update(int material, int modes, int stretchIdx, float pitchOct,
                float decayT60, float dampTilt, float brightness)
    {
        fMaterial = material; fModes = modes; fStretchIdx = stretchIdx; fPitch = pitchOct;
        fDecayT60 = decayT60; fDampTilt = dampTilt; fBrightness = brightness;
        recompute();
        repaint();
    }

protected:
    void onNanoDisplay() override
    {
        const float w = getWidth(), h = getHeight();

        // Panel background + border.
        beginPath(); roundedRect(0, 0, w, h, 6); fillColor(Color(28, 31, 37)); fill();
        beginPath(); roundedRect(0.5f, 0.5f, w - 1, h - 1, 6);
        strokeColor(Color(58, 62, 70)); strokeWidth(1.0f); stroke();

        const float padX = 12, padTop = 10, padBot = 16;
        const float x0 = padX, x1 = w - padX, y1 = h - padBot;
        const float plotH = h - padTop - padBot;
        if (fCount <= 0 || plotH <= 4) return;

        // Log-frequency axis spanning the mode range (with a little headroom).
        const float fLo = std::log2(std::max(20.0f, fMinFreq * 0.9f));
        const float fHi = std::log2(std::max(fLo + 1.0f, fMaxFreq * 1.1f));

        // Baseline.
        beginPath(); moveTo(x0, y1 + 3); lineTo(x1, y1 + 3);
        strokeColor(Color(70, 74, 82)); strokeWidth(1.0f); stroke();

        for (int i = 0; i < fCount; ++i)
        {
            const float fx  = (std::log2(fFreq[i]) - fLo) / (fHi - fLo);
            const float px  = x0 + fx * (x1 - x0);
            const float mag = std::clamp(fT60[i] / fT60Max, 0.02f, 1.0f);
            const float bh  = mag * plotH;

            // Longer-ringing modes draw more opaque; the cap dot keeps short modes visible.
            Color bar = fAccent; bar.alpha = 0.35f + 0.55f * mag;
            beginPath(); rect(px - 1.5f, y1 - bh, 3.0f, bh);
            fillColor(bar); fill();
            beginPath(); circle(px, y1 - bh, 2.2f); fillColor(Color(240, 244, 248)); fill();
        }
    }

private:
    void recompute()
    {
        // Fundamental for the display only: a nominal C3 transposed by the Pitch offset.
        const float f0 = 130.81f * std::exp2(fPitch);
        fBank.setFundamental(f0);
        fBank.setModeCount(fModes);
        fBank.setStretch(kModalStretchValues[std::clamp(fStretchIdx, 0, kModalStretchCount - 1)]);
        fBank.setDecayT60(fDecayT60);
        fBank.setDampTilt(fDampTilt);
        fBank.setBrightness(fBrightness);
        selectMaterial(fMaterial);
        fBank.prepare();

        fCount = std::min(fBank.activeModes(), (int)ModalBank::kMaxModes);
        fMinFreq = 1e9f; fMaxFreq = 1.0f; fT60Max = 1e-4f;
        for (int i = 0; i < fCount; ++i)
        {
            fFreq[i] = fBank.modeFrequency(i);
            fT60[i]  = fBank.modeT60(i);
            fMinFreq = std::min(fMinFreq, fFreq[i]);
            fMaxFreq = std::max(fMaxFreq, fFreq[i]);
            fT60Max  = std::max(fT60Max, fT60[i]);
        }
        if (fCount <= 0) { fMinFreq = 100.0f; fMaxFreq = 1000.0f; }
    }

    // Mirror of ModalBankModule's selectMaterial: static tables vs procedural generators.
    void selectMaterial(int idx)
    {
        idx = std::clamp(idx, 0, (int)ModalBankMaterialCount - 1);
        if (idx < (int)MODAL_BANK_FIRST_GENERATOR) { fBank.setMaterial(kModalMaterials[idx]); return; }
        const ModalSeriesID series = (ModalSeriesID)(idx - MODAL_BANK_FIRST_GENERATOR);
        const float param = (series == ModalSeriesPiano) ? 0.0003f
                          : (series == ModalSeriesPower) ? 1.5f : 0.0f;
        const int n = generateModalSeries(series, param, ModalBank::kMaxModes,
                                          fGenBuffer, ModalBank::kMaxModes);
        fGenMaterial.modes = fGenBuffer;
        fGenMaterial.count = n;
        fGenMaterial.name  = "Gen";
        fBank.setMaterial(fGenMaterial);
        fBank.invalidate();
    }

    ModalBank     fBank;
    ModalMode     fGenBuffer[ModalBank::kMaxModes];
    ModalMaterial fGenMaterial { nullptr, 0, "Gen" };
    Color         fAccent = colObject();

    int   fMaterial = 1, fModes = 64, fStretchIdx = 9;
    float fPitch = 0.0f, fDecayT60 = 2.0f, fDampTilt = 0.9f, fBrightness = 0.0f;

    int   fCount = 0;
    float fFreq[ModalBank::kMaxModes] = {};
    float fT60 [ModalBank::kMaxModes] = {};
    float fMinFreq = 100.0f, fMaxFreq = 1000.0f, fT60Max = 1.0f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModalSpectrum)
};

// --------------------------------------------------------------------------------------------
class StruckSynthUI : public UI,
                      public Knob::Callback,
                      public Dropdown::Callback,
                      public PopupMenu::Callback
{
public:
    StruckSynthUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        setGeometryConstraints(720, 576, true, false); // lock the 900:720 design ratio
        fHasFont = loadSharedResources();               // DPF's bundled DejaVu Sans for the title

        for (uint32_t i = 0; i < kParamCount; ++i)
            fValues[i] = kParams[i].def;

        fSpectrum = std::make_unique<ModalSpectrum>(this);

        fGlobalGroup = makeGroup("Global", colGlobal());
        fStrikeGroup = makeGroup("Strike", colStrike());
        fObjectGroup = makeGroup("Object", colObject());
        fDecayGroup  = makeGroup("Decay",  colDecay());
        fOutputGroup = makeGroup("Timbre & Output", colOutput());

        // Build each param control into its panel, in panel order.
        for (uint32_t i = 0; i < kParamCount; ++i)
        {
            GroupBox* g = groupFor(i);
            if (isDropdownParam(i))
            {
                auto d = std::make_unique<Dropdown>(g, this);
                d->setId(i);
                d->setLabel(kParams[i].name);
                d->setChoices(choicesFor(i));
                d->setIndex(static_cast<int>(kParams[i].def));
                g->addControl(d.get());
                fDropdowns[i] = std::move(d);
            }
            else
            {
                const ParamSpec& s = kParams[i];
                auto k = std::make_unique<Knob>(g, this);
                k->setId(i);
                k->setLabel(s.name);
                k->setAccentColor(accentFor(i));
                k->configure(s.min, s.max, s.logarithmic, s.def);
                k->setRealValue(s.def, false);
                k->setFormatter(formatterFor(i));
                g->addControl(k.get());
                fKnobs[i] = std::move(k);
            }
        }

        fMenu = std::make_unique<PopupMenu>(this, this); // created last → draws on top

        pushSpectrum();
        layout();
    }

protected:
    /* -- host -> UI ------------------------------------------------------- */
    void parameterChanged(uint32_t index, float value) override
    {
        if (index >= kParamCount) return;
        fValues[index] = value;
        if (fKnobs[index])     { fKnobs[index]->setRealValue(value, false); fKnobs[index]->repaint(); }
        if (fDropdowns[index])   fDropdowns[index]->setIndex(static_cast<int>(value + 0.5f));
        if (affectsSpectrum(index)) pushSpectrum();
    }

    /* -- knob -> host ----------------------------------------------------- */
    void knobDragStarted(Knob* k) override  { editParameter(k->getId(), true); }
    void knobDragFinished(Knob* k) override { editParameter(k->getId(), false); }
    void knobValueChanged(Knob* k, float v) override
    {
        const uint32_t id = k->getId();
        fValues[id] = v;
        setParameterValue(id, v);
        if (affectsSpectrum(id)) pushSpectrum();
    }

    /* -- dropdown + menu -------------------------------------------------- */
    void dropdownClicked(Dropdown* d) override
    {
        int x, y, w, h;
        d->boxRect(x, y, w, h);
        fMenu->openFor(d->getId(), d->choices(), d->index(), x, y + h, y, w,
                       static_cast<int>(getHeight()));
    }
    void menuSelected(PopupMenu* menu, int index) override
    {
        const uint32_t id = menu->tag();
        if (id >= kParamCount) return;
        editParameter(id, true);
        setParameterValue(id, static_cast<float>(index));
        editParameter(id, false);
        fValues[id] = static_cast<float>(index);
        if (fDropdowns[id]) fDropdowns[id]->setIndex(index);
        if (affectsSpectrum(id)) pushSpectrum();
    }

    /* -- drawing / layout ------------------------------------------------- */
    void onNanoDisplay() override
    {
        const float w = getWidth(), h = getHeight();
        beginPath(); rect(0, 0, w, h); fillColor(Color(22, 24, 29)); fill();

        // Title (top-left of the header strip).
        if (fHasFont)
        {
            const float s = w / 900.0f;
            fontFace(NANOVG_DEJAVU_SANS_TTF);
            fontSize(22 * s);
            fillColor(Color(224, 190, 150));
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            text(16 * s, 22 * s, "StruckSynth", nullptr);
            fillColor(Color(150, 154, 162)); fontSize(12 * s);
            text(150 * s, 24 * s, "modal synthesis  ·  Unlikelyware", nullptr);
        }
    }
    void onResize(const ResizeEvent& ev) override { UI::onResize(ev); layout(); }

private:
    void layout()
    {
        const int W = getWidth(), H = getHeight();
        const float s = W / 900.0f;
        const int M = static_cast<int>(16 * s);
        const int headerH = static_cast<int>(40 * s);

        // Spectrum spans the width under the header.
        const int specY = headerH;
        const int specH = static_cast<int>(150 * s);
        fSpectrum->setAbsolutePos(M, specY);
        fSpectrum->setSize(W - 2 * M, specH);

        // Five stacked full-width panels.
        GroupBox* panels[5] = { fGlobalGroup.get(), fStrikeGroup.get(), fObjectGroup.get(),
                                fDecayGroup.get(), fOutputGroup.get() };
        int py = specY + specH + static_cast<int>(12 * s);
        const int gap = static_cast<int>(8 * s);
        const int panelH = (H - py - M - 4 * gap) / 5;
        const float knobD = std::min(60.0f * s, panelH * 0.52f);

        for (GroupBox* g : panels)
        {
            g->setUiScale(s);
            g->setAbsolutePos(M, py);
            g->setSize(W - 2 * M, panelH);
            py += panelH + gap;
        }
        // Size the child controls, then let each box arrange its row.
        for (uint32_t i = 0; i < kParamCount; ++i)
        {
            if (fKnobs[i])     fKnobs[i]->setDiameter(knobD, s);
            if (fDropdowns[i]) { fDropdowns[i]->setUiScale(s); fDropdowns[i]->setSize(static_cast<int>(130 * s), static_cast<int>(26 * s)); }
        }
        for (GroupBox* g : panels) g->relayout();
    }

    std::unique_ptr<GroupBox> makeGroup(const char* title, Color accent)
    {
        auto g = std::make_unique<GroupBox>(this);
        g->setTitle(title);
        g->setAccentColor(accent);
        return g;
    }

    GroupBox* groupFor(uint32_t i) const
    {
        switch (i)
        {
        case kParamVoiceMode: case kParamGlide: case kParamLevel: case kParamSpread:
            return fGlobalGroup.get();
        case kParamStrikeType: case kParamHardness: case kParamVelToLevel:
        case kParamVelToHardness: case kParamContactMode:
            return fStrikeGroup.get();
        case kParamPitch: case kParamMaterial: case kParamModeCount: case kParamStretch:
            return fObjectGroup.get();
        case kParamDecayT60: case kParamDampTilt: case kParamDampBellAmount:
        case kParamDampBellCentre: case kParamDampBellWidth:
            return fDecayGroup.get();
        default:
            return fOutputGroup.get(); // strikePos, brightness, mix
        }
    }

    static Color accentFor(uint32_t i)
    {
        switch (i)
        {
        case kParamVoiceMode: case kParamGlide: case kParamLevel: case kParamSpread:
            return colGlobal();
        case kParamStrikeType: case kParamHardness: case kParamVelToLevel:
        case kParamVelToHardness: case kParamContactMode:
            return colStrike();
        case kParamPitch: case kParamMaterial: case kParamModeCount: case kParamStretch:
            return colObject();
        case kParamDecayT60: case kParamDampTilt: case kParamDampBellAmount:
        case kParamDampBellCentre: case kParamDampBellWidth:
            return colDecay();
        default:
            return colOutput();
        }
    }

    static bool isDropdownParam(uint32_t i)
    {
        return i == kParamVoiceMode || i == kParamStrikeType || i == kParamContactMode
            || i == kParamMaterial;
    }

    static std::vector<std::string> choicesFor(uint32_t i)
    {
        switch (i)
        {
        case kParamVoiceMode:  return { "Mono", "Poly" };
        case kParamStrikeType: return { "Mallet", "Noise" };
        case kParamContactMode:return { "Note-Rel", "Wall-Clock" };
        case kParamMaterial:
        {
            std::vector<std::string> v;
            for (const char* n : kMaterialNames) v.emplace_back(n);
            return v;
        }
        default: return {};
        }
    }

    static bool affectsSpectrum(uint32_t i)
    {
        return i == kParamMaterial || i == kParamModeCount || i == kParamStretch
            || i == kParamPitch || i == kParamDecayT60 || i == kParamDampTilt
            || i == kParamBrightness;
    }

    void pushSpectrum()
    {
        fSpectrum->update(static_cast<int>(fValues[kParamMaterial] + 0.5f),
                          static_cast<int>(fValues[kParamModeCount] + 0.5f),
                          static_cast<int>(fValues[kParamStretch] + 0.5f),
                          fValues[kParamPitch], fValues[kParamDecayT60],
                          fValues[kParamDampTilt], fValues[kParamBrightness]);
    }

    static std::function<std::string(float)> formatterFor(uint32_t i)
    {
        const std::string u = kParams[i].unit;
        const bool integer = kParams[i].integer;
        const bool isTime  = (u == "s");
        return [u, integer, isTime](float v) {
            char b[32];
            if (integer)      std::snprintf(b, sizeof(b), "%d", static_cast<int>(v + 0.5f));
            else if (isTime)  v < 1.0f ? std::snprintf(b, sizeof(b), "%.0f ms", v * 1000.0f)
                                       : std::snprintf(b, sizeof(b), "%.2f s", v);
            else if (!u.empty()) std::snprintf(b, sizeof(b), "%.2f %s", v, u.c_str());
            else              std::snprintf(b, sizeof(b), "%.2f", v);
            return std::string(b);
        };
    }

    std::unique_ptr<ModalSpectrum> fSpectrum;
    std::unique_ptr<GroupBox>      fGlobalGroup, fStrikeGroup, fObjectGroup, fDecayGroup, fOutputGroup;
    std::unique_ptr<Knob>          fKnobs[kParamCount];
    std::unique_ptr<Dropdown>      fDropdowns[kParamCount];
    std::unique_ptr<PopupMenu>     fMenu;
    float                          fValues[kParamCount];
    bool                           fHasFont = false;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StruckSynthUI)
};

UI* createUI()
{
    return new StruckSynthUI();
}

END_NAMESPACE_DISTRHO
