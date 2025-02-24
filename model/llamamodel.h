#ifndef LLAMAMODEL_H
#define LLAMAMODEL_H

#include <QObject>
#include <QString>

// Forward declarations: use the appropriate types if they’re defined in the llama headers
struct llama_model;
struct llama_context;
struct llama_vocab;

class LlamaInterface : public QObject
{
    Q_OBJECT
public:
    explicit LlamaInterface(QObject *parent = nullptr);
    ~LlamaInterface();

    // Load the model from the given file path. Returns true if loaded.
    bool loadModel(const QString &modelFile);

    // Ask a question and return an answer. (This is a simple synchronous method;
    // in a production app you might want asynchronous generation.)
    QString askQuestion(const QString &question);

signals:
    // Emitted when the model is loaded
    void modelLoaded();

    // Emitted when a generated answer is ready
    void answerReady(const QString &answer);

private:
    // Pointer to the underlying llama context.
    // (Depending on your version of llama.cpp, this might be a
    // 'llama_context*', but here we use void* for generality.)
    llama_context* m_context = nullptr;
    llama_model*  m_model = nullptr;
    const struct llama_vocab * m_vocab = nullptr;

};

#endif // LLAMAMODEL_H
