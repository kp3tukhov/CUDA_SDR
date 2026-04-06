cmake_minimum_required(VERSION 3.16)
project(GPS_Signal_Processing C CUDA)

# Стандарты языков
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Архитектура GPU (Compute Capability)
set(CMAKE_CUDA_ARCHITECTURES native)

# Компилятор C и флаги
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -ffast-math -march=native")

# CUDA флаги
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")

# Поиск зависимостей
find_library(FFTW3F_LIB fftw3f REQUIRED)
find_library(MATH_LIB m REQUIRED)

# Поиск CUDA Toolkit для импортированной цели CUDA::cudart
find_package(CUDAToolkit REQUIRED)

if(NOT FFTW3F_LIB)
    message(FATAL_ERROR "FFTW3F library not found")
endif()

# Пути к исходникам
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(APP_DIR ${CMAKE_SOURCE_DIR}/app)

# Включаемые директории
include_directories(${SRC_DIR})

# === Исходники CORE ===
set(CORE_SRCS
    ${SRC_DIR}/core/utils.c
)

# === Исходники CORE CUDA ===
set(CORE_CUDA_SRCS
    ${SRC_DIR}/core/init_gpu.cu
)

# === Исходники DSP ===
set(DSP_SRCS
    ${SRC_DIR}/dsp/code.c
    ${SRC_DIR}/dsp/mixer.c
)

# === Исходники DSP CUDA ===
set(DSP_CUDA_SRCS
    ${SRC_DIR}/dsp/mixer_gpu.cu
)

# === Исходники IO ===
set(IO_SRCS
    ${SRC_DIR}/io/file_io.c
    ${SRC_DIR}/io/config_io.c
)

# === Исходники коррелятора ===
set(CORR_SRCS
    ${SRC_DIR}/correlator/correlator.c
    ${SRC_DIR}/correlator/corr_interface.c
    ${SRC_DIR}/correlator/corr_accumulator.c
)

# === Исходники acquisition ===
set(ACQ_SRCS
    ${SRC_DIR}/acquisition/acq_processor.c
    ${SRC_DIR}/acquisition/peak_detection.c
    ${SRC_DIR}/acquisition/cn0_estimator.c
)

# === CUDA библиотека (отдельная static library) ===
# Важно: CUDA файлы должны компилироваться отдельно
add_library(cuda_mixers STATIC ${DSP_CUDA_SRCS})
set_target_properties(cuda_mixers PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(cuda_mixers PUBLIC ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

# === gen_sig (без CUDA) ===
add_executable(gen_sig
    ${APP_DIR}/gen_sig.c
    ${DSP_SRCS}
    ${IO_SRCS}
    ${CORE_SRCS}
)
target_link_libraries(gen_sig ${FFTW3F_LIB} ${MATH_LIB})

# === corr_sig (с CUDA) ===
add_executable(corr_sig
    ${APP_DIR}/corr_sig.c
    ${CORR_SRCS}
    ${DSP_SRCS}
    ${IO_SRCS}
    ${CORE_SRCS}
)
target_link_libraries(corr_sig
    cuda_mixers
    ${FFTW3F_LIB}
    ${MATH_LIB}
    CUDA::cudart
)

# === acq_sig (с CUDA) ===
add_executable(acq_sig
    ${APP_DIR}/acq_sig.c
    ${ACQ_SRCS}
    ${CORR_SRCS}
    ${DSP_SRCS}
    ${IO_SRCS}
    ${CORE_SRCS}
)
target_link_libraries(acq_sig
    cuda_mixers
    ${FFTW3F_LIB}
    ${MATH_LIB}
    CUDA::cudart
)

# Вывод исполняемых файлов в корневую директорию
set_target_properties(gen_sig corr_sig acq_sig PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
