#include "Analyzer.h"

Analyzer::Analyzer()
    : histL ((size_t) historySize, 0.0f), histR ((size_t) historySize, 0.0f)
{
    prepare (16384);
}

void Analyzer::setSampleRate (double sr)
{
    if (sr > 0 && ! juce::exactlyEqual (sr, sampleRate)) { sampleRate = sr; reset(); }
    binHzValue = (float) (sampleRate / currentSize);
}

void Analyzer::reset()
{
    std::fill (histL.begin(), histL.end(), 0.0f);
    std::fill (histR.begin(), histR.end(), 0.0f);
    std::fill (smoothMag.begin(), smoothMag.end(), 0.0f);
    std::fill (db.begin(), db.end(), -200.0f);
    writePos = 0;
}

void Analyzer::push (const float* l, const float* r, int n)
{
    // only the newest historySize samples matter
    if (n > historySize) { l += n - historySize; r += n - historySize; n = historySize; }
    for (int i = 0; i < n; ++i)
    {
        histL[(size_t) writePos] = l[i];
        histR[(size_t) writePos] = r[i];
        writePos = (writePos + 1) & (historySize - 1);
    }
}

void Analyzer::prepare (int size)
{
    if (size == currentSize) return;
    currentSize = size;
    fft = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 (size)));
    window.resize ((size_t) size);
    for (int i = 0; i < size; ++i)
    {
        const double x = juce::MathConstants<double>::twoPi * i / size;
        window[(size_t) i] = (float) (0.42 - 0.5 * std::cos (x) + 0.08 * std::cos (2 * x));
    }
    work.assign ((size_t) size * 2, 0.0f);
    smoothMag.assign ((size_t) size / 2, 0.0f);
    db.assign ((size_t) size / 2, -200.0f);
    binHzValue = (float) (sampleRate / size);
}

float Analyzer::sampleChannel (int idx, int channel) const
{
    const float l = histL[(size_t) idx], r = histR[(size_t) idx];
    switch (channel)
    {
        case 1:  return l;
        case 2:  return r;
        case 3:  return 0.5f * (l - r);   // side
        default: return 0.5f * (l + r);   // mid / mono mix (what Web Audio's downmix does)
    }
}

void Analyzer::compute (const Settings& s)
{
    prepare (s.fftSize);
    const int n = currentSize;
    const int start = (writePos - n + historySize) & (historySize - 1);

    // time-domain stats over the same window, and windowed input for the FFT
    double sumSq = 0.0;
    float pk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float v = sampleChannel ((start + i) & (historySize - 1), s.channel);
        sumSq += (double) v * v;
        pk = juce::jmax (pk, std::abs (v));
        work[(size_t) i] = v * window[(size_t) i];
    }
    std::fill (work.begin() + n, work.end(), 0.0f);
    const double rms = std::sqrt (sumSq / n);
    rmsDb  = rms > 0 ? (float) (20.0 * std::log10 (rms) + 3.0103) : -200.0f;
    peakDb = pk  > 0 ? 20.0f * std::log10 (pk) : -200.0f;

    fft->performFrequencyOnlyForwardTransform (work.data(), true);

    const float tau = s.smoothing, inv = 1.0f / (float) n;
    for (int k = 0; k < n / 2; ++k)
    {
        auto& m = smoothMag[(size_t) k];
        m = tau * m + (1.0f - tau) * work[(size_t) k] * inv;
        db[(size_t) k] = m > 0 ? juce::jmax (-200.0f, 20.0f * std::log10 (m)) : -200.0f;
    }
}

Analyzer::Peak Analyzer::peak (float fmin, float fmax) const
{
    const int nb = numBins();
    const int ka = juce::jmax (2, (int) std::ceil (fmin / binHzValue));
    const int kb = juce::jmin (nb - 2, (int) std::floor (fmax / binHzValue));
    if (kb <= ka) return {};
    int k = ka;
    for (int i = ka; i <= kb; ++i) if (db[(size_t) i] > db[(size_t) k]) k = i;
    // parabolic interpolation on the dB values around the top bin
    const float a = db[(size_t) k - 1], b = db[(size_t) k], c = db[(size_t) k + 1];
    const float den = a - 2 * b + c;
    const float p = ! juce::exactlyEqual (den, 0.0f) ? juce::jlimit (-0.5f, 0.5f, 0.5f * (a - c) / den) : 0.0f;
    return { (k + p) * binHzValue, b - 0.25f * (a - c) * p + lineCal };
}
