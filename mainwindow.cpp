#include "mainwindow.h"
#include "./ui_mainwindow.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QByteArray>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
#include <QAudioSink>
#include <QTextToSpeechEngine>

#include "piper/piper.hpp"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int sampleRate = 22050;    // For example, or use pVoice.synthesisConfig.sampleRate
    int channelCount = 1;      // For example, or use pVoice.synthesisConfig.channels
    int sampleSize = 16;       // bits per sample (pVoice.synthesisConfig.sampleWidth)

    const std::string modelPath = "/path/to/piper/model";


    piper::PiperConfig pConf;
    pConf.eSpeakDataPath = "";
    pConf.useESpeak = true;

    piper::Voice pVoice;
    std::optional<piper::SpeakerId> speakerId;
    piper::loadVoice(pConf, "en_US-lessac-high.onnx", "en_US-lessac-high.onnx.json", pVoice,speakerId, false);

    piper::SynthesisResult result = {};


    piper::initialize(pConf);
    // std::ofstream audioFile("output.wav", std::ios::binary);

    std::vector<int16_t> audioBuffer;

    piper::textToAudio(pConf,pVoice, "Hello ali", audioBuffer,result, nullptr);

    QByteArray audioData(reinterpret_cast<const char*>(audioBuffer.data()),
                         static_cast<int>(audioBuffer.size() * sizeof(int16_t)));

    // Use a QBuffer to wrap the data for streaming playback.
    QBuffer *audioBufferQ = new QBuffer;
    audioBufferQ->setData(audioData);

    QAudioFormat format;
    format.setSampleRate(22050); // or pVoice.synthesisConfig.sampleRate
    format.setChannelCount(1);   // or pVoice.synthesisConfig.channels
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice inputDevice = QMediaDevices::defaultAudioOutput();

    // QAudioDevice info(QMediaDevices::defaultAudioOutput());
    // if (!info.isFormatSupported(format)) {
    //     qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    // }

    // auto audio = new QAudioSink(format, this);
    // audio->start(audioBufferQ);

    // Populate engine selection list
    ui->engine->addItem("Default", "default");
    const auto engines = QTextToSpeech::availableEngines();
    for (const QString &engine : engines)
        ui->engine->addItem(engine, engine);
    ui->engine->setCurrentIndex(0);
    engineSelected(0);

    connect(ui->pitch, &QSlider::valueChanged, this, &MainWindow::setPitch);
    connect(ui->rate, &QSlider::valueChanged, this, &MainWindow::setRate);
    connect(ui->volume, &QSlider::valueChanged, this, &MainWindow::setVolume);
    connect(ui->engine, &QComboBox::currentIndexChanged, this, &MainWindow::engineSelected);
    connect(ui->language, &QComboBox::currentIndexChanged, this, &MainWindow::languageSelected);
    connect(ui->voice, &QComboBox::currentIndexChanged, this, &MainWindow::voiceSelected);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setRate(int rate)
{
m_speech->setRate(rate / 10.0);
}

void MainWindow::setPitch(int pitch)
{
m_speech->setPitch(pitch / 10.0);
}

void MainWindow::setVolume(int volume)
{
m_speech->setVolume(volume / 100.0);
}

void MainWindow::engineSelected(int index)
{
    const QString engineName = ui->engine->itemData(index).toString();

    delete m_speech;
    m_speech = engineName == u"default"
                   ? new QTextToSpeech(this)
                   : new QTextToSpeech(engineName, this);

    // some engines initialize asynchronously
    if (m_speech->state() == QTextToSpeech::Ready) {
        onEngineReady();
    } else {
        connect(m_speech, &QTextToSpeech::stateChanged, this, &MainWindow::onEngineReady,
                Qt::SingleShotConnection);
    }
}

void MainWindow::languageSelected(int language)
{
    QLocale locale = ui->language->itemData(language).toLocale();
    m_speech->setLocale(locale);
}

void MainWindow::voiceSelected(int index)
{
    m_speech->setVoice(m_voices.at(index));
}

void MainWindow::stateChanged(QTextToSpeech::State state)
{
    switch (state) {
    case QTextToSpeech::Speaking:
        ui->statusbar->showMessage(tr("Speech started..."));
        break;
    case QTextToSpeech::Ready:
        ui->statusbar->showMessage(tr("Speech stopped..."), 2000);
        break;
    case QTextToSpeech::Paused:
        ui->statusbar->showMessage(tr("Speech paused..."));
        break;
    default:
        ui->statusbar->showMessage(tr("Speech error!"));
        break;
    }

    ui->pauseButton->setEnabled(state == QTextToSpeech::Speaking);
    ui->resumeButton->setEnabled(state == QTextToSpeech::Paused);
    ui->stopButton->setEnabled(state == QTextToSpeech::Speaking || state == QTextToSpeech::Paused);
}

void MainWindow::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->language->setCurrentIndex(ui->language->findData(localeVariant));

    QSignalBlocker blocker(ui->voice);

    ui->voice->clear();

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    for (const QVoice &voice : std::as_const(m_voices)) {
        ui->voice->addItem( QString("%1 - %2 - %3")
                              .arg(voice.name(), QVoice::genderName(voice.gender()),
                                   QVoice::ageName(voice.age())));
        if (voice.name() == currentVoice.name())
            ui->voice->setCurrentIndex(ui->voice->count() - 1);
    }
}

void MainWindow::onEngineReady()
{
    if (m_speech->state() != QTextToSpeech::Ready) {
        stateChanged(m_speech->state());
        return;
    }

    const bool hasPauseResume = m_speech->engineCapabilities()
                                & QTextToSpeech::Capability::PauseResume;
    ui->pauseButton->setVisible(hasPauseResume);
    ui->resumeButton->setVisible(hasPauseResume);

    // Block signals of the languages combobox while populating
    QSignalBlocker blocker(ui->language);

    ui->language->clear();
    const QList<QLocale> locales = m_speech->availableLocales();
    QLocale current = m_speech->locale();
    for (const QLocale &locale : locales) {
        QString name = QString("%1 (%2)").arg(QLocale::languageToString(locale.language()),
                              QLocale::territoryToString(locale.territory()));
        QVariant localeVariant(locale);
        ui->language->addItem(name, localeVariant);
        if (locale.name() == current.name())
            current = locale;
    }
    setRate(ui->rate->value());
    setPitch(ui->pitch->value());
    setVolume(ui->volume->value());
    //! [say]
    connect(ui->speakButton, &QPushButton::clicked, m_speech, [this]{
        m_speech->say(ui->txtToSpeach->toPlainText());
    });
    //! [say]
    //! [stop]
    connect(ui->stopButton, &QPushButton::clicked, m_speech, [this]{
        m_speech->stop();
    });
    //! [stop]
    //! [pause]
    connect(ui->pauseButton, &QPushButton::clicked, m_speech, [this]{
        m_speech->pause();
    });
    //! [pause]
    //! [resume]
    connect(ui->resumeButton, &QPushButton::clicked, m_speech, &QTextToSpeech::resume);
    //! [resume]

    connect(m_speech, &QTextToSpeech::stateChanged, this, &MainWindow::stateChanged);
    connect(m_speech, &QTextToSpeech::localeChanged, this, &MainWindow::localeChanged);

    blocker.unblock();

    localeChanged(current);
}

