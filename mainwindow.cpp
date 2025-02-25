#include "mainwindow.h"
#include "./ui_mainwindow.h"


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

#include "piper/piper.hpp"


// audio recorder:
#include <QAudioInput>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QStandardPaths>
#include <QStatusBar>
#include <QDebug>
#include <iostream>
#include <QDateTime>

#if QT_CONFIG(permissions)
#include <QPermission>
#endif

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_model = new LlamaInterface();
    m_model->loadModel("/extra/jan/models/llama3.1-8b-instruct/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf");

    m_thread = new QThread();
    m_model->moveToThread(m_thread);
    m_thread->start();


    connect(m_model, &LlamaInterface::answerReady, this, [this](QString c)
    {
        ui->txtToSpeach->insertPlainText(c);
    });
    m_devices = new QMediaDevices(this);

    QAudioFormat  format;

    format.setSampleRate(22050); // or pVoice.synthesisConfig.sampleRate
    format.setChannelCount(1);   // or pVoice.synthesisConfig.channels
    format.setSampleFormat(QAudioFormat::Int16);


    auto  defaultDeviceInfo = m_devices->defaultAudioOutput();

    m_audioOutput = new QAudioSink(defaultDeviceInfo, format);

    int  sampleRate   = 22050;    // For example, or use pVoice.synthesisConfig.sampleRate
    int  channelCount = 1;      // For example, or use pVoice.synthesisConfig.channels
    int  sampleSize   = 16;       // bits per sample (pVoice.synthesisConfig.sampleWidth)

    m_pConf.eSpeakDataPath = "/usr/piper/espeak-ng-data/";
    m_pConf.useESpeak      = true;

    std::optional<piper::SpeakerId>  speakerId;

    on_language_currentIndexChanged(0);

    // whisper audio recorder

    requestMicrophonePermission();
    setupAudioFormat();

    // whisper
    m_whisperTranscriber = new WhisperTranscriber(this);
    m_whisperTranscriber->initialize("ggml-small-q8_0.bin", "en");
    connect(m_whisperTranscriber, &WhisperTranscriber::transcriptionCompleted, this, &MainWindow::transcriptionCompleted);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pVoice;
}

void  MainWindow::on_speakButton_clicked()
{
    piper::SynthesisResult  result = { };
    std::vector<int16_t>    audioBuffer;
    auto                    text = ui->txtToSpeach->toPlainText();

    piper::textToAudio(m_pConf, *m_pVoice, text.toStdString(), audioBuffer, result, nullptr);

    QByteArray  audioData(reinterpret_cast<const char *>(audioBuffer.data()),
                          static_cast<int>(audioBuffer.size() * sizeof(int16_t)));

    // Use a QBuffer to wrap the data for streaming playback.
    // QBuffer *audioBufferQ = new QBuffer;
    // audioBufferQ->setData(audioData);
    auto  io = m_audioOutput->start();

    io->write(audioData.data(), audioData.size());
}

void  MainWindow::on_language_currentIndexChanged(int index)
{
    std::optional<piper::SpeakerId>  speakerId;

    piper::terminate(m_pConf);
    delete m_pVoice;
    m_pVoice = new piper::Voice();

    if (index == 0)
    {
        piper::loadVoice(m_pConf, "en_US-lessac-high.onnx", "en_US-lessac-high.onnx.json", *m_pVoice, speakerId, false);
    }

    if (index == 1)
    {
        piper::loadVoice(m_pConf, "fa_IR-gyro-medium.onnx", "fa_IR-gyro-medium.onnx.json", *m_pVoice, speakerId, false);
    }

    if (index == 2)
    {
        piper::loadVoice(m_pConf, "fa_IR-amir-medium.onnx", "fa_IR-amir-medium.onnx.json", *m_pVoice, speakerId, false);
    }

    piper::initialize(m_pConf);
}

void  MainWindow::on_pbSend_clicked()
{
    auto  str = ui->lineModelText->text();
    QMetaObject::invokeMethod(m_model, "askQuestion", Qt::QueuedConnection, Q_ARG(QString, str));
    // m_model->askQuestion(ui->lineModelText->text());
}

void  MainWindow::setupAudioFormat()
{
    QAudioFormat  format;
    format.setSampleRate(16000);           // 16 kHz sample rate
    format.setChannelCount(1);            // Mono audio
    format.setSampleFormat(QAudioFormat::Int16); // 16-bit signed integer PCM

    const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

    if (!inputDevice.isFormatSupported(format))
    {
        format = inputDevice.preferredFormat();
        qDebug() << "Default format not supported; using closest match.";
    }

    // Create the QAudioSource with our chosen format
    m_audioSource = new QAudioSource(inputDevice, format, this);
}

void  MainWindow::on_recordBtn_clicked()
{
    // Start or stop recording based on state
    if (!m_isRecording)
    {
        // Start recording
        pcmf32.clear();
        m_audioInputDevice = m_audioSource->start();

        connect(m_audioInputDevice, &QIODevice::readyRead, this, &MainWindow::handleAudioData);

        statusBar()->showMessage("Recording...");
        m_isRecording = true;
    }
    else
    {
        // Stop recording
        m_audioSource->stop();
        statusBar()->showMessage("Recording Stopped. Processing audio...");

        m_isRecording = false;
        m_whisperTranscriber->transcribeAudio(pcmf32, pcmf32s);
        // Optionally: Handle the captured audio data in pcmf32 or pcmf32s
    }
}

void  MainWindow::handleAudioData()
{
    // Read audio data from the input device
    QByteArray  audioData = m_audioInputDevice->readAll();

    // Convert raw PCM data to float samples
    const qint16 *rawData     = reinterpret_cast<const qint16 *>(audioData.data());
    int           sampleCount = audioData.size() / sizeof(qint16);

    for (int i = 0; i < sampleCount; ++i)
    {
        // Convert 16-bit signed integer sample to a 32-bit float in range [-1.0, 1.0]
        float  sample = rawData[i] / 32768.0f;

        pcmf32.push_back(sample);
    }

    // If stereo audio is required (not applicable here, as channel count is 1):
    // Store this as a single-channel vector. If multi-channel, you would need
    // to split samples by channel here.
}

void  MainWindow::requestMicrophonePermission()
{
#if QT_CONFIG(permissions)
    QMicrophonePermission  microphonePermission;

    switch (qApp->checkPermission(microphonePermission))
    {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &MainWindow::requestMicrophonePermission);

        return;
    case Qt::PermissionStatus::Denied:
        statusBar()->showMessage("Microphone permission denied!");
        ui->recordBtn->setEnabled(false);

        return;
    case Qt::PermissionStatus::Granted:
        statusBar()->showMessage("Microphone permission granted!");
        break;
    }

#endif
}

void  MainWindow::on_sendSpeechBtn_clicked()
{
    m_whisperTranscriber->transcribeAudio("audio1.wav");
}

void  MainWindow::transcriptionCompleted(const QString &text)
{
    ui->speechTxtEdit->setText(text);
}
