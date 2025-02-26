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

    format.setSampleRate(22050);   // or pVoice.synthesisConfig.sampleRate
    format.setChannelCount(1);     // or pVoice.synthesisConfig.channels
    format.setSampleFormat(QAudioFormat::Int16);


    auto  defaultDeviceInfo = m_devices->defaultAudioOutput();

    m_audioOutput = new QAudioSink(defaultDeviceInfo, format);

    int  sampleRate   = 22050;      // For example, or use pVoice.synthesisConfig.sampleRate
    int  channelCount = 1;        // For example, or use pVoice.synthesisConfig.channels
    int  sampleSize   = 16;         // bits per sample (pVoice.synthesisConfig.sampleWidth)

    m_pConf.eSpeakDataPath = "espeak-ng-data";
    m_pConf.useESpeak      = true;

    std::optional<piper::SpeakerId>  speakerId;

    on_language_currentIndexChanged(0);

    // whisper audio recorder

    requestMicrophonePermission();
    setupAudioFormat();

    // whisper
    m_whisperTranscriber = new WhisperTranscriber(this);
    m_whisperTranscriber->initialize("ggml-small-q8_0.bin", "auto");
    connect(m_whisperTranscriber, &WhisperTranscriber::transcriptionCompleted, this, &MainWindow::transcriptionCompleted);

    // Initialize FFTW resources
    initializeFFTW();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pVoice;

    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }

    // Clean up FFTW resources
    cleanupFFTW();
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

    // Connect signals to slots (optional)
    connect(this, &MainWindow::userStartedSpeaking, this, []()
    {
        std::cout << "User started speaking!" << std::endl;
    });

    connect(this, &MainWindow::userStoppedSpeaking, this, []()
    {
        std::cout << "User stopped speaking!" << std::endl;
    });
}

void  MainWindow::on_pbSend_clicked()
{
    auto  str = ui->lineModelText->text();
    QMetaObject::invokeMethod(m_model, "generate", Qt::QueuedConnection, Q_ARG(QString, str));
    // m_model->askQuestion(ui->lineModelText->text());
}

void  MainWindow::setupAudioFormat()
{
    m_formatInput.setSampleRate(16000);           // 16 kHz sample rate
    m_formatInput.setChannelCount(1);            // Mono audio
    m_formatInput.setSampleFormat(QAudioFormat::Float); // 16-bit signed integer PCM

    const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

    if (!inputDevice.isFormatSupported(m_formatInput))
    {
        m_formatInput = inputDevice.preferredFormat();
        qDebug() << "Default format not supported; using closest match.";
    }

    // Create the QAudioSource with our chosen format
    m_audioSource = new QAudioSource(inputDevice, m_formatInput, this);
}

qreal  MainWindow::calculateLevel(const std::vector<float> &data) const
{
    if (data.empty())
    {
        return 0.0; // Return 0 if the input data is empty
    }

    qreal  sum = 0.0;

    // Calculate the sum of squares of the samples
    for (const float sample : data)
    {
        sum += sample * sample; // Sum of squares
    }

    // Calculate the RMS value
    qreal  rms = sqrt(sum / data.size());

    return rms;
}

void  MainWindow::initializeFFTW()
{
    // Set the FFT size (e.g., 1024 samples)
    m_fftwSize = 1024;

    // Allocate memory for FFTW input and output arrays
    m_fftwIn  = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);
    m_fftwOut = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);

    // Create the FFTW plan
    m_fftwPlan = fftw_plan_dft_1d(m_fftwSize, m_fftwIn, m_fftwOut, FFTW_FORWARD, FFTW_MEASURE);
}

void  MainWindow::cleanupFFTW()
{
    // Destroy the FFTW plan and free memory
    if (m_fftwPlan)
    {
        fftw_destroy_plan(m_fftwPlan);
    }

    if (m_fftwIn)
    {
        fftw_free(m_fftwIn);
    }

    if (m_fftwOut)
    {
        fftw_free(m_fftwOut);
    }
}

