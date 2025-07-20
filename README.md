# AzazelAssistant
## v0.1 Basic Working Release
AzazelAssistant is a local, privacy-respecting AI assistant built on top of [llama.cpp](https://github.com/ggml-org/llama.cpp) for efficient LLM inference and [nlohmann/json](https://github.com/nlohmann/json) for lightweight JSON handling. It provides a simple, fast, and extensible interface for interacting with large language models (LLMs) on your own machine—no cloud required.

---

## Features

- Runs fully locally (offline support)
- LLM backend using [llama.cpp](https://github.com/ggml-org/llama.cpp)
- Easy model management via `models/` directory
- Lightweight C++ architecture
- JSON-based configuration using [nlohmann/json](https://github.com/nlohmann/json)
- Designed for speed and simplicity

---

## Dependencies

This project relies on:

- [`llama.cpp`](https://github.com/ggml-org/llama.cpp): Local LLM inference engine
- [`nlohmann/json`](https://github.com/nlohmann/json): Modern C++ JSON library

These libraries are included manually under a `lib/` subdirectory.

---

## Installation

### 1. Clone the Repository

```
git clone https://github.com/SalvagedMetal/AzazelAssistant.git
cd AzazelAssistant
```
### 2. Set Up External Libraries
Create the lib/ directory and clone required dependencies:

```
mkdir lib
cd lib
```
Clone llama.cpp
```
git clone https://github.com/ggml-org/llama.cpp
```
Clone nlohmann/json
```
git clone https://github.com/nlohmann/json
```
### 3. Download and Install a Model

Install pkgx if you don't already have it:
```
curl -fsS https://pkgx.sh | sh
```
Create the models/ directory and install the model using pkgx:
```
mkdir models
cd models
pkgx huggingface-cli download unsloth/Phi-4-mini-instruct-GGUF  Phi-4-mini-instruct-Q6_K.gguf --local-dir .
````

## Building the Assistant

```
mkdir build
cd build
cmake --build .
```

## Project Structure

```
AzazelAssistant/
├── lib/
│   ├── llama.cpp/           # LLM backend
│   └── json/                # JSON library
├── models/                  # Place GGUF models here
├── src/                     # Core source code
├── include/                 # Headers
├── tests/                   # Unit test files
```

## External Libraries
### ggml-org/llama.cpp - High-performance inference for LLaMA models in C/C++
https://github.com/ggml-org/llama.cpp
### nlohmann/json - JSON for Modern C++
https://github.com/nlohmann/json
## License
Check the LICENSE file for licensing details.
