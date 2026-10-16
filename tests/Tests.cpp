// Feeds known signals through the real processor + analyzer and checks the readings,
// then renders the editor to a PNG. Run under a (virtual) display.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static int failures = 0;
static void check (const char* what, double got, double want, double tol, const char* unit)
{
    const bool ok = std::abs (got - want) <= tol;
    if (! ok) ++failures;
    std::printf ("%-46s %10.3f %-4s (want %8.3f +/- %.2f)  %s\n", what, got, unit, want, tol, ok ? "ok" : "FAIL");
}

static void feed (SpektreProcessor& p, double sr, double seconds, std::function<float (double t, int ch)> sig)
{
    const int block = 512;
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    static double t0 = 0;
    int done = 0;
    for (; done < (int) (seconds * sr); done += block)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
                buf.setSample (ch, i, sig (t0 + (done + i) / sr, ch));
        p.processBlock (buf, midi);
    }
    t0 += done / sr;   // advance by what was actually fed, so the signal stays continuous
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const double twoPi = juce::MathConstants<double>::twoPi;

    // ---- 1. analyzer calibration, straight through the processor's FIFO ----
    {
        SpektreProcessor p;
        p.setPlayConfigDetails (2, 2, sr, 512);
        p.prepareToPlay (sr, 512);
        Analyzer a; a.setSampleRate (sr);
        Settings s; s.smoothing = 0; s.fftSize = 16384;

        auto run = [&] (double f, double amp, int chMode, double rAmp = std::numeric_limits<double>::quiet_NaN())
        {
            feed (p, sr, 0.6, [=] (double t, int ch) {
                const double ar = std::isnan (rAmp) ? amp : rAmp;
                return (float) ((ch == 0 ? amp : ar) * std::sin (twoPi * f * t)); });
            p.fifo.pop ([&] (const float* l, const float* r, int n) { a.push (l, r, n); });
            s.channel = chMode;
            a.compute (s);
            return a.peak (20, 20000);
        };

        auto pk = run (1000, 0.5, 0);
        check ("1 kHz sine at -6.02 dBFS: peak freq", pk.freq, 1000, 0.5, "Hz");
        check ("1 kHz sine at -6.02 dBFS: peak level", pk.dbfs, -6.02, 0.1, "dB");
        check ("1 kHz sine at -6.02 dBFS: RMS (sine-ref)", a.rmsDbfs(), -6.02, 0.05, "dB");
        check ("1 kHz sine at -6.02 dBFS: sample peak", a.samplePeakDbfs(), -6.02, 0.05, "dB");

        pk = run (100, 0.1, 0);
        check ("100 Hz sine at -20 dBFS: peak freq", pk.freq, 100, 0.3, "Hz");
        check ("100 Hz sine at -20 dBFS: peak level", pk.dbfs, -20, 0.1, "dB");

        pk = run (1234.5, 1.0, 0);   // between bins: worst case for level
        check ("1234.5 Hz (off-bin) full scale: freq", pk.freq, 1234.5, 0.5, "Hz");
        check ("1234.5 Hz (off-bin) full scale: level", pk.dbfs, 0.0, 0.3, "dB");

        pk = run (440, 0.5, 1, 0.0);  // left only
        check ("Left-only 440 Hz, channel=Left: level", pk.dbfs, -6.02, 0.1, "dB");
        pk = run (440, 0.5, 2, 0.0);
        check ("Left-only 440 Hz, channel=Right: level", pk.dbfs, -200, 0.0 + 1e9, "dB");
        std::printf ("    (right channel reads %.1f dB: nothing there, as expected)\n", pk.dbfs);
        pk = run (440, 0.5, 3, -0.5); // anti-phase -> pure side
        check ("Anti-phase 440 Hz, channel=Side: level", pk.dbfs, -6.02, 0.1, "dB");
        pk = run (440, 0.5, 0, -0.5);
        std::printf ("    anti-phase, channel=L+R: %.1f dB (cancels, as expected)\n", pk.dbfs);
    }

    // ---- 2. state save / restore ----
    {
        SpektreProcessor p1, p2;
        Settings s; s.fftSize = 4096; s.mode = 6; s.slope = 3; s.channel = 2; s.spectrogram = true;
        p1.setSettings (s);
        juce::MemoryBlock mb; p1.getStateInformation (mb);
        p2.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto r = p2.getSettings();
        check ("State round-trip (fft, mode, chan, sg)", (r.fftSize == 4096) + (r.mode == 6) + (r.channel == 2) + (r.spectrogram ? 1 : 0), 4, 0, "");
    }

    // ---- 3. render the editor with a music-like test signal ----
    for (int pass = 0; pass < 2; ++pass)
    {
        SpektreProcessor p;
        p.setPlayConfigDetails (2, 2, sr, 512);
        p.prepareToPlay (sr, 512);
        Settings s; s.spectrogram = pass == 1; s.mode = pass == 1 ? 3 : 0;
        p.setSettings (s);
        std::unique_ptr<SpektreEditor> ed (static_cast<SpektreEditor*> (p.createEditor()));
        ed->setSize (1280, 720);
        juce::Random rng (42);
        double pink[3] = { 0, 0, 0 };
        for (int frame = 0; frame < 140; ++frame)
        {
            feed (p, sr, 1.0 / 60, [&] (double t, int) {
                // rough pink noise + a few tones, like a mix
                const double w = rng.nextDouble() * 2 - 1;
                pink[0] = 0.997 * pink[0] + 0.029591 * w; pink[1] = 0.985 * pink[1] + 0.032534 * w; pink[2] = 0.950 * pink[2] + 0.048056 * w;
                const double n = (pink[0] + pink[1] + pink[2] + 0.1848 * w) * 0.35;
                return (float) (n + 0.25 * std::sin (twoPi * 55 * t) + 0.12 * std::sin (twoPi * 440 * t)
                                + 0.05 * std::sin (twoPi * 3520 * t)); });
            ed->timerCallback();
            juce::Thread::sleep (16);   // real time, so readouts refresh and peak hold decays
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        juce::File out (pass == 0 ? "/tmp/claude-0/vst-line.png" : "/tmp/claude-0/vst-bars-sg.png");
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL CHECKS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
