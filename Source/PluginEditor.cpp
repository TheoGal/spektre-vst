#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_utils/juce_audio_utils.h>
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

SpektreEditor::SpektreEditor (SpektreProcessor& p)
    : AudioProcessorEditor (p), proc (p), settings (p.getSettings())
{
    setLookAndFeel (&lnf);

   #if JucePlugin_Build_Standalone
    // JUCE's standalone wrapper mutes the input by default to prevent feedback. Spektre's
    // standalone app has no output, so there is nothing to feed back: let the input through.
    if (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->getMuteInputValue().setValue (false);
   #endif
    addAndMakeVisible (view);

    addCombo (fftBox, fftLabel, "FFT size",
              { { 2048, "2048" }, { 4096, "4096" }, { 8192, "8192" }, { 16384, "16384" }, { 32768, "32768" } }, settings.fftSize);
    addCombo (modeBox, modeLabel, "Display",
              { { 100, "Line" }, { 3, "1/3 octave" }, { 6, "1/6 octave" }, { 12, "1/12 octave" } }, settings.mode == 0 ? 100 : settings.mode);
    addCombo (slopeBox, slopeLabel, "Slope",
              { { 1, "0 dB/oct" }, { 31, "3 dB/oct" }, { 46, "4.5 dB/oct" }, { 61, "6 dB/oct" } },
              1 + juce::roundToInt (settings.slope * 10));
    addCombo (peakBox, peakLabel, "Peak hold",
              { { 1, "Off" }, { 2, "Decay" }, { 3, "Max hold" } }, settings.peakHold + 1);
    addCombo (rangeBox, rangeLabel, "Range",
              { { 60, "0 to -60 dB" }, { 80, "0 to -80 dB" }, { 100, "0 to -100 dB" }, { 120, "0 to -120 dB" } }, -settings.floorDb);
    addCombo (chanBox, chanLabel, "Channel",
              { { 1, "L+R mix" }, { 2, "Left" }, { 3, "Right" } }, settings.channel + 1);

    smoothLabel.setFont (Fonts::label (13.0f).withExtraKerningFactor (0.09f));
    addAndMakeVisible (smoothLabel);
    smoothSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    smoothSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    smoothSlider.setRange (0.0, 0.95, 0.05);
    smoothSlider.setValue (settings.smoothing, juce::dontSendNotification);
    smoothSlider.onValueChange = [this] { settingsChanged(); };
    addAndMakeVisible (smoothSlider);

    sgButton.setClickingTogglesState (true);
    sgButton.setToggleState (settings.spectrogram, juce::dontSendNotification);
    sgButton.onClick = [this] { settingsChanged(); };
    addAndMakeVisible (sgButton);

    freezeButton.setClickingTogglesState (true);
    freezeButton.onClick = [this] { frozen = freezeButton.getToggleState(); };
    addAndMakeVisible (freezeButton);

    readouts[0] = { "Peak freq", "-", true };
    readouts[1] = { "Note", "-" };
    readouts[2] = { "Peak level", "-", true };
    readouts[3] = { "RMS", "-" };
    readouts[4] = { "Sample peak", "-" };
    readouts[5] = { "Resolution", "-" };

    view.setSettings (settings);
    view.setActive (true);
    settingsChanged();

    setResizable (true, true);
    setResizeLimits (640, 400, 2600, 1600);
    setSize (980, 580);
    lastReset = proc.resetCounter.load();
    startTimerHz (60);
}

SpektreEditor::~SpektreEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SpektreEditor::addCombo (juce::ComboBox& box, juce::Label& label, const juce::String& title,
                              std::initializer_list<std::pair<int, juce::String>> items, int selected)
{
    for (auto& [id, text] : items) box.addItem (text, id);
    box.setSelectedId (selected, juce::dontSendNotification);
    if (box.getSelectedId() == 0) box.setSelectedItemIndex (0, juce::dontSendNotification);
    box.onChange = [this] { settingsChanged(); };
    addAndMakeVisible (box);
    label.setText (title.toUpperCase(), juce::dontSendNotification);
    label.setFont (Fonts::label (13.0f).withExtraKerningFactor (0.09f));
    label.setJustificationType (juce::Justification::bottomLeft);
    addAndMakeVisible (label);
}

void SpektreEditor::settingsChanged()
{
    settings.fftSize     = fftBox.getSelectedId();
    settings.mode        = modeBox.getSelectedId() == 100 ? 0 : modeBox.getSelectedId();
    settings.slope       = (slopeBox.getSelectedId() - 1) / 10.0f;
    settings.peakHold    = peakBox.getSelectedId() - 1;
    settings.floorDb     = -rangeBox.getSelectedId();
    settings.channel     = chanBox.getSelectedId() - 1;
    settings.smoothing   = (float) smoothSlider.getValue();
    settings.spectrogram = sgButton.getToggleState();
    smoothLabel.setText ("SMOOTHING  " + juce::String (settings.smoothing, 2), juce::dontSendNotification);
    proc.setSettings (settings);
    view.setSettings (settings);
    readouts[5].value = juce::String (analyzer.getSampleRate() / settings.fftSize, 2) + " Hz  "
                        + juce::String (juce::roundToInt (1000.0 * settings.fftSize / analyzer.getSampleRate())) + " ms";
    repaint (readoutArea);
}

bool SpektreEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey) { freezeButton.triggerClick(); return true; }
    return false;
}

void SpektreEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = lastTick > 0 ? (float) juce::jmin (0.1, now - lastTick) : 0.0f;
    lastTick = now;

    // stream restarted or sample rate changed: start clean
    const auto rc = proc.resetCounter.load();
    if (rc != lastReset) { lastReset = rc; analyzer.reset(); view.resetHold(); }
    const double sr = proc.sampleRate.load();
    if (! juce::exactlyEqual (sr, analyzer.getSampleRate())) { analyzer.setSampleRate (sr); settingsChanged(); }

    proc.fifo.pop ([this] (const float* l, const float* r, int n) { analyzer.push (l, r, n); });
    if (frozen) return;

    analyzer.compute (settings);
    view.update (analyzer, dt, frozen);

    if (now - lastReadout > 0.125)
    {
        lastReadout = now;
        const auto pk = analyzer.peak (20.0f, (float) juce::jmin (20000.0, sr * 0.5));
        const bool ok = pk.dbfs > settings.floorDb + 3;
        readouts[0].value = ok ? SpectrumView::fmtFreq (pk.freq) : "-";
        readouts[1].value = ok ? SpectrumView::noteOf (pk.freq) : "-";
        readouts[2].value = ok ? SpectrumView::fmtDb (pk.dbfs) : "-";
        readouts[3].value = SpectrumView::fmtDb (analyzer.rmsDbfs());
        readouts[4].value = SpectrumView::fmtDb (analyzer.samplePeakDbfs());
        repaint (readoutArea);
    }
}

void SpektreEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::bg);

    // header
    auto h = headerArea;
    g.setFont (Fonts::label (24.0f, true).withExtraKerningFactor (0.12f));
    g.setColour (Palette::trace);
    const juce::String title = "SPEKTRE";
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);
    g.drawText (title, h, juce::Justification::centredLeft);
    g.setFont (Fonts::label (13.0f).withExtraKerningFactor (0.06f));
    g.setColour (Palette::muted);
    g.drawText ("SPECTRUM ANALYZER", h.withTrimmedLeft (juce::roundToInt (tw) + 12).translated (0, 2), juce::Justification::centredLeft);

    // readout strip
    auto r = readoutArea.toFloat();
    g.setColour (Palette::edge);
    g.fillRoundedRectangle (r, 6.0f);
    const float cw = (r.getWidth() - 2) / (float) readouts.size();
    for (size_t i = 0; i < readouts.size(); ++i)
    {
        auto c = juce::Rectangle<float> (r.getX() + 1 + i * cw + (i ? 0.5f : 0.0f), r.getY() + 1, cw - (i ? 0.5f : 0.0f), r.getHeight() - 2);
        g.setColour (Palette::panel);
        g.fillRect (c);
        auto t = c.reduced (10, 5);
        g.setColour (Palette::muted);
        g.setFont (Fonts::label (12.0f).withExtraKerningFactor (0.09f));
        g.drawText (readouts[i].title.toUpperCase(), t.removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (readouts[i].key ? Palette::trace : Palette::fg);
        g.setFont (Fonts::mono (14.5f, true));
        g.drawText (readouts[i].value, t, juce::Justification::centredLeft, true);
    }
    g.setColour (Palette::edge);
    g.drawRoundedRectangle (view.getBounds().toFloat().expanded (0.5f), 6.0f, 1.0f);
}

void SpektreEditor::resized()
{
    auto r = getLocalBounds().reduced (12, 8);
    headerArea = r.removeFromTop (30);
    r.removeFromTop (6);
    readoutArea = r.removeFromTop (44);
    r.removeFromTop (8);

    // settings row (wraps to two rows when narrow)
    const int ctlH = 28, labH = 16;
    struct Item { juce::Component* c; juce::Label* l; int w; };
    std::vector<Item> items = {
        { &fftBox, &fftLabel, 90 }, { &modeBox, &modeLabel, 116 }, { &slopeBox, &slopeLabel, 104 },
        { &peakBox, &peakLabel, 100 }, { &rangeBox, &rangeLabel, 124 }, { &chanBox, &chanLabel, 112 },
        { &smoothSlider, &smoothLabel, 130 }, { &sgButton, nullptr, 136 }, { &freezeButton, nullptr, 96 } };
    const int gap = 14;
    int total = 0;
    for (auto& it : items) total += it.w + gap;
    const int rows = total - gap > r.getWidth() ? 2 : 1;
    auto controls = r.removeFromBottom (rows * (ctlH + labH) + (rows - 1) * 8);
    r.removeFromBottom (10);
    view.setBounds (r);

    int x = controls.getX(), y = controls.getY();
    for (auto& it : items)
    {
        if (x + it.w > controls.getRight()) { x = controls.getX(); y += ctlH + labH + 8; }
        if (it.l) it.l->setBounds (x, y, it.w + 10, labH);
        it.c->setBounds (x, y + labH, it.w, ctlH);
        x += it.w + gap;
    }
}
