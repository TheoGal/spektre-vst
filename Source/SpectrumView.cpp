#include "SpectrumView.h"
#include "SpektreLookAndFeel.h"

static constexpr float fMin = 20.0f;

SpectrumView::SpectrumView()
{
    // spectrogram colour map: ink -> navy -> magenta -> amber -> cream
    const struct { float t; juce::uint32 c; } stops[] = {
        { 0.00f, 0xff0a0d13 }, { 0.28f, 0xff182656 }, { 0.52f, 0xff8c2c76 }, { 0.78f, 0xffffa03c }, { 1.00f, 0xfffff6e0 } };
    for (int i = 0; i < 256; ++i)
    {
        const float t = i / 255.0f;
        int j = 0;
        while (j < 3 && t > stops[j + 1].t) ++j;
        const float u = (t - stops[j].t) / (stops[j + 1].t - stops[j].t);
        sgLut[i] = juce::Colour (stops[j].c).interpolatedWith (juce::Colour (stops[j + 1].c), u);
    }
    setOpaque (true);
}

/* ---------- formatting ---------- */
juce::String SpectrumView::fmtFreq (float f)
{
    if (f < 1000) return juce::String (f, f < 100 ? 2 : 1) + " Hz";
    return juce::String (f / 1000, f < 10000 ? 3 : 2) + " kHz";
}

juce::String SpectrumView::fmtDb (float d)
{
    if (d <= -200 || ! std::isfinite (d)) return juce::String::fromUTF8 ("-\xe2\x88\x9e dB");
    return (d < 0 ? "-" : "+") + juce::String (std::abs (d), 1) + " dB";
}

juce::String SpectrumView::noteOf (float f)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (f <= 0) return "-";
    const double n = 12 * std::log2 (f / 440.0) + 69;
    const int r = (int) std::round (n), c = (int) std::round ((n - r) * 100);
    if (r < 0 || r > 135) return "-";
    return juce::String (names[r % 12]) + juce::String (r / 12 - 1) + " " + (c >= 0 ? "+" : "-")
           + juce::String (std::abs (c)) + juce::String::fromUTF8 ("\xc2\xa2");
}

/* ---------- settings / layout ---------- */
void SpectrumView::setSettings (const Settings& s)
{
    const bool sgChanged = s.spectrogram != settings.spectrogram;
    const bool modeChanged = s.mode != settings.mode;
    settings = s;
    buildBands();
    if (sgChanged || modeChanged) layoutPlot();
    resetHold();
    repaint();
}

void SpectrumView::resized() { layoutPlot(); }

void SpectrumView::layoutPlot()
{
    auto r = getLocalBounds();
    if (settings.spectrogram)
    {
        sgArea = r.removeFromBottom (juce::roundToInt (r.getHeight() * 0.38f));
        sgArea.removeFromTop (1);   // divider
    }
    else sgArea = {};
    plotArea = r;

    const bool narrow = getWidth() < 520;
    pl = narrow ? 34.0f : 42.0f; pr = 10; pt = 10; pb = 22;
    pw = juce::jmax (1.0f, plotArea.getWidth() - pl - pr);
    ph = juce::jmax (1.0f, plotArea.getHeight() - pt - pb);
    nPix = juce::jmax (2, juce::roundToInt (pw));

    const size_t n = settings.mode == 0 ? (size_t) nPix + 1 : bands.size();
    disp.assign (n, -999.0f); hold.assign (n, -999.0f); lowRes.assign (n, 0);

    if (! sgArea.isEmpty())
    {
        sgImage = juce::Image (juce::Image::RGB, juce::jmax (1, sgArea.getWidth()), juce::jmax (1, sgArea.getHeight()), true);
        juce::Graphics g (sgImage);
        g.fillAll (Palette::bg);
    }
    else sgImage = {};
}

void SpectrumView::buildBands()
{
    bands.clear();
    if (settings.mode == 0) return;
    const float n = (float) settings.mode, h = std::pow (2.0f, 1.0f / (2.0f * n));
    for (int k = -200; k < 200; ++k)
    {
        const float fc = 1000.0f * std::pow (2.0f, k / n);
        if (fc < fMin || fc > fmax) continue;
        bands.push_back ({ fc, fc / h, juce::jmin (fc * h, sampleRate * 0.5f) });
    }
    disp.assign (bands.size(), -999.0f); hold.assign (bands.size(), -999.0f); lowRes.assign (bands.size(), 0);
}

