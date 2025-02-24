#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QMainWindow>
#include <QTextToSpeech>
#include <QFile>

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

public slots:
    void  setRate(int rate);

    void  setPitch(int pitch);

    void  setVolume(int volume);

private slots:
    void  engineSelected(int index);

    void  languageSelected(int language);

    void  voiceSelected(int index);

    void  stateChanged(QTextToSpeech::State state);

    void  localeChanged(const QLocale &locale);

    void  onEngineReady();

    void  on_recordBtn_clicked();

    void  handleStateChanged(QMediaRecorder::RecorderState state);

    void  displayError();

private:
    Ui::MainWindow *ui;
    QTextToSpeech  *m_speech = nullptr;
    QList<QVoice>   m_voices;

    // audio recording
    void  setupAudioFormat();

    void  requestMicrophonePermission();

    QMediaCaptureSession  m_captureSession;
    QMediaRecorder       *m_recorder  = nullptr;
    bool                  m_recording = false;
};
#endif // MAINWINDOW_H
