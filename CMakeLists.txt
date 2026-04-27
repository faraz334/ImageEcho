# CMakeLists.txt
# This file tells CMake (our build tool) how to compile the project.
# Think of it like a recipe: "here are the ingredients, here's what to make."

cmake_minimum_required(VERSION 3.16)   # minimum CMake version required

project(ImageEcho VERSION 1.0)         # name of our project

# Tell the compiler to use C++17 (modern C++ with useful features)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Turn on helpful warnings so the compiler tells us about mistakes
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ── Main executable ──────────────────────────────────────────────────────────
# This says: "build an exe called ImageEcho from src/main.cpp"
add_executable(ImageEcho
    src/main.cpp
)

# Tell the compiler where to find our header files (.hpp files)
target_include_directories(ImageEcho PRIVATE
    include        # our own headers
    third_party    # stb_image lives here
)

target_compile_options(ImageEcho PRIVATE -w)