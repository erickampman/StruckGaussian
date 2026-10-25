//
//  cloudlab.cpp
//
//  Offline renderer for the "mode cloud" experiment: one struck note through
//  the plugin's own ModalBank, whose modes are drawn from a Gaussian instead of
//  coming from a material table or a ModalGenerators series.
//
//    strike (StrikeModule's note-relative Hann contact)
//      -> ModalBank (src/Module, unchanged), fed a generated ModalMaterial
//         exactly as ModalBankModule feeds a generator: a stable buffer,
//         setMaterial, invalidate
//
//  The cloud: log2(ratio) ~ Normal(mean, sigma), in octaves above the
//  fundamental. With keep1=1 (the default) mode 0 is the fundamental itself,
//  ratio 1, and the other N-1 modes are drawn; with keep1=0 all N are drawn and
//  nothing is guaranteed to sit at the note's pitch. Amplitude is 1/sqrt(ratio),
//  the generators' law, and damp is 1: DampTilt, Brightness and Strike Position
//  shape the cloud as they shape every other material.
//
//  The cloud itself is src/Module/ModalCloud.hpp, the plugin's generator (in
//  StruckSynth, where this lab started, it was written out here).
//
//  Usage: cloudlab out.wav [key=value ...]   (cloudlab --help lists the keys)
//

#include "ModalBank.hpp"
#include "ModalCloud.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

// StruckGaussian's trim (ModalBankModule.cpp): the strike scaled so the bank's
// in-phase peak is Bar's at the defaults.
constexpr double kTrimRef = 2.1181;

// Pairs of audible modes closer than this beat (ModalGenerators' "Density").
constexpr double kBeatHz = 30.0;

struct Params {
    double note    = 60.0;
    double vel     = 1.0;
    double fs      = 48000.0;
    double seconds = 6.0;

    // the cloud
    int    count = 16;          // modes, 1..ModalBank::kMaxModes
    double mean  = 1.0;         // octaves above the fundamental
    double sigma = 0.5;         // octaves
    int    seed  = 1;
    int    keep1 = 1;           // 1: mode 0 is the fundamental (ratio 1)
    double focus = 0.0;         // loudness: 0 = 1/sqrt(ratio), 1 = a Gaussian peaked at fctr
    double fctr  = 0.0;         // octaves above the fundamental
    double fwid  = 0.5;         // octaves
    std::string mat;            // instead of a cloud: a ModalMaterials table, for reference

    // the bank and the strike, at the plugin's defaults
    double t60      = 2.0;
    double tilt     = 0.9;
    double pos      = 0.25;
    double bright   = 0.0;
    double stretch  = 0.0;
    double spread   = 0.3;
    double hardness = 0.15;     // effective: Hardness 0.3 at velocity 1 with VelToHardness 0.5
    int    trim     = 0;        // 1: StruckGaussian's peak trim. 0, as the README's measurements were made
};

const ModalMaterial* findMaterial(const std::string& name) {
    auto squash = [](std::string s) {
        std::string o;
        for (char c : s) if (c != ' ' && c != '_' && c != '-') o += (char)std::tolower((unsigned char)c);
        return o;
    };
    const std::string want = squash(name);
    for (int i = 0; i < ModalMaterialCount; ++i)
        if (squash(kModalMaterials[i].name) == want) return &kModalMaterials[i];
    return nullptr;
}

bool setParam(Params& p, const std::string& k, const std::string& v) {
    const double d = std::atof(v.c_str());
    std::map<std::string, double*> num = {
        {"note", &p.note}, {"vel", &p.vel}, {"fs", &p.fs}, {"seconds", &p.seconds},
        {"mean", &p.mean}, {"sigma", &p.sigma},
        {"focus", &p.focus}, {"fctr", &p.fctr}, {"fwid", &p.fwid},
        {"t60", &p.t60}, {"tilt", &p.tilt}, {"pos", &p.pos}, {"bright", &p.bright},
        {"stretch", &p.stretch}, {"spread", &p.spread}, {"hardness", &p.hardness},
    };
    if (auto it = num.find(k); it != num.end()) { *it->second = d; return true; }
    if (k == "count") { p.count = std::atoi(v.c_str()); return true; }
    if (k == "seed")  { p.seed  = std::atoi(v.c_str()); return true; }
    if (k == "keep1") { p.keep1 = std::atoi(v.c_str()); return true; }
    if (k == "mat")   { p.mat   = v; return true; }
    if (k == "trim")  { p.trim  = std::atoi(v.c_str()); return true; }
    return false;
}

void usage() {
    std::puts(
        "cloudlab out.wav [key=value ...]\n"
        "  note vel seconds fs\n"
        "  cloud: count mean sigma (octaves above f0) seed keep1 (1 = mode 0 at ratio 1)\n"
        "  loudness: focus (0 = 1/sqrt(ratio), 1 = Gaussian) fctr fwid (octaves)\n"
        "  mat=<material> renders a ModalMaterials table instead, for reference\n"
        "  bank:  t60 tilt pos bright stretch spread    strike: hardness (0..1 of 1.6 periods)\n"
        "  trim=1: StruckGaussian's peak trim (the plugin always applies it)\n"
        "Materials: string bar tunedbar membrane opentube closedtube tine bell\n"
        "           guitarbody violinbody soundboard genericbody");
}

// The plugin's generator (src/Module/ModalCloud.hpp), so the lab draws the cloud the plugin plays.
int makeCloud(const Params& p, ModalMode* out) {
    return ModalCloud::generate(p.count, p.mean, p.sigma, (uint32_t)p.seed, p.keep1 != 0,
                                out, ModalBank::kMaxModes, p.focus, p.fctr, p.fwid);
}

