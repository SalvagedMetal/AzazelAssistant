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
set(SOURCE_DIR src/dateTime.cpp src/dateTime.h src/model.cpp src/model.h src/functionCall.cpp src/functionCall.h src/configReader.cpp src/configReader.h src/configVars.h)
set(TESTS_DIR tests/tests.cpp tests/testDateTime.cpp tests/testDateTime.h tests/testModel.cpp tests/testModel.h tests/testFunctionCall.cpp tests/testFunctionCall.h)

add_subdirectory(lib/llama.cpp)
add_subdirectory(lib/json)

add_executable(Azazel main.cpp ${SOURCE_DIR})
# Include external libraries
target_link_libraries(Azazel PRIVATE llama)
target_link_libraries(Azazel PRIVATE nlohmann_json::nlohmann_json)

add_executable(testConfigReader tests/testConfigReader.cpp ${SOURCE_DIR})
add_executable(Azazel_test ${TESTS_DIR} ${SOURCE_DIR})
# Include external libraries
target_link_libraries(testConfigReader PRIVATE llama)
target_link_libraries(testConfigReader PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Azazel_test PRIVATE llama)
target_link_libraries(Azazel_test PRIVATE nlohmann_json::nlohmann_json)

# Enable testing
add_test(NAME testConfigReader COMMAND testConfigReader)
add_test(NAME Azazel_test COMMAND Azazel_test)