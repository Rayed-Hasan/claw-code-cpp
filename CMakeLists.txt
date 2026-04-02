cmake_minimum_required(VERSION 3.20)
project(claw-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies via FetchContent ─────────────────────────────────────────

include(FetchContent)

# nlohmann/json — industry standard JSON library
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# cpp-httplib — lightweight HTTP/HTTPS client
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
    GIT_SHALLOW    TRUE
)

# CLI11 — modern CLI argument parser
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.1
    GIT_SHALLOW    TRUE
)

# fmt — type-safe formatting (with color support)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(json httplib cli11 fmt)

# ── OpenSSL for HTTPS support ─────────────────────────────────────────────

find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found — HTTPS enabled")
else()
    message(WARNING
        "OpenSSL NOT found — HTTPS will NOT work.\n"
        "Install OpenSSL and re-run cmake, or set OPENSSL_ROOT_DIR.\n"
        "On Windows: winget install ShiningLight.OpenSSL"
    )
endif()

# ── Core static library ──────────────────────────────────────────────────

add_library(claw_core STATIC
    src/api/client.cpp
    src/runtime/session.cpp
    src/tools/bash_tool.cpp
    src/tools/file_read_tool.cpp
    src/tools/file_write_tool.cpp
)

target_include_directories(claw_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(claw_core PUBLIC
    nlohmann_json::nlohmann_json
    httplib::httplib
    fmt::fmt
)

# Link OpenSSL if available and enable HTTPS in cpp-httplib
if(OPENSSL_FOUND)
    target_compile_definitions(claw_core PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(claw_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

# Platform-specific networking libraries
if(WIN32)
    target_link_libraries(claw_core PUBLIC ws2_32 crypt32)
endif()

# ── Main executable ──────────────────────────────────────────────────────

add_executable(claw-cpp
    src/cli/main.cpp
)

target_link_libraries(claw-cpp PRIVATE
    claw_core
    CLI11::CLI11
)
