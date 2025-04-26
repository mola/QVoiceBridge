#include "llamamodel.h"

#include "llama.h"
#include <QDebug>
#include "document.h"

LlamaInterface::LlamaInterface(QObject *parent):
    QObject(parent), m_context(nullptr)
{
    // ggml_backend_load_all();
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
    params.n_gpu_layers = 99;

    // (Optionally tweak params here; for example: params.seed = 0;)

    // Initialize the model from file.
    m_model = llama_model_load_from_file(modelFile.toUtf8().constData(), params);

    if (!m_model)
    {
        qWarning() << "Failed to load model:" << modelFile;

        return false;
    }

    m_vocab = llama_model_get_vocab(m_model);

    // Create a context for the model.
    llama_context_params  ctx_params = llama_context_default_params();

    ctx_params.n_ctx   = 2048;
    ctx_params.n_batch = 2048;

    m_context = llama_init_from_model(m_model, ctx_params);

    if (!m_context)
    {
        qWarning() << "Failed to create context for model:" << modelFile;
        llama_model_free(m_model);
        m_model = nullptr;

        return false;
    }

    auto  m_samplerParams = llama_sampler_chain_default_params();

    m_sampler = llama_sampler_chain_init(m_samplerParams);
    llama_sampler_chain_add(m_sampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    llama_sampler_chain_add(m_sampler, llama_sampler_init_greedy());

    m_formatted = std::vector<char>(llama_n_ctx(m_context));

    emit  modelLoaded();

    m_messages.push_back({ "system", strdup(documents.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus1.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus2.c_str()) });
    // m_messages.push_back({ "system", strdup(documentStatus3.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus4.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus5.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus5.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus6.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus7.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus8.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus9.c_str()) });
    m_messages.push_back({ "system", strdup(documentStatus10.c_str()) });

    return true;
}

void  LlamaInterface::generate(const QString &msg)
{
    std::string  message = msg.toStdString();

    const char *tmpl = llama_model_chat_template(m_model, /* name */ nullptr);
    m_messages.push_back({ "user", strdup(message.c_str()) });

    int  new_len = llama_chat_apply_template(tmpl, m_messages.data(), m_messages.size(), true, m_formatted.data(), m_formatted.size());

    if (new_len > (int)m_formatted.size())
    {
        m_formatted.resize(new_len);
        new_len = llama_chat_apply_template(tmpl, m_messages.data(), m_messages.size(), true, m_formatted.data(), m_formatted.size());
    }

    // remove previous messages to obtain the prompt to generate the response
    std::string  prompt(m_formatted.begin() + m_prev_len, m_formatted.begin() + new_len);

    std::string  response = askQuestion(prompt);

    m_messages.push_back({ "assistant", strdup(response.c_str()) });
    m_prev_len = llama_chat_apply_template(tmpl, m_messages.data(), m_messages.size(), false, nullptr, 0);

    emit  generateFinished(response);
}

std::string  LlamaInterface::askQuestion(const std::string &prompt)
{
    std::string  response;
    std::string  answer;

    const bool  is_first = llama_get_kv_cache_used_cells(m_context) == 0;

    // tokenize the prompt
    const int                 n_prompt_tokens = -llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token>  prompt_tokens(n_prompt_tokens);

    if (llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0)
    {
        emit  errorOccure("failed to tokenize the prompt");

        return answer;
    }

    // prepare a batch for the prompt
    llama_batch  batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
    llama_token  new_token_id;

    while (true)
    {
        // check if we have enough space in the context to evaluate this batch
        int  n_ctx      = llama_n_ctx(m_context);
        int  n_ctx_used = llama_get_kv_cache_used_cells(m_context);

        if (n_ctx_used + batch.n_tokens > n_ctx)
        {
            emit  errorOccure("context size exceeded");

            break;
        }

        if (llama_decode(m_context, batch))
        {
            emit  errorOccure("failed to decode");

            break;
        }

        // sample the next token
        new_token_id = llama_sampler_sample(m_sampler, m_context, -1);

        // is it an end of generation?
        if (llama_vocab_is_eog(m_vocab, new_token_id))
        {
            break;
        }

        // convert the token to a string, print it and add it to the response
        char  buf[256];
        int   n = llama_token_to_piece(m_vocab, new_token_id, buf, sizeof(buf), 0, true);

        if (n < 0)
        {
            emit  errorOccure("failed to convert token to piece");

            break;
        }

        std::string  piece(buf, n);

        answer.append(piece);

        emit  answerReady(QString::fromStdString(piece));

        // prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    return answer;
}
