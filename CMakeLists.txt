cmake_minimum_required(VERSION 3.16)
project(pqc_v2x_simulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(OQS_BUILD_ONLY_LIB ON CACHE BOOL "" FORCE)
set(OQS_MINIMAL_BUILD "SIG_ml_dsa_44;SIG_ml_dsa_65;SIG_ml_dsa_87;SIG_falcon_512;SIG_falcon_1024;SIG_sphincs_sha2_128s_simple;KEM_ml_kem_512;KEM_ml_kem_768;KEM_ml_kem_1024" CACHE STRING "" FORCE)
set(OQS_USE_OPENSSL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    liboqs
    GIT_REPOSITORY https://github.com/open-quantum-safe/liboqs.git
    GIT_TAG 0.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(liboqs)

add_library(v2x_core
    src/crypto/signature_scheme.cpp
    src/crypto/kem_scheme.cpp
    src/metrics/metrics.cpp
    src/network/message.cpp
    src/network/vehicle.cpp
    src/output/writers.cpp
    src/simulation/configuration.cpp
    src/simulation/simulator.cpp
)
target_include_directories(v2x_core PUBLIC include PRIVATE "${liboqs_BINARY_DIR}/include")
target_link_libraries(v2x_core PUBLIC oqs)

add_executable(pqc_v2x_simulator src/main.cpp)
target_link_libraries(pqc_v2x_simulator PRIVATE v2x_core)

add_executable(pqc_v2x_tests tests/test_main.cpp)
target_link_libraries(pqc_v2x_tests PRIVATE v2x_core)
enable_testing()
add_test(NAME simulator_tests COMMAND pqc_v2x_tests)

if (MSVC)
    target_compile_options(v2x_core PRIVATE /W4 /permissive-)
    target_compile_options(pqc_v2x_simulator PRIVATE /W4 /permissive-)
    target_compile_options(pqc_v2x_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(v2x_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pqc_v2x_simulator PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pqc_v2x_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
