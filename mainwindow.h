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
#include "audio/audiostreamer.h"
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

private slots:
    void  on_speakButton_clicked();

    void  on_language_currentIndexChanged(int index);

    void  on_pbSend_clicked();

    void  on_sendSpeechBtn_clicked();

    void  transcriptionCompleted(const QString &text, QPair<QString, QString> language);

    void  on_pbRecord_toggled(bool checked);

    void  handleAudioDataProcessed(const std::vector<double> &magnitudes);

    void  on_spinThreshold_valueChanged(double arg1);

private:
    void  requestMicrophonePermission();

    void  playText(std::string msg);

private:
    Ui::MainWindow     *ui;
    piper::PiperConfig  m_pConf;
    piper::Voice       *m_pVoice      = nullptr;
    QMediaDevices      *m_devices     = nullptr;
    QAudioSink         *m_audioOutput = nullptr;
    LlamaInterface     *m_model       = nullptr;
    bool                m_modelLoaded = false;
    QThread            *m_thread      = nullptr;
    WhisperTranscriber *m_whisperTranscriber;
    AudioStreamer      *m_audioStreamer = nullptr;
    QThread            *m_audioThread   = nullptr;
};
#endif // MAINWINDOW_H
