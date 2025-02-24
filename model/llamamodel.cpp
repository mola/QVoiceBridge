#include "llamamodel.h"

#include "llama.h"
#include <QDebug>

LlamaInterface::LlamaInterface(QObject *parent):
    QObject(parent), m_context(nullptr)
{
}

LlamaInterface::~LlamaInterface()
{
    // Free the llama model context if it has been created.
    if (m_context)
    {
        llama_free(m_context);
        m_context = nullptr;
    }

    if (m_model)
    {
        llama_model_free(m_model);
        m_model = nullptr;
    }
}

bool  LlamaInterface::loadModel(const QString &modelFile)
{
    // Create a set of parameters for the llama context.
    llama_model_params  params = llama_model_default_params();
    // (Optionally tweak params here; for example: params.seed = 0;)

    // Initialize the model from file.
    m_model = llama_model_load_from_file(modelFile.toUtf8().constData(), params);

    if (!m_model)
    {
        qWarning() << "Failed to load model:" << modelFile;

        return false;
    }

    // Create a context for the model.
    llama_context_params  ctx_params = llama_context_default_params();

    m_context = llama_new_context_with_model(m_model, ctx_params);

    if (!m_context)
    {
        qWarning() << "Failed to create context for model:" << modelFile;
        llama_model_free(m_model);
        m_model = nullptr;

        return false;
    }

    m_vocab = llama_model_get_vocab(m_model);

    emit  modelLoaded();

    return true;
}

QString  LlamaInterface::askQuestion(const QString &question)
{
    if (!m_context || !m_model)
    {
        qWarning() << "Model or context not loaded.";

        return QString();
    }

    // Convert the question to a format suitable for the model.
    std::string  input    = question.toStdString();
    const char  *text     = input.c_str();
    int32_t      text_len = static_cast<int32_t>(input.length());

    // Allocate a buffer for the tokens.
    const int32_t             n_tokens_max = 1024; // Adjust this size as needed.
    std::vector<llama_token>  tokens(n_tokens_max);

    // Tokenize the input text.
    int32_t  n_tokens = llama_tokenize(
        m_vocab, // Get the vocabulary from the model.
        text,    // Input text.
        text_len, // Length of the input text.
        tokens.data(), // Buffer to store the tokens.
        n_tokens_max, // Maximum number of tokens.
        true,    // Add special tokens (e.g., BOS, EOS).
        false    // Do not parse special tokens.
        );

    if (n_tokens < 0)
    {
        qWarning() << "Tokenization failed. Required buffer size:" << -n_tokens;

        return QString();
    }

    // Resize the tokens vector to the actual number of tokens.
    tokens.resize(n_tokens);

    // Create a llama_batch structure for the tokens.
    llama_batch  batch = llama_batch_init(tokens.size(), 0, 0);

    // Fill the batch with the tokens.
    for (int32_t i = 0; i < tokens.size(); ++i)
    {
        batch.token[i]  = tokens[i];
        batch.pos[i]    = i;
        batch.seq_id[i] = 0;
        batch.logits[i] = (i == tokens.size() - 1); // Only generate logits for the last token.
    }

    // Evaluate the batch using the llama_decode function.
    int32_t  decode_result = llama_decode(m_context, batch);

    if (decode_result != 0)
    {
        qWarning() << "Failed to evaluate tokens. Decode result:" << decode_result;
        llama_batch_free(batch); // Free the batch memory.

        return QString();
    }

    // Generate a response.
    QString  answer;
    int      n_past = 0; // Number of tokens already evaluated.

    // while (true)
    // {
    //// Get the next token from the model.
    // llama_token  next_token = llama_sample(m_context, tokens.data(), tokens.size(), n_past);

    //// Break if the end-of-sequence token is generated.
    // if (next_token == llama_vocab_eos(m_vocab))
    // {
    // break;
    // }

    //// Convert the token to a string and append it to the answer.
    // const char *token_str = llama_vocab_get_text(m_vocab, next_token);

    // answer += QString::fromUtf8(token_str);

    //// Append the generated token to the tokens vector for the next iteration.
    // tokens.push_back(next_token);
    // n_past++;
    // }

    // Emit the answer ready signal.
    emit  answerReady(answer);

    return answer;
}
