cmake_minimum_required(VERSION 3.13.1)

set(COMPONENT_DIR ${CMAKE_CURRENT_LIST_DIR})

# Include the utility macros from edge-impulse SDK
include(${COMPONENT_DIR}/edge-impulse-sdk/cmake/utils.cmake)

# Find the model source files
RECURSIVE_FIND_FILE_APPEND(MODEL_SOURCE "${COMPONENT_DIR}/tflite-model" "*.cpp")

# Find all source files from critical SDK directories
file(GLOB_RECURSE EI_DSP_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/dsp/*.cpp")
file(GLOB_RECURSE EI_DSP_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/dsp/*.c")
file(GLOB_RECURSE EI_PORTING_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/porting/*.cpp")
file(GLOB_RECURSE EI_PORTING_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/porting/*.c")
# TensorFlow Lite Micro uses both .cpp and .cc extensions
file(GLOB_RECURSE EI_TFLITE_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/micro/*.cpp" "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/micro/*.cc")
file(GLOB_RECURSE EI_TFLITE_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/micro/*.c")
# Kernels use .cc extension - this is critical for linking
file(GLOB_RECURSE EI_TFLITE_KERNELS_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/*.cc")
file(GLOB_RECURSE EI_TFLITE_KERNELS_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/*.c")
# Core also uses both extensions
file(GLOB_RECURSE EI_TFLITE_CORE_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/core/*.cpp" "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/core/*.cc")
file(GLOB_RECURSE EI_TFLITE_CORE_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/core/*.c")
file(GLOB_RECURSE EI_TFLITE_SCHEMA_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/schema/*.cpp")
file(GLOB_RECURSE EI_TFLITE_SCHEMA_C_SOURCES "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/schema/*.c")
# Kernel utilities and internal implementations (must be included for linker resolution)
file(GLOB EI_TFLITE_KERNELS_UTIL "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/*.cc")
file(GLOB_RECURSE EI_TFLITE_KERNELS_INTERNAL "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/internal/*.cc")
file(GLOB_RECURSE EI_TFLITE_KERNELS_CUSTOM "${COMPONENT_DIR}/edge-impulse-sdk/tensorflow/lite/kernels/custom/*.cc")

# Combine all sources (exclude CMSIS)
list(FILTER EI_DSP_SOURCES EXCLUDE REGEX ".*CMSIS.*")
list(FILTER EI_DSP_C_SOURCES EXCLUDE REGEX ".*CMSIS.*")
list(FILTER EI_TFLITE_SOURCES EXCLUDE REGEX ".*CMSIS.*|.*test.*")
list(FILTER EI_TFLITE_C_SOURCES EXCLUDE REGEX ".*CMSIS.*|.*test.*")
list(FILTER EI_TFLITE_KERNELS_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER EI_TFLITE_KERNELS_C_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER EI_TFLITE_CORE_SOURCES EXCLUDE REGEX ".*CMSIS.*|.*test.*")
list(FILTER EI_TFLITE_CORE_C_SOURCES EXCLUDE REGEX ".*CMSIS.*|.*test.*")

set(ALL_SOURCES 
    ${EI_DSP_SOURCES} ${EI_DSP_C_SOURCES}
    ${EI_PORTING_SOURCES} ${EI_PORTING_C_SOURCES}
    ${EI_TFLITE_SOURCES} ${EI_TFLITE_C_SOURCES}
    ${EI_TFLITE_KERNELS_SOURCES} ${EI_TFLITE_KERNELS_C_SOURCES}
    ${EI_TFLITE_KERNELS_UTIL}
    ${EI_TFLITE_KERNELS_INTERNAL}
    ${EI_TFLITE_KERNELS_CUSTOM}
    ${EI_TFLITE_CORE_SOURCES} ${EI_TFLITE_CORE_C_SOURCES}
    ${EI_TFLITE_SCHEMA_SOURCES} ${EI_TFLITE_SCHEMA_C_SOURCES}
    ${MODEL_SOURCE})

# Configurar flags de optimización y aceleración
set(EI_COMPILE_DEFINITIONS
    EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN=1
    EI_CLASSIFIER_TFLITE_LOAD_CMSIS_NN=0
)

# Register as ESP-IDF component
idf_component_register(
    SRCS ${ALL_SOURCES}
    INCLUDE_DIRS 
        . 
        edge-impulse-sdk
        edge-impulse-sdk/dsp
        edge-impulse-sdk/classifier
        edge-impulse-sdk/tensorflow
        edge-impulse-sdk/tensorflow/lite
        edge-impulse-sdk/tensorflow/lite/micro
        edge-impulse-sdk/tensorflow/lite/micro/kernels
        edge-impulse-sdk/tensorflow/lite/kernels
        edge-impulse-sdk/tensorflow/lite/kernels/internal
        edge-impulse-sdk/tensorflow/lite/core
        edge-impulse-sdk/tensorflow/lite/core/api
        edge-impulse-sdk/tensorflow/lite/schema
        edge-impulse-sdk/porting
        edge-impulse-sdk/porting/espressif
        model-parameters
        tflite-model
        tflite-model
    REQUIRES
        esp_timer
        driver
        espressif__esp-dsp)

target_compile_definitions(${COMPONENT_LIB} PUBLIC ${EI_COMPILE_DEFINITIONS})
