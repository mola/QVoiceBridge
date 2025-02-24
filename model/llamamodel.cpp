#include "llamamodel.h"

#include "llama.h"
#include <QDebug>

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

    m_context = llama_new_context_with_model(m_model, ctx_params);

    if (!m_context)
    {
        qWarning() << "Failed to create context for model:" << modelFile;
        llama_model_free(m_model);
        m_model = nullptr;

        return false;
    }

    auto  m_samplerParams = llama_sampler_chain_default_params();

    m_sampler = llama_sampler_chain_init(m_samplerParams);
    llama_sampler_chain_add(m_sampler, llama_sampler_init_greedy());

    emit  modelLoaded();

    return true;
}

void  LlamaInterface::askQuestion(const QString &question)
{
    std::string  prompt = question.toStdString();
    std::string  response;
    QString      answer;
    const bool   is_first = llama_get_kv_cache_used_cells(m_context) == 0;

    // tokenize the prompt
    const int                 n_prompt_tokens = -llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token>  prompt_tokens(n_prompt_tokens);

    if (llama_tokenize(m_vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0)
    {
        GGML_ABORT("failed to tokenize the prompt\n");
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
            printf("\033[0m\n");
            fprintf(stderr, "context size exceeded\n");
            exit(0);
        }

        if (llama_decode(m_context, batch))
        {
            GGML_ABORT("failed to decode\n");
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
            GGML_ABORT("failed to convert token to piece\n");
        }

        std::string  piece(buf, n);

        answer = QString::fromStdString(piece);

        emit  answerReady(answer);

        // printf("%s", piece.c_str());
        // fflush(stdout);
        // response += piece;

        // prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    // QString  answer = QString::fromStdString(response);
    // Emit the answer ready signal.
    // emit  answerReady(answer);

    // return answer;
}
