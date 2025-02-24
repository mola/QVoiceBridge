#include "mainwindow.h"
#include "./ui_mainwindow.h"


#include <vector>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QByteArray>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
#include <QAudioSink>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_devices = new QMediaDevices(this);

    QAudioFormat format;
    format.setSampleRate(22050); // or pVoice.synthesisConfig.sampleRate
    format.setChannelCount(1);   // or pVoice.synthesisConfig.channels
    format.setSampleFormat(QAudioFormat::Int16);


    auto defaultDeviceInfo = m_devices->defaultAudioOutput();

    m_audioOutput = new QAudioSink(defaultDeviceInfo, format) ;

    int sampleRate = 22050;    // For example, or use pVoice.synthesisConfig.sampleRate
    int channelCount = 1;      // For example, or use pVoice.synthesisConfig.channels
    int sampleSize = 16;       // bits per sample (pVoice.synthesisConfig.sampleWidth)

    m_pConf.eSpeakDataPath = "/usr/piper/espeak-ng-data/";
    m_pConf.useESpeak = true;

    std::optional<piper::SpeakerId> speakerId;

    on_language_currentIndexChanged(0);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pVoice;

}

void MainWindow::on_speakButton_clicked()
{

    piper::SynthesisResult result = {};
    std::vector<int16_t> audioBuffer;

    auto text = ui->txtToSpeach->toPlainText();


    piper::textToAudio(m_pConf, *m_pVoice, text.toStdString(), audioBuffer,result, nullptr);



    QByteArray audioData(reinterpret_cast<const char*>(audioBuffer.data()),
                         static_cast<int>(audioBuffer.size() * sizeof(int16_t)));

    // Use a QBuffer to wrap the data for streaming playback.
    // QBuffer *audioBufferQ = new QBuffer;
    // audioBufferQ->setData(audioData);


    auto io = m_audioOutput->start();
    io->write(audioData.data(), audioData.size());

}


void MainWindow::on_language_currentIndexChanged(int index)
{
    std::optional<piper::SpeakerId> speakerId;

    piper::terminate(m_pConf);
    delete m_pVoice;
    m_pVoice = new  piper::Voice();
    if (index == 0)
    {
        piper::loadVoice(m_pConf, "en_US-lessac-high.onnx", "en_US-lessac-high.onnx.json", *m_pVoice,speakerId, false);
    }
    if (index == 1)
        piper::loadVoice(m_pConf, "fa_IR-gyro-medium.onnx", "fa_IR-gyro-medium.onnx.json", *m_pVoice,speakerId, false);
        if (index == 2)
    {
        piper::loadVoice(m_pConf, "fa_IR-amir-medium.onnx", "fa_IR-amir-medium.onnx.json", *m_pVoice,speakerId, false);
    }

    piper::initialize(m_pConf);

}

