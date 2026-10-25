"""Render the mode-cloud experiments with cloudlab.

Each experiment is a sweep with one thing changing, rendered at C2, C4 and C6.
Every file in a sweep, all three notes included, gets the SAME gain (peak of the
loudest at -1 dBFS), so level differences between files are real and A/B
comparisons are fair. Writes 24-bit WAVs, one spectrogram sheet per sweep
(a column per note) and stats.txt (the figures and the cloud's ratios) into
renders/<experiment>/.

    python3 -I experiments.py            # all
    python3 -I experiments.py 2 3        # just some
"""
import os
import subprocess
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import spectrogram
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from stats import measure, line  # noqa: E402

CLOUDLAB = os.path.join(HERE, "build", "cloudlab")
OUT = os.path.join(HERE, "renders")

NOTES = [("C2", 36), ("C4", 60), ("C6", 84)]
# The base cloud every sweep varies one thing of: 16 modes, centred an octave
# above the note, half an octave wide, the fundamental kept, seed 1.
BASE = dict(count=16, mean=1, sigma=0.5, seed=1, keep1=1)

EXPERIMENTS = {
    "1": ("exp1_sigma", "Spread (sigma, octaves); 16 modes centred 1 octave up; Bar for reference", [
        ("00_bar_reference", {"mat": "bar"}),
        ("01_sigma_0.02", {"sigma": 0.02}),
        ("02_sigma_0.1",  {"sigma": 0.1}),
        ("03_sigma_0.25", {"sigma": 0.25}),
        ("04_sigma_0.5",  {"sigma": 0.5}),
        ("05_sigma_1",    {"sigma": 1}),
        ("06_sigma_2",    {"sigma": 2}),
    ]),
    "2": ("exp2_mean", "Centre (mean, octaves above f0); sigma 0.5, 16 modes", [
        ("01_mean_-1", {"mean": -1}),
        ("02_mean_0",  {"mean": 0}),
        ("03_mean_1",  {"mean": 1}),
        ("04_mean_2",  {"mean": 2}),
        ("05_mean_3",  {"mean": 3}),
    ]),
    "3": ("exp3_count", "Mode count; mean 1, sigma 0.5", [
        ("01_n4",  {"count": 4}),
        ("02_n8",  {"count": 8}),
        ("03_n16", {"count": 16}),
        ("04_n32", {"count": 32}),
        ("05_n64", {"count": 64}),
    ]),
    "4": ("exp4_seed", "Seed, with and without the fundamental kept; mean 1, sigma 0.5, 16 modes", [
        ("01_seed1",       {"seed": 1}),
        ("02_seed2",       {"seed": 2}),
        ("03_seed3",       {"seed": 3}),
        ("04_seed1_free",  {"seed": 1, "keep1": 0}),
        ("05_seed2_free",  {"seed": 2, "keep1": 0}),
        ("06_seed3_free",  {"seed": 3, "keep1": 0}),
    ]),
    "5": ("exp5_unison", "A tight cloud on the fundamental (mean 0); 16 modes", [
        ("01_sigma_0.005", {"mean": 0, "sigma": 0.005}),
        ("02_sigma_0.02",  {"mean": 0, "sigma": 0.02}),
        ("03_sigma_0.05",  {"mean": 0, "sigma": 0.05}),
        ("04_sigma_0.1",   {"mean": 0, "sigma": 0.1}),
    ]),
}


def render(path, params):
    args = [CLOUDLAB, path] + [f"{k}={v}" for k, v in params.items()]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stderr)
    return r.stderr.strip().splitlines()


def run(key):
    folder, title, variants = EXPERIMENTS[key]
    d = os.path.join(OUT, folder)
    os.makedirs(d, exist_ok=True)
    print(f"== {folder}: {title}")
    raws = []      # (name, fs, x, log)
    for name, params in variants:
        for nname, note in NOTES:
            full = f"{name}_{nname}"
            tmp = os.path.join(d, f".{full}.f32.wav")
            cloud = {} if "mat" in params else BASE
            log = render(tmp, {**cloud, **params, "note": note})
            fs, x = wavfile.read(tmp)
            os.remove(tmp)
            raws.append((full, fs, x.astype(np.float64), log))

    peak = max(np.abs(x).max() for _, _, x, _ in raws)
    gain = 10 ** (-1 / 20) / peak
    report = [f"{title}", f"shared gain {20 * np.log10(gain):+.1f} dB", ""]
    for name, fs, x, log in raws:
        y = np.clip(x * gain, -1, 1)
        wavfile.write(os.path.join(d, f"{name}.wav"), fs, (y * (2**31 - 1)).astype(np.int32))
        s = line(name, measure(fs, y))
        modes = next((l for l in log if l.startswith("modes")), "")
        report += [s, "    " + modes]
        if name.endswith("_C4"):
            report.append("    " + next((l for l in log if l.startswith("ratios")), ""))
        print("  " + s)
    with open(os.path.join(d, "stats.txt"), "w") as f:
        f.write("\n".join(report) + "\n")

    rows = len(variants)
    fig, axes = plt.subplots(rows, len(NOTES), figsize=(15, 2.1 * rows), sharex=True, squeeze=False)
    for i, (name, fs, x, _) in enumerate(raws):
        ax = axes[i // len(NOTES)][i % len(NOTES)]
        f, t, S = spectrogram(x.mean(axis=1) * gain, fs, nperseg=4096, noverlap=3072)
        ax.pcolormesh(t, f, 10 * np.log10(S + 1e-14), vmin=-130, vmax=-30, shading="auto", cmap="magma")
        ax.set_yscale("symlog", linthresh=200)
        ax.set_ylim(20, 20000)
        ax.set_title(name, fontsize=8, loc="left")
        if i % len(NOTES) == 0:
            ax.set_ylabel("Hz")
    for ax in axes[-1]:
        ax.set_xlabel("seconds")
    fig.suptitle(title)
    fig.tight_layout()
    fig.savefig(os.path.join(d, "spectrograms.png"), dpi=80)
    plt.close(fig)


if __name__ == "__main__":
    keys = sys.argv[1:] or list(EXPERIMENTS)
    keys = [k for k in EXPERIMENTS if any(k == a or k.startswith(a) for a in keys)]
    for k in keys:
        run(k)
