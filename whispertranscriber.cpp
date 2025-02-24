#include "whispertranscriber.h"
#include "common.h"

#include <QFile>
#include <QDebug>

WhisperTranscriber::WhisperTranscriber(QObject *parent):
    QObject(parent), m_context(nullptr)
{
}

WhisperTranscriber::~WhisperTranscriber()
{
    if (m_context)
    {
        whisper_free(m_context);  // Free Whisper context when cleaning up
        m_context = nullptr;
    }
}

bool  WhisperTranscriber::initialize(const QString &modelPath, const QString &languag)
{
    // Ensure the model file exists
    QFile  modelFile(modelPath);

    if (!modelFile.exists())
    {
        qWarning() << "Model file not found:" << modelPath;

        return false;
    }

    m_params = new whisper_params();
    // Adjust processing options:
    m_params->n_threads    = std::min(4, (int32_t)std::thread::hardware_concurrency());
    m_params->n_processors = 1;
    m_params->offset_t_ms  = 0;
    m_params->duration_ms  = 0;      // process entire audio
    m_params->language     = languag.toStdString();   // spoken language: set to "auto" to auto-detect
    m_params->translate    = false;  // set to true if you want to translate from source language
    m_params->debug_mode   = false;  // set to true to print debugging information
    m_params->model        = modelPath.toStdString();

    // ─────────────────────────────────────────────────────────────
    // Initialize the whisper context with (optional) GPU support.
    struct whisper_context_params  cparams = whisper_context_default_params();

    cparams.use_gpu    = m_params->use_gpu;
    cparams.flash_attn = m_params->flash_attn;

    // Initialize from the model file
    m_context = whisper_init_from_file_with_params(m_params->model.c_str(), cparams);

    if (!m_context)
    {
        qWarning() << "Failed to initialize Whisper context.";

        return false;
    }

    return true;
}

void  WhisperTranscriber::transcribeAudio(const QString &audioFilePath)
{
    std::string  wavfile = audioFilePath.toStdString();

    if (!is_file_exist(wavfile.c_str()))
    {
        qWarning() << "error: input file not found '%s'\n" << audioFilePath;

        return;
    }

    std::vector<float>               pcmf32;               // mono-channel WAV samples (32-bit float)
    std::vector<std::vector<float>>  pcmf32s;   // stereo channels if diarization is enabled

    if (!read_wav(wavfile, pcmf32, pcmf32s, m_params->diarize))
    {
        fprintf(stderr, "error: failed to read WAV file '%s'\n", wavfile.c_str());

        return;
    }

    // ─────────────────────────────────────────────────────────────
    // (Optional) Print some basic info about the file
    fprintf(stderr, "\nProcessing '%s' (%d samples, %.1f sec) ...\n",
            wavfile.c_str(), int(pcmf32.size()), float(pcmf32.size()) / WHISPER_SAMPLE_RATE);

    // ─────────────────────────────────────────────────────────────
    // Set up whisper processing parameters
    whisper_full_params  wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.n_threads        = m_params->n_threads;
    wparams.translate        = m_params->translate;
    wparams.language         = m_params->language.c_str();
    wparams.offset_ms        = m_params->offset_t_ms;
    wparams.duration_ms      = m_params->duration_ms;
    wparams.print_timestamps = true;  // change to false if you don’t want time info
    // You can customize additional parameters (temperature, beam size, etc.) if needed

    // ─────────────────────────────────────────────────────────────
    // Run the inference using the full (parallel) runner
    if (whisper_full_parallel(m_context, wparams, pcmf32.data(), pcmf32.size(), m_params->n_processors) != 0)
    {
        fprintf(stderr, "error: failed to process audio\n");

        return;
    }

    QString    result;
    const int  n_segments = whisper_full_n_segments(m_context);

    for (int i = 0; i < n_segments; i++)
    {
        // Optionally, you can also get the timestamps by:
        // int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        // int64_t t1 = whisper_full_get_segment_t1(ctx, i);
        // printf("[%s --> %s] ", to_timestamp(t0).c_str(), to_timestamp(t1).c_str());
        result += whisper_full_get_segment_text(m_context, i);
    }

    // Emit signal when transcription is done
    emit  transcriptionCompleted(result);

    return;
}
