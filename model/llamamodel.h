#ifndef LLAMAMODEL_H
#define LLAMAMODEL_H

#include <QObject>
#include <QString>


// Forward declarations: use the appropriate types if they’re defined in the llama headers
struct llama_model;
struct llama_context;
struct llama_vocab;
struct llama_sampler_chain_params;
struct llama_sampler;
struct llama_chat_message;

class LlamaInterface: public QObject
{
    Q_OBJECT

public:
    explicit LlamaInterface(QObject *parent = nullptr);

    ~LlamaInterface();

    // Load the model from the given file path. Returns true if loaded.
    bool  loadModel(const QString &modelFile);

public  slots:
    // Ask a question and return an answer. (This is a simple synchronous method;
    // in a production app you might want asynchronous generation.)
    void         generate(const QString &prompt);

    std::string  askQuestion(const std::string &prompt);

signals:
    // Emitted when the model is loaded
    void         modelLoaded();

    // Emitted when a generated answer is ready
    void         answerReady(const QString &answer);

    void         generateFinished(std::string);

    void         errorOccure(QString);

private:
    // Pointer to the underlying llama context.
    // (Depending on your version of llama.cpp, this might be a
    // 'llama_context*', but here we use void* for generality.)
    llama_context                   *m_context = nullptr;
    llama_model                     *m_model   = nullptr;
    llama_sampler                   *m_sampler = nullptr;
    const struct llama_vocab        *m_vocab   = nullptr;
    std::vector<llama_chat_message>  m_messages;
    std::vector<char>                m_formatted;
    int                              m_n_prompt = 0;
    int                              m_prev_len = 0;
};

#endif // LLAMAMODEL_H