void  MainWindow::handleAudioData()
{
    // Read audio data from the input device
    QByteArray  audioData = m_audioInputDevice->readAll();

    if (audioData.isEmpty())
    {
        return;
    }

    if (m_ignore < 10)
    {
        m_ignore++;

        return;
    }

    // Convert raw PCM data to float samples
    const float *rawData     = reinterpret_cast<const float *>(audioData.data());
    int          sampleCount = audioData.size() / sizeof(float);

    // Ensure the sample count matches the FFT size
    if (sampleCount > m_fftwSize)
    {
        sampleCount = m_fftwSize; // Truncate if necessary
    }

    // Fill the input array with the audio data
    for (int i = 0; i < sampleCount; ++i)
    {
        m_fftwIn[i][0] = rawData[i]; // Real part
        m_fftwIn[i][1] = 0.0;        // Imaginary part (set to 0 for real input)
    }

    // Execute the FFT
    fftw_execute(m_fftwPlan);

    // Process the FFT output (m_fftwOut array contains the frequency domain data)
    std::vector<double>  magnitudes(m_fftwSize);
    double               maxMagnitude = 0.0;

    for (int i = 0; i < m_fftwSize; ++i)
    {
        magnitudes[i] = sqrt(m_fftwOut[i][0] * m_fftwOut[i][0] + m_fftwOut[i][1] * m_fftwOut[i][1]);

        if (magnitudes[i] > maxMagnitude)
        {
            maxMagnitude = magnitudes[i];
        }
    }

    // Optionally, you can pass the magnitudes to your chart view or other processing
    ui->chartView->frequenciesChanged(magnitudes.data(), m_fftwSize);

    // Voice Activity Detection (VAD) logic with hysteresis
    if ((maxMagnitude > m_speechThreshold) && !m_isSpeaking)
    {
        // User started speaking
        m_isSpeaking = true;

        emit  userStartedSpeaking();  // Emit signal
    }
    else if ((maxMagnitude <= m_speechThreshold) && m_isSpeaking)
    {
        // User stopped speaking
        m_isSpeaking = false;

        emit  userStoppedSpeaking();  // Emit signal
    }

    // If stereo audio is required (not applicable here, as channel count is 1):
    // Store this as a single-channel vector. If multi-channel, you would need
    // to split samples by channel here.

    // Voice Activity Detection (VAD) logic with hysteresis
    // static const qreal  silenceThreshold  = 0.0015; // Lower threshold for silence
    // static const qreal  activityThreshold = 0.001; // Higher threshold for activity

    // if ((lvl >= activityThreshold) && !m_isAboveThreshold)
    // {
    //// Level crossed above the activity threshold
    // m_isAboveThreshold = true;
    // }
    // else if ((lvl < silenceThreshold) && m_isAboveThreshold)
    // {
    //// Level crossed below the silence threshold
    // m_isAboveThreshold = false;

    //// Trigger transcription
    // m_whisperTranscriber->transcribeAudio(pcmf32, pcmf32s);
    // pcmf32.clear();
    // pcmf32s.clear();
    // }

    // if (m_isAboveThreshold)
    // {
    //// Append the new audio data directly to pcmf32
    // pcmf32.insert(pcmf32.end(), rawData, rawData + sampleCount);
    // }
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
    m_whisperTranscriber->transcribeAudio("audio1.wav");
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

        // QMetaObject::invokeMethod(m_model, "generate", Qt::QueuedConnection, Q_ARG(QString, text));
    }
}

void  MainWindow::on_pbRecord_toggled(bool checked)
{
    // Start or stop recording based on state
    if (checked)
    {
        // Start recording
        pcmf32.clear();
        pcmf32s.clear();
        m_audioInputDevice = m_audioSource->start();

        connect(m_audioInputDevice, &QIODevice::readyRead, this, &MainWindow::handleAudioData);

        statusBar()->showMessage("Recording...");
        ui->pbRecord->setText("Stop Recording");
    }

    // else
    // {
    //// Stop recording
    // m_audioSource->stop();
    // statusBar()->showMessage("Recording Stopped. Processing audio...");

    // m_whisperTranscriber->transcribeAudio(pcmf32, pcmf32s);
    // pcmf32.clear();
    // pcmf32s.clear();
    // ui->pbRecord->setText("Start Recording");
    //// Optionally: Handle the captured audio data in pcmf32 or pcmf32s
    // }
}
