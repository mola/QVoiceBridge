#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioSource>
#include <QIODevice>
#include <QThread>

#include "piper/piper.hpp"
#include "model/llamamodel.h"

#include "whispertranscriber.h"

#include <fftw3.h> // Include FFTW3 header

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

signals:
    // Signal emitted when the user starts speaking
    void   userStartedSpeaking();

    // Signal emitted when the user stops speaking
    void   userStoppedSpeaking();

private slots:
    void   on_speakButton_clicked();

    void   on_language_currentIndexChanged(int index);

    void   on_pbSend_clicked();

    void   on_sendSpeechBtn_clicked();

    void   transcriptionCompleted(const QString &text, QPair<QString, QString> language);

    void   on_pbRecord_toggled(bool checked);

private:
    void   setupAudioFormat();

    void   handleAudioData();

    void   requestMicrophonePermission();

    void   playText(std::string msg);

    qreal  calculateLevel(const std::vector<float> &data) const;

    // FFTW3 resources
    void   initializeFFTW();

    void   cleanupFFTW();

private:
    Ui::MainWindow     *ui;
    piper::PiperConfig  m_pConf;
    piper::Voice       *m_pVoice      = nullptr;
    QMediaDevices      *m_devices     = nullptr;
    QAudioSink         *m_audioOutput = nullptr;
    LlamaInterface     *m_model       = nullptr;
    bool                m_modelLoaded = false;
    QThread            *m_thread      = nullptr;

    // whisper
    QAudioSource *m_audioSource      = nullptr;
    QIODevice    *m_audioInputDevice = nullptr;

    std::vector<float>               pcmf32; // Mono-channel float
    std::vector<std::vector<float>>  pcmf32s; // Stereo-channel floats
    WhisperTranscriber              *m_whisperTranscriber;
    QAudioFormat                     m_formatInput;
    bool                             m_isAboveThreshold = false;

    // FFTW3 resources
    fftw_complex *m_fftwIn  = nullptr;  // Input array
    fftw_complex *m_fftwOut = nullptr;  // Output array
    fftw_plan     m_fftwPlan;           // FFTW plan
    int           m_fftwSize = 0;       // Size of the FFT (number of samples)

    // Threshold for detecting speech
    double  m_speechThreshold = 0.03;  // Adjust this value based on your needs
    bool    m_isSpeaking      = false;      // Track whether the user is currently speaking
    int     m_ignore          = 0;
};
#endif // MAINWINDOW_H
