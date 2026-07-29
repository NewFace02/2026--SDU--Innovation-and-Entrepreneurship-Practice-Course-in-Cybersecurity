cmake_minimum_required(VERSION 3.22)
project(fhe_conv_seal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SEAL 4.3 REQUIRED)

add_executable(fhe_conv src/main.cpp)
target_link_libraries(fhe_conv PRIVATE SEAL::seal)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fhe_conv PRIVATE -O3 -march=native -Wall -Wextra -Wpedantic)
endif()
