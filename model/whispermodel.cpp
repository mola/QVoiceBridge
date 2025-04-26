#include "whispermodel.h"

#include <QDebug>

// Constructor: Initialize the model loader, then load the model using whisper_init_with_params.
WhisperModel::WhisperModel(QObject *parent):
    QObject(parent), ctx(nullptr)
{
    // Specify the model file path.
    const char *modelPath = "/home/mola/Downloads/tts/ggml-tiny-fp16.bin";

    // You can set additional parameters in loader here if needed. For example:
    // loader.some_parameter = some_value;

    // Initialize the whisper context using the new API.
    ctx = whisper_init_from_file_with_params(modelPath, whisper_context_default_params());

    if (!ctx)
    {
        qWarning() << "Failed to load model from:" << modelPath;
    }
    else
    {
        qDebug() << "Model loaded successfully from:" << modelPath;
    }
}

// Destructor: Free the whisper context.
WhisperModel::~WhisperModel()
{
    if (ctx)
    {
        // Free resources associated with the model context.
        whisper_free(ctx);
        ctx = nullptr;
    }
}

// askQuestion: Process the question using the whisper inference API.
// Replace the pseudocode with the actual inference calls.
QString  WhisperModel::askQuestion(const QString &question)
{
    if (!ctx)
    {
        return QString("Error: Model not loaded.");
    }

    // Convert the question to a UTF-8 encoded QByteArray.
    QByteArray  promptData = question.toUtf8();

    // Prepare the inference parameters.
    //
    // Assume whisper_full_params is the structure used for inference and that
    // whisper_full_default_params() is available to initialize it.
    //
    // Also assume that an enum value WHISPER_SAMPLING_GREEDY is available.
    whisper_full_params  params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    // IMPORTANT:
    // The API expects the pointer to your prompt text.
    // Ensure that the API either copies the content or that the lifetime of the
    // prompt is sufficient for the inference call.
    params.initial_prompt = promptData.constData();

    // Optional: you might want to adjust other parameters in 'params' such as
    // number of threads, maximum tokens, etc.
    // For example:
    // params.n_threads = 4;

    // Run full inference.
    int  ret = whisper_full(ctx, &params, samples.data(), n_samples);

    if (ret != 0)
    {
        return QString("Error during model inference (error code %1)").arg(ret);
    }

    // Retrieve the output text.
    QString  output;
    int      n_segments = whisper_full_n_segments(ctx);

    for (int i = 0; i < n_segments; ++i)
    {
        const char *segment_text = whisper_full_get_segment_text(ctx, i);

        if (segment_text != nullptr)
        {
            output.append(QString::fromUtf8(segment_text));
            output.append(" ");
        }
    }

    return output.trimmed();
}
