#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMainWindow>
#include <QFile>
#include <QMediaDevices>

#include "piper/piper.hpp"
#include "model/llamamodel.h"

#include "whispertranscriber.h"
// #include <QMediaCaptureSession>
// #include <QMediaRecorder>
#include <QAudioSource>
#include <QIODevice>

#include <QThread>
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

private slots:
    void  on_speakButton_clicked();

    void  on_language_currentIndexChanged(int index);


    // void  handleStateChanged(QMediaRecorder::RecorderState state);

    // void  displayError();

    void  on_pbSend_clicked();

    void  on_sendSpeechBtn_clicked();

    void  transcriptionCompleted(const QString &text); // emitted when whisper speech to text transcription is completed

    void  on_recordBtn_toggled(bool checked);

private:
    // audio recording
    void  setupAudioFormat();

    void  handleAudioData();

    void  requestMicrophonePermission();

    void  playText(std::string msg);

private:
    Ui::MainWindow     *ui;
    piper::PiperConfig  m_pConf;
    piper::Voice       *m_pVoice      = nullptr;
    QMediaDevices      *m_devices     = nullptr;
    QAudioSink         *m_audioOutput = nullptr;
    LlamaInterface     *m_model       = nullptr;
    QThread            *m_thread      = nullptr;

    // whisper
    QAudioSource *m_audioSource      = nullptr;
    QIODevice    *m_audioInputDevice = nullptr;

    std::vector<float>               pcmf32; // Mono-channel float
    std::vector<std::vector<float>>  pcmf32s; // Stereo-channel floats
    WhisperTranscriber              *m_whisperTranscriber;
};
#endif // MAINWINDOW_H
