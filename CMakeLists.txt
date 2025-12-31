cmake_minimum_required (VERSION 3.25)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Azazel)

include(CTest)

# Disable unit tests in nlohmann_json
set(JSON_BuildTests OFF CACHE INTERNAL "")

# Disabe unnesessary features in llama.cpp
set(LLAMMA_BUILD_TESTS OFF CACHE INTERNAL "")
set(LLAMA_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(LLAMA_BUILD_TOOLS OFF CACHE INTERNAL "")
set(LLAMA_BUILD_COMMON OFF CACHE INTERNAL "")

# Enable LLama.cpp features
#set(LLAMA_LLGUIDANCE ON CACHE BOOL "Enable LLGuidance for structured output")

# Source files list
set(SOURCE_DIR src/dateTime.cpp src/dateTime.h src/model.cpp src/model.h 
    src/functionCall.cpp src/commandList.cpp src/functionCall.h
    src/configReader.cpp src/configReader.h src/configVars.h
    src/mqtt.cpp src/mqtt.h src/voice.cpp src/voice.h)

set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/tests)

include_directories(${CMAKE_SOURCE_DIR}/lib/libpiper/include)

add_subdirectory(lib/llama.cpp)
add_subdirectory(lib/json)
add_subdirectory(lib/libpiper)

add_executable(Azazel main.cpp ${SOURCE_DIR})

# Include external libraries
target_link_libraries(Azazel PRIVATE llama)
target_link_libraries(Azazel PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Azazel PRIVATE mosquitto)
target_link_libraries(Azazel PRIVATE piper)

# Main executable stays in the project root
set_target_properties(Azazel PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Enable testing
set(TEST_EXECUTABLES testConfigReader testDateTime testModel testMqtt testFunctionCall testVoiceTTS)

# Add tests
foreach(test_exec ${TEST_EXECUTABLES})
    set(TEST_SOURCES_${test_exec} tests/${test_exec}.cpp ${SOURCE_DIR})
    add_executable(${test_exec} ${TEST_SOURCES_${test_exec}})
    target_link_libraries(${test_exec} PRIVATE llama)
    target_link_libraries(${test_exec} PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(${test_exec} PRIVATE mosquitto)
    target_link_libraries(${test_exec} PRIVATE piper)
    set_target_properties(${test_exec} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    add_test(NAME ${test_exec} COMMAND ${test_exec})
endforeach()