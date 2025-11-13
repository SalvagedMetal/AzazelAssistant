# AzazelAssistant
## v0.3 Configuration and Modularised Testing
AzazelAssistant is a local, privacy-respecting AI assistant built on top of [llama.cpp](https://github.com/ggml-org/llama.cpp) for efficient LLM inference and [nlohmann/json](https://github.com/nlohmann/json) for lightweight JSON handling. It provides a simple, fast, and extensible interface for interacting with large language models (LLMs) on your own machine—no cloud required. Can send commands to other devices through MQTT protocol.

---

## Features

- Runs fully locally (offline support)
- LLM backend using [llama.cpp](https://github.com/ggml-org/llama.cpp)
- Easy model management via `models/` directory
- Lightweight C++ architecture
- JSON-based configuration using [nlohmann/json](https://github.com/nlohmann/json)
- Designed for speed and simplicity
- MQTT Support for subscribing and publishing for messages to MQTT enabled devices

---

## Dependencies

This project relies on:

These libraries are included manually under a `lib/` subdirectory.

- [`llama.cpp`](https://github.com/ggml-org/llama.cpp): Local LLM inference engine
- [`nlohmann/json`](https://github.com/nlohmann/json): Modern C++ JSON library

This library is included in the `etc/` directory.

- [`eclipse-mosquitto`](https://github.com/eclipse-mosquitto/mosquitto): MQTT library for C++


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
Install Mosquitto library
```
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev
sudo systemctl enable --now mosquitto
sudo systemctl status mosquitto
```
If you want to run Moquitto local host also install mosquitto client
```
sudo apt update && sudo apt upgrade
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto.service
```
For more information follow the installation guide [here](https://randomnerdtutorials.com/how-to-install-mosquitto-broker-on-raspberry-pi)

How to use the mosquitto client [here](https://randomnerdtutorials.com/testing-mosquitto-broker-and-client-on-raspbbery-pi)

### 3. Download and Install a Model

Install pkgx if you don't already have it:
```
curl -fsS https://pkgx.sh | sh
```
Create the models/ directory and install the model using pkgx:
```
mkdir models
cd models
pkgx huggingface-cli download bartowski/microsoft_Phi-4-mini-instruct-GGUF  microsoft_Phi-4-mini-instruct-Q6_K_L.gguf --local-dir .
````

## Building the Assistant

```
cmake -B build -S .
cmake --build build
```
In order to run the application, once you are in the root directory of the project use
```
./Azazel
```
to run it.

## Project Structure

```
AzazelAssistant/
├── lib/
│   ├── llama.cpp/           # LLM backend
│   └── json/                # JSON library
├── models/                  # Place GGUF models here
├── src/                     # Core source code
├── tests/                   # Unit test files
```

## Testing
This project uses the CTest library to run the function tests.
```
cd build
ctest
```

## License
Check the LICENSE file for licensing details.
