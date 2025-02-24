# QVoiceBridge
Implies bridging between voice input and text output (and vice versa)

```
sudo pacman -S qt6-speech
sudo pacman -S onnxruntime
sudo pacman -S nlohmann-json  

```


sudo pamac build libggml


git clone https://github.com/ggml-org/llama.cpp.git

cd llama.cpp
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr

git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr

