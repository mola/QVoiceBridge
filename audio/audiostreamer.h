#ifndef AUDIOSTREAMER_H
#define AUDIOSTREAMER_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QThread>
#include <fftw3.h>
#include <QTimer>

class AudioStreamer: public QObject
{
    Q_OBJECT

public:
    explicit AudioStreamer(QObject *parent = nullptr);

    ~AudioStreamer();

    void    startStreaming();

    void    stopStreaming();

    double  speechThreshold() const;

    void    setSpeechThreshold(double newSpeechThreshold);

signals:
    void    userStartedSpeaking();

    void    userStoppedSpeaking();

    void    audioDataProcessed(const std::vector<double> &magnitudes);

    void    audioDataRaw(std::vector<float>);

    void    audioDataLevel(double);

private slots:
    void    handleAudioData();

    // Slot for handling the timer timeout
    void    onDelayTimerTimeout();

private:
    void    setupAudioFormat();

    void    initializeFFTW();

    void    cleanupFFTW();

private:
    QAudioSource *m_audioSource      = nullptr;
    QIODevice    *m_audioInputDevice = nullptr;
    QAudioFormat  m_formatInput;

    // FFTW3 resources
    fftw_complex *m_fftwIn  = nullptr;               // Input array
    fftw_complex *m_fftwOut = nullptr;               // Output array
    fftw_plan     m_fftwPlan;                        // FFTW plan
    int           m_fftwSize = 0;                    // Size of the FFT (number of samples)

    std::vector<float>               pcmf32; // Mono-channel float
    std::vector<std::vector<float>>  pcmf32s; // Stereo-channel floats

    // Threshold for detecting speech
    double  m_speechThreshold = 10.0;                // Adjust this value based on your needs
    bool    m_isSpeaking      = false;               // Track whether the user is currently speaking
    int     m_ignore          = 0;


    // Timer for one-second delay
    QTimer *m_delayTimer = nullptr;
    // Flag to track if we're in the delay period
    bool  m_isDelaying = false;
};

#endif // AUDIOSTREAMER_H
