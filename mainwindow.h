#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMainWindow>
#include <QMediaDevices>

#include "piper/piper.hpp"
#include "model/llamamodel.h"


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

private slots:

   void on_speakButton_clicked();

    void on_language_currentIndexChanged(int index);

   void on_pbSend_clicked();

   private:
    Ui::MainWindow *ui;

    piper::PiperConfig m_pConf;
    piper::Voice *m_pVoice = nullptr;

    QMediaDevices *m_devices = nullptr;
    QAudioSink *m_audioOutput = nullptr;

    LlamaInterface *m_model = nullptr;
};
#endif // MAINWINDOW_H
