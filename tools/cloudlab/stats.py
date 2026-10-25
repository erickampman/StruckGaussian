"""Print level and decay figures for cloudlab renders: peak, RMS, a T60 from a
Schroeder backward integral, the spectral centroid of the first half second,
and L/R correlation.

T60 is fitted to the -5 to -35 dB stretch of the energy decay curve (T30, x2).
A render whose curve never reaches -35 dB reports '>' and the time it ran out.
Crest is peak / RMS over the first 100 ms, in dB: how much of a click the
attack is, since every mode starts at the same instant."""
import sys
import numpy as np
from scipy.io import wavfile


def measure(fs, x):
    x = np.asarray(x, dtype=np.float64)
    m = x.mean(axis=1)
    e = np.cumsum((m ** 2)[::-1])[::-1]          # Schroeder backward integral
    edb = 10 * np.log10(np.maximum(e / e[0], 1e-30))
    t = np.arange(len(m)) / fs
    i5 = np.argmax(edb < -5)
    if (edb < -35).any():
        i35 = np.argmax(edb < -35)
        slope = np.polyfit(t[i5:i35], edb[i5:i35], 1)[0]
        t60 = f"{-60 / slope:.2f}s"
    else:
        t60 = f">{len(m) / fs:.0f}s"
    head = m[: int(0.5 * fs)]
    spec = np.abs(np.fft.rfft(head * np.hanning(len(head)))) ** 2
    freqs = np.fft.rfftfreq(len(head), 1 / fs)
    centroid = (freqs * spec).sum() / spec.sum()
    a = m[: int(0.1 * fs)]
    crest = 20 * np.log10(np.abs(a).max() / np.sqrt((a ** 2).mean()))
    corr = np.corrcoef(x[:, 0], x[:, 1])[0, 1]
    return dict(peak=np.abs(x).max(), rms=np.sqrt((x ** 2).mean()), t60=t60,
                centroid=centroid, crest=crest, corr=corr)


def line(name, s):
    return (f"{name:28s} peak {s['peak']:.4g}  rms {s['rms']:.4g}  T60 {s['t60']:>6s}  "
            f"centroid {s['centroid']:6.0f} Hz  crest {s['crest']:4.1f} dB  L/R corr {s['corr']:.2f}")


if __name__ == "__main__":
    for path in sys.argv[1:]:
        fs, x = wavfile.read(path)
        print(line(path, measure(fs, x)))
