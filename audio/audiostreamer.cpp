#include "audiostreamer.h"
#include <QDebug>
#include <iostream>

AudioStreamer::AudioStreamer(QObject *parent):
    QObject(parent)
{
    setupAudioFormat();
    initializeFFTW();


    // Initialize the timer
    m_delayTimer = new QTimer(this);
    m_delayTimer->setInterval(1000);  // 1 second delay
    m_delayTimer->setSingleShot(true);  // Ensure the timer only fires once
    connect(m_delayTimer, &QTimer::timeout, this, &AudioStreamer::onDelayTimerTimeout);
}

AudioStreamer::~AudioStreamer()
{
    stopStreaming();
    cleanupFFTW();
}

void  AudioStreamer::startStreaming()
{
    if (!m_audioSource)
    {
        const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

        m_audioSource      = new QAudioSource(inputDevice, m_formatInput, this);
        m_audioInputDevice = m_audioSource->start();
        connect(m_audioInputDevice, &QIODevice::readyRead, this, &AudioStreamer::handleAudioData);
    }
}

void  AudioStreamer::stopStreaming()
{
    if (m_audioSource)
    {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource      = nullptr;
        m_audioInputDevice = nullptr;
    }
}

void  AudioStreamer::setupAudioFormat()
{
    // 16 kHz sample rate
    m_formatInput.setSampleRate(16000);
    // Mono audio
    m_formatInput.setChannelCount(1);
    m_formatInput.setSampleFormat(QAudioFormat::Float);

    const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

    if (!inputDevice.isFormatSupported(m_formatInput))
    {
        m_formatInput = inputDevice.preferredFormat();
        qDebug() << "Default format not supported; using closest match.";
    }
}

void  AudioStreamer::initializeFFTW()
{
    // Set the FFT size (e.g., 1024 samples)
    m_fftwSize = 1024;

    // Allocate memory for FFTW input and output arrays
    m_fftwIn  = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);
    m_fftwOut = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);

    // Create the FFTW plan
    m_fftwPlan = fftw_plan_dft_1d(m_fftwSize, m_fftwIn, m_fftwOut, FFTW_FORWARD, FFTW_MEASURE);
}

void  AudioStreamer::cleanupFFTW()
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

double  AudioStreamer::speechThreshold() const
{
    return m_speechThreshold;
}

void  AudioStreamer::setSpeechThreshold(double newSpeechThreshold)
{
    m_speechThreshold = newSpeechThreshold;
}

void  AudioStreamer::handleAudioData()
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
        sampleCount = m_fftwSize;  // Truncate if necessary
    }

    pcmf32.reserve(pcmf32.size() + sampleCount);
    pcmf32.insert(pcmf32.end(), rawData, rawData + sampleCount);

    // Fill the input array with the audio data
    for (int i = 0; i < sampleCount; ++i)
    {
        m_fftwIn[i][0] = rawData[i];  // Real part
        m_fftwIn[i][1] = 0.0;         // Imaginary part (set to 0 for real input)
    }

    // Execute the FFT
    fftw_execute(m_fftwPlan);

    // Process the FFT output (m_fftwOut array contains the frequency domain data)
    int                  s = m_fftwSize / 2.0;
    std::vector<double>  magnitudes(s);
    double               maxMagnitude           = 0.0;
    double               maxMagnitudeWithOffset = 0.0;  // Reset the offset maximum magnitude

    for (int i = 0; i < s; ++i)
    {
        magnitudes[i] = sqrt(m_fftwOut[i][0] * m_fftwOut[i][0] + m_fftwOut[i][1] * m_fftwOut[i][1]);

        if (magnitudes[i] > maxMagnitude)
        {
            maxMagnitude = magnitudes[i];
        }

        // Calculate the maximum magnitude with an offset of 10
        if ((i >= 10) && (magnitudes[i] > maxMagnitudeWithOffset))
        {
            maxMagnitudeWithOffset = magnitudes[i];
        }
    }

    // Emit the processed audio data
    emit  audioDataProcessed(magnitudes);
    // emit  audioDataLevel(maxMagnitude);
    emit  audioDataLevel(maxMagnitudeWithOffset);

    // Voice Activity Detection (VAD) logic with hysteresis
    if ((maxMagnitudeWithOffset > m_speechThreshold) && !m_isSpeaking)
    {
        // User started speaking
        m_isSpeaking = true;

        // If the delay timer is running, stop it
        if (m_delayTimer->isActive())
        {
            m_delayTimer->stop();
            m_isDelaying = false;
        }
        else
        {
            pcmf32.clear();
        }

        emit  userStartedSpeaking();
    }
    else if ((maxMagnitudeWithOffset <= m_speechThreshold) && m_isSpeaking)
    {
        // User stopped speaking
        m_isSpeaking = false;

        // Start the delay timer if it's not already running
        if (!m_isDelaying)
        {
            m_delayTimer->start();
            m_isDelaying = true;
        }
    }
}

void  AudioStreamer::onDelayTimerTimeout()
{
    // If the user is still not speaking after the delay, emit userStoppedSpeaking
    if (!m_isSpeaking)
    {
        emit  userStoppedSpeaking();
        emit  audioDataRaw(pcmf32);

        pcmf32.clear();
    }

    // Reset the delay flag
    m_isDelaying = false;
}
