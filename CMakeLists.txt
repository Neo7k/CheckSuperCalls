cmake_minimum_required(VERSION 3.10)
project(CheckSuperCalls)
aux_source_directory(CheckSuperCalls SRCS)
add_executable(CheckSuperCalls ${SRCS})
target_compile_features(CheckSuperCalls PUBLIC cxx_std_17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(CheckSuperCalls PRIVATE Threads::Threads)
