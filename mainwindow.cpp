#include "mainwindow.h"
#include "./ui_mainwindow.h"


#include <vector>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QByteArray>
#include <QSettings>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
#include <QAudioSink>

#include "piper/piper.hpp"

#include <iostream>

// audio recorder:
#include <QAudioInput>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QStandardPaths>
#include <QStatusBar>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>

#if QT_CONFIG(permissions)
#include <QPermission>
#endif

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSettings  settings;
    QString    modelPath = settings.value("model_path", "/home/mola/Data/Model/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf").toString();

    m_model = new LlamaInterface();

    if (QFile::exists(modelPath))
    {
        m_modelLoaded = m_model->loadModel(modelPath);
        ui->pbSend->setEnabled(m_modelLoaded);
    }
    else
    {
        QMessageBox::warning(this, tr("Model Path"), tr("Please set model path in settings"), QMessageBox::Ok);
    }

    m_thread = new QThread();
    m_model->moveToThread(m_thread);
    m_thread->start();


    connect(m_model, &LlamaInterface::answerReady, this, [this](QString c)
    {
        ui->txtToSpeach->insertPlainText(c);
    });

    connect(m_model, &LlamaInterface::generateFinished, this, [this](std::string msg)
    {
        ui->txtToSpeach->insertPlainText("\n");
        playText(msg);
    }, Qt::QueuedConnection);

    m_devices = new QMediaDevices(this);

    QAudioFormat  format;

    format.setSampleRate(22050);                         // or pVoice.synthesisConfig.sampleRate
    format.setChannelCount(1);                           // or pVoice.synthesisConfig.channels
    format.setSampleFormat(QAudioFormat::Int16);


    auto  defaultDeviceInfo = m_devices->defaultAudioOutput();

    m_audioOutput = new QAudioSink(defaultDeviceInfo, format);

    int  sampleRate   = 22050;                            // For example, or use pVoice.synthesisConfig.sampleRate
    int  channelCount = 1;                              // For example, or use pVoice.synthesisConfig.channels
    int  sampleSize   = 16;                               // bits per sample (pVoice.synthesisConfig.sampleWidth)

    m_pConf.eSpeakDataPath = "espeak-ng-data";
    m_pConf.useESpeak      = true;

    std::optional<piper::SpeakerId>  speakerId;

    on_language_currentIndexChanged(0);

    // whisper audio recorder

    requestMicrophonePermission();

    // whisper
    m_whisperTranscriber = new WhisperTranscriber();
    m_whisperTranscriber->initialize("ggml-small-q8_0.bin", "auto");
    connect(m_whisperTranscriber, &WhisperTranscriber::transcriptionCompleted, this, &MainWindow::transcriptionCompleted);

    m_whisperThread = new QThread();
    m_whisperTranscriber->moveToThread(m_whisperThread);
    m_whisperThread->start();


    // Initialize audio streamer and move it to a separate thread
    m_audioStreamer = new AudioStreamer();
    m_audioThread   = new QThread();
    m_audioStreamer->moveToThread(m_audioThread);
    m_audioThread->start();

    connect(m_audioStreamer, &AudioStreamer::audioDataProcessed, this, &MainWindow::handleAudioDataProcessed);
    connect(m_audioStreamer, &AudioStreamer::userStartedSpeaking, this, []()
    {
        std::cout << "User started speaking!" << std::endl;
    });
    connect(m_audioStreamer, &AudioStreamer::userStoppedSpeaking, this, []()
    {
        std::cout << "User stopped speaking!" << std::endl;
    });
    connect(m_audioStreamer, &AudioStreamer::audioDataLevel, this, [this](double lvl)
    {
        ui->lblLEvel->setText(QString::number(lvl, 'f', 6));
    });

    connect(m_audioStreamer, &AudioStreamer::audioDataRaw, m_whisperTranscriber, &WhisperTranscriber::transcribeAudio, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    if (m_whisperThread)
    {
        m_whisperThread->quit();
        m_whisperThread->wait();
        delete m_whisperThread;
    }

    if (m_whisperTranscriber)
    {
        delete m_whisperTranscriber;
    }

    if (m_audioThread)
    {
        m_audioThread->quit();
        m_audioThread->wait();
        delete m_audioThread;
    }

    delete m_audioStreamer;

    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }

    delete m_model;

    delete ui;
    delete m_pVoice;
}

void  MainWindow::on_speakButton_clicked()
{
    auto  text = ui->txtToSpeach->toPlainText();

    playText(text.toStdString());
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
    QMetaObject::invokeMethod(m_model, "generate", Qt::QueuedConnection, Q_ARG(QString, str));

    // m_model->askQuestion(ui->lineModelText->text());
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
        ui->pbRecord->setEnabled(false);

        return;
    case Qt::PermissionStatus::Granted:
        statusBar()->showMessage("Microphone permission granted!");
        break;
    }

#endif
}

void  MainWindow::playText(std::string msg)
{
    std::vector<int16_t>    audioBuffer;
    piper::SynthesisResult  result = { };

    piper::textToAudio(m_pConf, *m_pVoice, msg, audioBuffer, result, nullptr);

    QByteArray  audioData(reinterpret_cast<const char *>(audioBuffer.data()),
                          static_cast<int>(audioBuffer.size() * sizeof(int16_t)));

    // Use a QBuffer to wrap the data for streaming playback.
    // QBuffer *audioBufferQ = new QBuffer;
    // audioBufferQ->setData(audioData);
    auto  io = m_audioOutput->start();

    io->write(audioData.data(), audioData.size());
}

void  MainWindow::on_sendSpeechBtn_clicked()
{
}

void  MainWindow::transcriptionCompleted(const QString &text, QPair<QString, QString> language)
{
    ui->speechTxtEdit->clear();
    ui->speechTxtEdit->setText(text);
    statusBar()->showMessage("language: " + language.first + " : " + language.second);
    ui->leLanguage->setText(language.second);

    if (text.isEmpty())
    {
        return;
    }

    if (!ui->language->currentText().startsWith(language.first, Qt::CaseInsensitive))
    {
    }

    if (m_modelLoaded)
    {
        if (text.contains("[") && text.contains("]"))
        {
            return;
        }

        QMetaObject::invokeMethod(m_model, "generate", Qt::QueuedConnection, Q_ARG(QString, text));
    }
}

void  MainWindow::on_pbRecord_toggled(bool checked)
{
    if (checked)
    {
        m_audioStreamer->startStreaming();
        statusBar()->showMessage("Recording...");
        ui->pbRecord->setText("Stop Recording");
    }
    else
    {
        m_audioStreamer->stopStreaming();
        statusBar()->showMessage("Recording Stopped.");
        ui->pbRecord->setText("Start Recording");
    }
}

void  MainWindow::handleAudioDataProcessed(const std::vector<double> &magnitudes)
{
    ui->chartView->frequenciesChanged(magnitudes.data(), magnitudes.size());
}

void  MainWindow::on_spinThreshold_valueChanged(double arg1)
{
    m_audioStreamer->setSpeechThreshold(arg1);
    ui->chartView->setThreshold(arg1);
}
