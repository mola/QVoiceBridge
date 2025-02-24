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

    m_model = new LlamaInterface(this);
    m_model->loadModel("/extra/jan/models/llama3.1-8b-instruct/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf");

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

	// audio recorder
    m_recorder = new QMediaRecorder(this);
    m_captureSession.setRecorder(m_recorder);
    m_captureSession.setAudioInput(new QAudioInput(this));

    connect(m_recorder, &QMediaRecorder::recorderStateChanged, this, &MainWindow::handleStateChanged);
    connect(m_recorder, &QMediaRecorder::errorChanged, this, &MainWindow::displayError);

    requestMicrophonePermission();
    setupAudioFormat();
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
    QString  res = m_model->askQuestion(ui->lineModelText->text());
    ui->txtToSpeach->setPlainText(res);
}

void  MainWindow::on_recordBtn_clicked()
{
    if (!m_recording)
    {
        // Set output file
        QString  fileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".wav";

        m_recorder->setOutputLocation(QUrl::fromLocalFile(fileName));

        // Start recording
        m_recorder->record();
    }
    else
    {
        m_recorder->stop();
    }
}

void  MainWindow::setupAudioFormat()
{
    // Set audio input format
    QAudioFormat  format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    // Validate format support
    const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

    if (!inputDevice.isFormatSupported(format))
    {
        format = inputDevice.preferredFormat();
        std::cout << "Default format not supported, using closest match:"
                  << format.sampleRate() << "Hz"
                  << format.channelCount() << "channels" << std::endl;
    }

    // Update capture session
    delete m_captureSession.audioInput();
    m_captureSession.setAudioInput(new QAudioInput(this));

    // Set media format for recorder
    QMediaFormat  mediaFormat;

    mediaFormat.setFileFormat(QMediaFormat::Wave);
    mediaFormat.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    m_recorder->setMediaFormat(mediaFormat);
}

void  MainWindow::handleStateChanged(QMediaRecorder::RecorderState state)
{
    m_recording = (state == QMediaRecorder::RecordingState);
    ui->recordBtn->setText(m_recording ? "Stop Recording" : "Start Recording");

    if (state == QMediaRecorder::RecordingState)
    {
        statusBar()->showMessage("Recording");
        m_recorder->setAudioSampleRate(16000);  // 16 kHz
    }

    if (state == QMediaRecorder::StoppedState)
    {
        statusBar()->showMessage("Recording saved to: " + m_recorder->actualLocation().toLocalFile());
    }
}

void  MainWindow::displayError()
{
    statusBar()->showMessage("Error: " + m_recorder->errorString());
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
