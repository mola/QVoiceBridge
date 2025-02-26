#include "audiostreamer.h"
#include <QDebug>
#include <iostream>

AudioStreamer::AudioStreamer(QObject *parent):
    QObject(parent)
{
    setupAudioFormat();
    initializeFFTW();
}

AudioStreamer::~AudioStreamer()
{
    stopStreaming();
    cleanupFFTW();
}

void AudioStreamer::startStreaming()
{
    if (!m_audioSource)
    {
        const QAudioDevice  inputDevice = QMediaDevices::defaultAudioInput();

        m_audioSource      = new QAudioSource(inputDevice, m_formatInput, this);
        m_audioInputDevice = m_audioSource->start();
        connect(m_audioInputDevice, &QIODevice::readyRead, this, &AudioStreamer::handleAudioData);
    }
}

void AudioStreamer::stopStreaming()
{
    if (m_audioSource)
    {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource      = nullptr;
        m_audioInputDevice = nullptr;
    }
}

void AudioStreamer::setupAudioFormat()
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

void AudioStreamer::initializeFFTW()
{
    // Set the FFT size (e.g., 1024 samples)
    m_fftwSize = 1024;

    // Allocate memory for FFTW input and output arrays
    m_fftwIn  = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);
    m_fftwOut = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * m_fftwSize);

    // Create the FFTW plan
    m_fftwPlan = fftw_plan_dft_1d(m_fftwSize, m_fftwIn, m_fftwOut, FFTW_FORWARD, FFTW_MEASURE);
}

void AudioStreamer::cleanupFFTW()
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

double AudioStreamer::speechThreshold() const
{
    return m_speechThreshold;
}

void AudioStreamer::setSpeechThreshold(double newSpeechThreshold)
{
    m_speechThreshold = newSpeechThreshold;
}

void AudioStreamer::handleAudioData()
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
        sampleCount = m_fftwSize;                         // Truncate if necessary
    }

    // Fill the input array with the audio data
    for (int i = 0; i < sampleCount; ++i)
    {
        m_fftwIn[i][0] = rawData[i];                         // Real part
        m_fftwIn[i][1] = 0.0;                                // Imaginary part (set to 0 for real input)
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

    // Emit the processed audio data
    emit  audioDataProcessed(magnitudes);

    // Voice Activity Detection (VAD) logic with hysteresis
    if ((maxMagnitude > m_speechThreshold) && !m_isSpeaking)
    {
        // User started speaking
        m_isSpeaking = true;

        emit  userStartedSpeaking();
    }
    else if ((maxMagnitude <= m_speechThreshold) && m_isSpeaking)
    {
        // User stopped speaking
        m_isSpeaking = false;

        emit  userStoppedSpeaking();
    }
}
