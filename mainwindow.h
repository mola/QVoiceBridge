#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QMainWindow>
#include <QTextToSpeech>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE




class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setRate(int rate);
    void setPitch(int pitch);
    void setVolume(int volume);

private slots:

    void engineSelected(int index);
    void languageSelected(int language);
    void voiceSelected(int index);
    void stateChanged(QTextToSpeech::State state);
    void localeChanged(const QLocale &locale);
    void onEngineReady();
private:
    Ui::MainWindow *ui;
    QTextToSpeech *m_speech = nullptr;
    QList<QVoice> m_voices;
};
#endif // MAINWINDOW_H
