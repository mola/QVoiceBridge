#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMainWindow>
#include <QFile>
#include <QMediaDevices>

#include "piper/piper.hpp"


#include <QMediaCaptureSession>
#include <QMediaRecorder>

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

    void  on_recordBtn_clicked();

    void  handleStateChanged(QMediaRecorder::RecorderState state);

    void  displayError();

private:
    Ui::MainWindow     *ui;
    piper::PiperConfig  m_pConf;
    piper::Voice       *m_pVoice      = nullptr;
    QMediaDevices      *m_devices     = nullptr;
    QAudioSink         *m_audioOutput = nullptr;

    // audio recording
    void  setupAudioFormat();

    void  requestMicrophonePermission();

    QMediaCaptureSession  m_captureSession;
    QMediaRecorder       *m_recorder  = nullptr;
    bool                  m_recording = false;
};
#endif // MAINWINDOW_H
