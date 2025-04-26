#ifndef WHISPERMODEL_H
#define WHISPERMODEL_H

#include <QObject>
#include <whisper.h>

class WhisperModel: public QObject
{
    Q_OBJECT

public:
    // Constructor and destructor
    explicit WhisperModel(QObject *parent = nullptr);

    ~WhisperModel();

    // Expose the method to QML (or other QObject-based code)
    Q_INVOKABLE QString  askQuestion(const QString &question);

private:
    // Pointer to the whisper model/context structure.
    // This type depends on the whisper.cpp API. Using "whisper_context" as example.
    whisper_context *ctx;
};

#endif // WHISPERMODEL_H