bool writeWavFloat(const char* path, const std::vector<float>& L, const std::vector<float>& R, int fs) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint32_t frames = (uint32_t)L.size(), ch = 2, bytes = frames * ch * 4;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + bytes); std::fwrite("WAVE", 1, 4, f);
    std::fwrite("fmt ", 1, 4, f); u32(16); u16(3); u16((uint16_t)ch); u32((uint32_t)fs);
    u32((uint32_t)fs * ch * 4); u16((uint16_t)(ch * 4)); u16(32);
    std::fwrite("data", 1, 4, f); u32(bytes);
    for (uint32_t i = 0; i < frames; ++i) { std::fwrite(&L[i], 4, 1, f); std::fwrite(&R[i], 4, 1, f); }
    std::fclose(f);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || !std::strcmp(argv[1], "--help")) { usage(); return argc < 2 ? 1 : 0; }
    Params p;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        const auto eq = a.find('=');
        if (eq == std::string::npos || !setParam(p, a.substr(0, eq), a.substr(eq + 1))) {
            std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
            usage();
            return 1;
        }
    }

    const double fs = p.fs;
    const double f0 = 440.0 * std::exp2((p.note - 69.0) / 12.0);
    const size_t N  = (size_t)(p.seconds * fs);

    // --- the material: a cloud in a stable buffer, or a table for reference ---
    ModalMode     cloud[ModalBank::kMaxModes];
    ModalMaterial cloudMat{ cloud, 0, "Cloud" };
    const ModalMaterial* mat = &cloudMat;
    if (!p.mat.empty()) {
        mat = findMaterial(p.mat);
        if (!mat) { std::fprintf(stderr, "unknown material %s\n", p.mat.c_str()); return 1; }
    } else {
        cloudMat.count = makeCloud(p, cloud);
    }

    // --- strike: raised-cosine contact, as StrikeModule's note-relative mode ---
    std::vector<float> strike(N, 0.0f);
    {
        const int len = std::max(1, (int)std::lround(p.hardness * 1.6 * fs / f0));
        double sum = 0.0;
        for (int i = 0; i < len && i < (int)N; ++i) {
            const double w = (len == 1) ? 1.0 : 0.5 - 0.5 * std::cos(2.0 * M_PI * (i + 0.5) / len);
            strike[i] = (float)w;
            sum += w;
        }
        for (int i = 0; i < len && i < (int)N; ++i) strike[i] = (float)(p.vel * strike[i] / sum);
    }

    // --- the bank, fed as ModalBankModule feeds a generator ---
    ModalBank bank;
    bank.setSampleRate(fs);
    bank.setMaterial(*mat);
    bank.invalidate();
    bank.setFundamental((float)f0);
    bank.setModeCount(ModalBank::kMaxModes);
    bank.setStretch((float)p.stretch);
    bank.setDecayT60((float)p.t60);
    bank.setDampTilt((float)p.tilt);
    bank.setStrikePosition((float)p.pos);
    bank.setPositionWeighting(!p.mat.empty());   // clouds as in StruckGaussian: no string mode shapes
    bank.setBrightness((float)p.bright);
    bank.setSpread((float)p.spread);
    bank.prepare();
    if (p.trim) {
        double peak = 0.0;
        for (int i = 0; i < bank.activeModes(); ++i) peak += bank.modeAmplitude(i);
        if (peak > 1e-6) for (float& s : strike) s = (float)(s * kTrimRef / peak);
    }

    std::vector<float> outL(N), outR(N);
    for (size_t i = 0; i < N; i += 512) {
        const int n = (int)std::min<size_t>(512, N - i);
        bank.processBlock(strike.data() + i, outL.data() + i, outR.data() + i, n);
    }

    if (!writeWavFloat(argv[1], outL, outR, (int)fs)) {
        std::fprintf(stderr, "can't write %s\n", argv[1]);
        return 1;
    }

    // --- report: what the bank actually built ---
    // Audible = below the bank's Nyquist guard (it zeroes the rest) and at or
    // above 20 Hz. Beating pairs are adjacent audible modes under kBeatHz apart.
    int audible = 0, sub = 0, beating = 0;
    double fmin = 1e9, fmax = 0.0, prev = -1.0, t60min = 1e9, t60max = 0.0;
    for (int i = 0; i < bank.activeModes(); ++i) {
        const double f = bank.modeFrequency(i), t = bank.modeT60(i);
        if (t <= 0.0) continue;                  // muted at Nyquist
        if (f < 20.0) { ++sub; continue; }
        ++audible;
        fmin = std::min(fmin, f); fmax = std::max(fmax, f);
        t60min = std::min(t60min, t); t60max = std::max(t60max, t);
        if (prev > 0.0 && f - prev < kBeatHz) ++beating;
        prev = f;
    }
    float peak = 0.0f;
    for (size_t i = 0; i < N; ++i) peak = std::max({ peak, std::fabs(outL[i]), std::fabs(outR[i]) });
    std::fprintf(stderr, "modes %d/%d audible (%d under 20 Hz), %.0f..%.0f Hz, "
                         "%d pairs < %.0f Hz apart, mode T60 %.2f..%.2f s\n",
                 audible, mat->count, sub, fmin, fmax, beating, kBeatHz, t60min, t60max);
    std::fprintf(stderr, "ratios");
    for (int i = 0; i < mat->count; ++i) std::fprintf(stderr, " %.3f", mat->modes[i].ratio);
    std::fprintf(stderr, "\nstarting amplitudes");
    for (int i = 0; i < bank.activeModes(); ++i) std::fprintf(stderr, " %.3f", bank.modeAmplitude(i));
    std::fprintf(stderr, "\n%s: note %.1f (%.1f Hz), peak %.4f\n", argv[1], p.note, f0, peak);
    return 0;
}
