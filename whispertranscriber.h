#ifndef WHISPERTRANSCRIBER_H
#define WHISPERTRANSCRIBER_H

#include <QObject>
#include <QString>
#include "whisper.h"  // Include the header file for whisper.cpp
#include <string>
#include <thread>

class WhisperTranscriber: public QObject
{
    Q_OBJECT

public:
    explicit WhisperTranscriber(QObject *parent = nullptr);

    ~WhisperTranscriber();

    // Initialize the Whisper model
    bool  initialize(const QString &modelPath, const QString &languag);

    // Asynchronously transcribe audio file
    // void  transcribeAudio(const QString &audioFilePath);

public slots:
    // Asynchronously transcribe audio data
    void  transcribeAudio(std::vector<float> pcmf32);

signals:
    // Signal emitted when transcription is done containing transcipted text and detected language code and detected language full name
    void  transcriptionCompleted(const QString &text, QPair<QString, QString> language);

private:
    struct whisper_context *m_context;  // Whisper context
    struct whisper_params  *m_params;
};


struct whisper_params
{
    int32_t  n_threads       = std::min(4, (int32_t)std::thread::hardware_concurrency());
    int32_t  n_processors    = 1;
    int32_t  offset_t_ms     = 0;
    int32_t  offset_n        = 0;
    int32_t  duration_ms     = 0;
    int32_t  progress_step   = 5;
    int32_t  max_context     = -1;
    int32_t  max_len         = 0;
    int32_t  best_of         = whisper_full_default_params(WHISPER_SAMPLING_GREEDY).greedy.best_of;
    int32_t  beam_size       = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH).beam_search.beam_size;
    int32_t  audio_ctx       = 0;
    float    word_thold      = 0.01f;
    float    entropy_thold   = 2.40f;
    float    logprob_thold   = -1.00f;
    float    no_speech_thold = 0.6f;
    float    grammar_penalty = 100.0f;
    float    temperature     = 0.0f;
    float    temperature_inc = 0.2f;
    bool     debug_mode      = false;
    bool     translate       = false;
    bool     detect_language = false;
    bool     diarize         = false;
    bool     tinydiarize     = false;
    bool     split_on_word   = false;
    bool     no_fallback     = false;
    bool     output_txt      = false;
    bool     output_vtt      = false;
    bool     output_srt      = false;
    bool     output_wts      = false;
    bool     output_csv      = false;
    bool     output_jsn      = false;
    bool     output_jsn_full = false;
    bool     output_lrc      = false;
    bool     no_prints       = false;
    bool     print_special   = false;
    bool     print_colors    = false;
    bool     print_progress  = false;
    bool     no_timestamps   = false;
    bool     log_score       = false;
    bool     use_gpu         = true;
    bool     flash_attn      = false;
    bool     suppress_nst    = false;

    std::string  language = "";
    std::string  prompt;
    std::string  font_path = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string  model     = "";
    std::string  grammar;
    std::string  grammar_rule;

    // [TDRZ] speaker turn string
    std::string  tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    // A regular expression that matches tokens to suppress
    std::string  suppress_regex;

    std::string  openvino_encode_device = "CPU";

    std::string  dtw = "";

    std::vector<std::string>  fname_inp = { };
    std::vector<std::string>  fname_out = { };

    // grammar_parser::parse_state  grammar_parsed;
};

#endif // WHISPERTRANSCRIBER_H
