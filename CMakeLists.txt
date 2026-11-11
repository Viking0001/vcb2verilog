cmake_minimum_required(VERSION 3.14)
project(vcb2verilog VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization and warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-switch -Wno-unused-variable -Wno-parentheses)
endif()

# Optional OpenMP for parallel grid processing
find_package(OpenMP)

# Dependencies: zstd and glm
find_package(PkgConfig QUIET)
find_package(zstd QUIET)

if(NOT TARGET zstd::libzstd_shared AND NOT TARGET zstd::libzstd_static)
    find_library(ZSTD_LIBRARY NAMES zstd libzstd)
    find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)
endif()

find_package(glm QUIET)
if(NOT TARGET glm::glm)
    find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp)
endif()

# Project source files
set(SOURCES
    src/main.cpp
    src/verilog_emitter.cpp
    src/openVCB.cpp
    src/openVCBReader.cpp
    src/openVCBBlueprint.cpp
    src/openVCBPreprocessing.cpp
    src/openVCBAssembler.cpp
    src/openVCBExpr.cpp
    gorder/Graph.cpp
    gorder/Util.cpp
    gorder/UnitHeap.cpp
)

add_executable(vcb2verilog ${SOURCES})

target_include_directories(vcb2verilog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(ZSTD_INCLUDE_DIR)
    target_include_directories(vcb2verilog PRIVATE ${ZSTD_INCLUDE_DIR})
endif()
if(GLM_INCLUDE_DIR)
    target_include_directories(vcb2verilog PRIVATE ${GLM_INCLUDE_DIR})
endif()

# Link libzstd
if(TARGET zstd::libzstd_shared)
    target_link_libraries(vcb2verilog PRIVATE zstd::libzstd_shared)
elseif(TARGET zstd::libzstd_static)
    target_link_libraries(vcb2verilog PRIVATE zstd::libzstd_static)
elseif(ZSTD_LIBRARY)
    target_link_libraries(vcb2verilog PRIVATE ${ZSTD_LIBRARY})
else()
    target_link_libraries(vcb2verilog PRIVATE zstd)
endif()

# Link GLM
if(TARGET glm::glm)
    target_link_libraries(vcb2verilog PRIVATE glm::glm)
endif()

# Link OpenMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(vcb2verilog PRIVATE OpenMP::OpenMP_CXX)
endif()

# Install target
install(TARGETS vcb2verilog DESTINATION bin)