float SpectrumView::xOfF (float f) const  { return plotArea.getX() + pl + pw * std::log (f / fMin) / std::log (fmax / fMin); }
float SpectrumView::fOfX (float x) const  { return fMin * std::pow (fmax / fMin, (x - plotArea.getX() - pl) / pw); }
float SpectrumView::yOfDb (float d) const { return plotArea.getY() + pt + ph * juce::jlimit (0.0f, 1.0f, d / (float) settings.floorDb); }
float SpectrumView::slopeAt (float f) const { return ! juce::exactlyEqual (settings.slope, 0.0f) ? settings.slope * std::log2 (f / 1000.0f) : 0.0f; }

void SpectrumView::drawGrid (juce::Graphics& g)
{
    if (plotArea.isEmpty()) return;
    const bool narrow = getWidth() < 520;
    const float top = plotArea.getY() + pt, bottom = top + ph, left = plotArea.getX() + pl;

    // frequency lines
    for (float dec = 10; dec <= 10000; dec *= 10)
        for (int m = 1; m < 10; ++m)
        {
            const float f = dec * m;
            if (f < fMin || f > fmax) continue;
            g.setColour (Palette::edge.withAlpha (m == 1 ? 1.0f : 0.45f));
            g.drawVerticalLine (juce::roundToInt (xOfF (f)), top, bottom);
        }
    g.setFont (Fonts::mono (11.0f));
    g.setColour (Palette::muted);
    const std::vector<int> labs = narrow ? std::vector<int> { 50, 100, 500, 1000, 5000, 10000 }
                                         : std::vector<int> { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (int f : labs)
    {
        if (f > fmax) continue;
        const float x = juce::jlimit (left + 8.0f, (float) getWidth() - 14.0f, xOfF ((float) f));
        g.drawText (f >= 1000 ? juce::String (f / 1000) + "k" : juce::String (f),
                    juce::Rectangle<float> (x - 20, bottom + 4, 40, 14), juce::Justification::centred);
    }
    // level lines
    const int lab = (ph < 220 || -settings.floorDb > 100) ? 20 : 10;
    for (int d = 0; d >= settings.floorDb; d -= 10)
    {
        const float y = yOfDb ((float) d);
        g.setColour (Palette::edge.withAlpha (d % lab == 0 ? 1.0f : 0.45f));
        g.drawHorizontalLine (juce::roundToInt (y), left, left + pw);
        if (d % lab == 0)
        {
            g.setColour (Palette::muted);
            g.drawText (juce::String (d), juce::Rectangle<float> (plotArea.getX() + 0.0f, juce::jmax (top, y - 7), pl - 6, 14),
                        juce::Justification::centredRight);
        }
    }
    g.setColour (Palette::muted);
    juce::String cap = "dBFS";
    if (! juce::exactlyEqual (settings.slope, 0.0f)) cap << "  " << juce::String (settings.slope, juce::exactlyEqual (settings.slope, std::round (settings.slope)) ? 0 : 1) << " dB/oct";
    g.drawText (cap, juce::Rectangle<float> (left + 6, top, 200, 16), juce::Justification::centredLeft);
}

/* ---------- sampling the spectrum ---------- */
float SpectrumView::binAt (int i) const { return dbw[(size_t) juce::jlimit (0, (int) dbw.size() - 1, i)]; }

float SpectrumView::interp (float x) const
{
    const int i = (int) std::floor (x);
    const float t = x - i;
    const float p0 = binAt (i - 1), p1 = binAt (i), p2 = binAt (i + 1), p3 = binAt (i + 2);
    const float v = 0.5f * (2 * p1 + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
    return juce::jmax (-200.0f, juce::jmin (juce::jmax (p1, p2), v));
}

// max over bins whose centres fall in [lo, hi]; cubic interpolation where bins are sparser than pixels
float SpectrumView::sampleDb (float lo, float hi) const
{
    const float b0 = lo / binHz, b1 = hi / binHz;
    if (b1 - b0 < 1) return interp ((b0 + b1) * 0.5f);
    const int a = juce::jmax (1, (int) std::ceil (b0)), b = juce::jmin ((int) dbw.size() - 1, (int) std::floor (b1));
    if (b < a) return interp ((b0 + b1) * 0.5f);
    float m = -200;
    for (int k = a; k <= b; ++k) m = juce::jmax (m, dbw[(size_t) k]);
    return m;
}

void SpectrumView::computeSeries (float dt)
{
    if (settings.mode == 0)
    {
        disp.resize ((size_t) nPix + 1);
        const float x0 = plotArea.getX() + pl, half = 0.5f * pw / nPix;
        for (int i = 0; i <= nPix; ++i)
        {
            const float x = x0 + i * pw / nPix;
            disp[(size_t) i] = sampleDb (fOfX (x - half), fOfX (x + half)) + Analyzer::lineCal + slopeAt (fOfX (x));
        }
    }
    else
    {
        disp.resize (bands.size()); lowRes.resize (bands.size());
        for (size_t j = 0; j < bands.size(); ++j)
        {
            const auto& B = bands[j];
            const int a = (int) std::ceil (B.lo / binHz), b = (int) std::ceil (B.hi / binHz) - 1;
            const float tilt = slopeAt (B.fc);
            if (b - a + 1 < 3) { disp[j] = interp (B.fc / binHz) + Analyzer::lineCal + tilt; lowRes[j] = 1; continue; }
            double p = 0;
            for (int k = a; k <= b && k < (int) dbw.size(); ++k) p += std::pow (10.0, dbw[(size_t) k] / 10.0);
            disp[j] = p > 0 ? (float) (10.0 * std::log10 (p)) + Analyzer::bandCal + tilt : -200.0f;
            lowRes[j] = 0;
        }
    }
    if (hold.size() != disp.size()) hold.assign (disp.size(), -999.0f);
    if (settings.peakHold != 0)
    {
        const float dec = settings.peakHold == 1 ? 20.0f * dt : 0.0f;   // decay: 20 dB per second
        for (size_t i = 0; i < disp.size(); ++i) hold[i] = juce::jmax (disp[i], hold[i] - dec);
    }
}

void SpectrumView::resetHold() { std::fill (hold.begin(), hold.end(), -999.0f); }

void SpectrumView::update (const Analyzer& a, float dt, bool frozen)
{
    if (frozen) return;
    const float newSr = (float) a.getSampleRate();
    const float newMax = juce::jmin (20000.0f, newSr * 0.5f);
    if (! juce::exactlyEqual (newSr, sampleRate) || ! juce::exactlyEqual (newMax, fmax))
    {
        sampleRate = newSr; fmax = newMax;
        buildBands(); layoutPlot();
    }
    dbw = a.bins();
    binHz = a.binHz();
    computeSeries (dt);
    peakInfo = a.peak (fMin, fmax);
    if (! sgArea.isEmpty()) drawSpectrogramColumn();
    repaint();
}

juce::Colour SpectrumView::sgColour (float t) const { return sgLut[juce::jlimit (0, 255, juce::roundToInt (t * 255))]; }

void SpectrumView::drawSpectrogramColumn()
{
    if (! sgImage.isValid()) return;
    const int w = sgImage.getWidth(), h = sgImage.getHeight();
    const int x0 = juce::roundToInt (pl), x1 = w - juce::roundToInt (pr);
    if (x1 - x0 < 4) return;
    sgImage.moveImageSection (x0, 0, x0 + 1, 0, x1 - x0 - 1, h);
    const float lr = std::log (fmax / fMin);
    juce::Image::BitmapData bd (sgImage, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h; ++y)
    {
        const float a = (float) (h - 1 - y) / h, b = (float) (h - y) / h;
        const float flo = fMin * std::exp (lr * a), fhi = fMin * std::exp (lr * b);
        const float v = sampleDb (flo, fhi) + Analyzer::lineCal + slopeAt (std::sqrt (flo * fhi));
        bd.setPixelColour (x1 - 1, y, sgColour (1.0f - v / (float) settings.floorDb));
    }
}

/* ---------- paint ---------- */
void SpectrumView::paint (juce::Graphics& g)
{
    g.fillAll (Palette::panel);
    drawGrid (g);

    // spectrogram
    if (! sgArea.isEmpty() && sgImage.isValid())
    {
        g.setColour (Palette::edge);
        g.drawHorizontalLine (sgArea.getY() - 1, 0.0f, (float) getWidth());
        g.drawImageAt (sgImage, sgArea.getX(), sgArea.getY());
        g.setColour (Palette::panel);
        g.fillRect (sgArea.withWidth (juce::roundToInt (pl)));
        g.setFont (Fonts::mono (11.0f));
        g.setColour (Palette::muted);
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            const float y = sgArea.getY() + sgArea.getHeight() * (1.0f - std::log (f / fMin) / std::log (fmax / fMin));
            if (y > sgArea.getY() + 8 && y < sgArea.getBottom() - 6)
                g.drawText (f >= 1000 ? juce::String ((int) f / 1000) + "k" : juce::String ((int) f),
                            juce::Rectangle<float> (0, y - 7, pl - 6, 14), juce::Justification::centredRight);
        }
    }

    const float left = plotArea.getX() + pl, top = plotArea.getY() + pt, bottom = top + ph;
    if (active && ! disp.empty())
    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (juce::Rectangle<float> (left, top, pw, ph).toNearestInt());
        if (settings.mode == 0)
        {
            juce::Path line;
            for (int i = 0; i <= nPix && i < (int) disp.size(); ++i)
            {
                const float x = left + i * pw / nPix, y = yOfDb (disp[(size_t) i]);
                i ? line.lineTo (x, y) : line.startNewSubPath (x, y);
            }
            juce::Path fill (line);
            fill.lineTo (left + pw, bottom); fill.lineTo (left, bottom); fill.closeSubPath();
            g.setGradientFill (juce::ColourGradient (Palette::trace.withAlpha (0.32f), 0, top, Palette::trace.withAlpha (0.02f), 0, bottom, false));
            g.fillPath (fill);
            g.setColour (Palette::trace);
            g.strokePath (line, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
            if (settings.peakHold != 0)
            {
                juce::Path hp;
                for (int i = 0; i <= nPix && i < (int) hold.size(); ++i)
                {
                    const float x = left + i * pw / nPix, y = yOfDb (hold[(size_t) i]);
                    i ? hp.lineTo (x, y) : hp.startNewSubPath (x, y);
                }
                g.setColour (Palette::hold.withAlpha (0.75f));
                g.strokePath (hp, juce::PathStrokeType (1.0f));
            }
        }
        else
        {
            for (size_t j = 0; j < bands.size() && j < disp.size(); ++j)
            {
                const auto& B = bands[j];
                const float x0 = xOfF (B.lo), x1 = xOfF (juce::jmin (B.hi, fmax)), gap = juce::jmax (1.0f, (x1 - x0) * 0.14f);
                const float y = yOfDb (disp[j]);
                g.setColour (lowRes[j] ? Palette::trace.withAlpha (0.35f) : Palette::trace);
                g.fillRect (juce::Rectangle<float> (x0 + gap / 2, y, juce::jmax (1.0f, x1 - x0 - gap), bottom - y));
                if (settings.peakHold != 0 && hold[j] > settings.floorDb)
                {
                    g.setColour (Palette::hold);
                    g.fillRect (juce::Rectangle<float> (x0 + gap / 2, yOfDb (hold[j]) - 1, juce::jmax (1.0f, x1 - x0 - gap), 2.0f));
                }
            }
        }
        // peak marker
        if (peakInfo.dbfs > settings.floorDb + 3)
        {
            const float x = xOfF (peakInfo.freq), y = yOfDb (peakInfo.dbfs + slopeAt (peakInfo.freq));
            juce::Path tri;
            tri.addTriangle (x, y - 9, x - 4, y - 15, x + 4, y - 15);
            g.setColour (Palette::fg);
            g.fillPath (tri);
        }
    }

    // cursor readout
    if (cursorX)
    {
        const float x = juce::jlimit (left, left + pw, *cursorX), f = fOfX (x);
        std::optional<float> v, tv;
        juce::String label;
        if (active && ! disp.empty())
        {
            if (settings.mode == 0)
            {
                const int i = juce::jlimit (0, (int) disp.size() - 1, juce::roundToInt ((x - left) / pw * nPix));
                v = disp[(size_t) i]; tv = *v - slopeAt (f);
            }
            else
                for (size_t j = 0; j < bands.size(); ++j)
                    if (f >= bands[j].lo && f < bands[j].hi) { v = disp[j]; tv = *v - slopeAt (bands[j].fc); label = fmtFreq (bands[j].fc) + " band"; break; }
        }
        if (label.isEmpty()) label = fmtFreq (f);
        if (tv) label << "  " << fmtDb (*tv);
        if (settings.mode == 0) label << "  " << noteOf (f);

        const float dash[] = { 3.0f, 3.0f };
        g.setColour (Palette::fg.withAlpha (0.5f));
        g.drawDashedLine ({ x, top, x, bottom }, dash, 2);
        if (v && *v > settings.floorDb)
        {
            const float y = yOfDb (*v);
            g.drawDashedLine ({ left, y, left + pw, y }, dash, 2);
        }
        const auto font = Fonts::mono (12.0f, true);
        g.setFont (font);
        const float tw = juce::GlyphArrangement::getStringWidth (font, label) + 14;
        const float bx = juce::jlimit (left + 4, (float) getWidth() - tw - 6, x + 8), by = top + 6;
        g.setColour (Palette::panel.withAlpha (0.92f));
        g.fillRect (bx, by, tw, 22.0f);
        g.setColour (Palette::edge);
        g.drawRect (bx, by, tw, 22.0f);
        g.setColour (Palette::fg);
        g.drawText (label, juce::Rectangle<float> (bx + 7, by, tw - 7, 22), juce::Justification::centredLeft);
    }
}

void SpectrumView::mouseMove (const juce::MouseEvent& e)
{
    if (plotArea.contains (e.getPosition())) cursorX = (float) e.x; else cursorX.reset();
    repaint();
}
void SpectrumView::mouseDrag (const juce::MouseEvent& e) { mouseMove (e); }
void SpectrumView::mouseExit (const juce::MouseEvent&)   { cursorX.reset(); repaint(); }
