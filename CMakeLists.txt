cmake_minimum_required(VERSION 3.16)
project(polymarket_bot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)

# nlohmann/json (header-only library)
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    # If not found, try to find it via pkg-config
    pkg_check_modules(NLOHMANN_JSON QUIET nlohmann_json)
    if(NLOHMANN_JSON_FOUND)
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIRS})
        target_link_libraries(nlohmann_json INTERFACE ${NLOHMANN_JSON_LIBRARIES})
    else()
        # Fallback: include as third-party
        include(FetchContent)
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.2
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
endif()

# SQLite3
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # Try pkg-config
    pkg_check_modules(SQLITE3 QUIET sqlite3)
    if(SQLITE3_FOUND)
        add_library(SQLite3 INTERFACE)
        target_include_directories(SQLite3 INTERFACE ${SQLITE3_INCLUDE_DIRS})
        target_link_libraries(SQLite3 INTERFACE ${SQLITE3_LIBRARIES})
    else()
        # Fallback: download and build
        include(FetchContent)
        FetchContent_Declare(
            sqlite3
            URL https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip
            URL_HASH SHA256=7a30dc588c6f9c8e5ae361cfc0a2d2689b3d40b8f3c3b9c8c8c8c8c8c8c8c8c8c8
        )
        FetchContent_MakeAvailable(sqlite3)
        add_library(SQLite3 ALIAS sqlite3)
    endif()
endif()

# cURL for HTTP requests
find_package(CURL REQUIRED)

# Threading
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files (excluding main.cpp)
file(GLOB_RECURSE LIBRARY_SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# Remove main.cpp from library sources
list(REMOVE_ITEM LIBRARY_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Create library
add_library(polymarket_bot_lib STATIC ${LIBRARY_SOURCES})

# Create main executable
add_executable(polymarket_bot src/main.cpp)

# Link libraries for the main executable
target_link_libraries(polymarket_bot
    polymarket_bot_lib
    nlohmann_json::nlohmann_json
    SQLite3
    CURL::libcurl
    Threads::Threads
)

# Link libraries for the library
target_link_libraries(polymarket_bot_lib
    nlohmann_json::nlohmann_json
    SQLite3
    CURL::libcurl
    Threads::Threads
)

# Set output directories
set_target_properties(polymarket_bot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Copy config files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/bin)
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})

# Environment Variables
# The application expects these environment variables to be set in the system:
# - ODDS_API_KEY: Your odds API key
# - POLY_ADDRESS: Your Polygon address
# - POLY_SIGNATURE: Your HMAC signature
# - POLY_TIMESTAMP: Current UNIX timestamp
# - POLY_API_KEY: Your Polymarket API key
# - POLY_PASSPHRASE: Your Polymarket passphrase

# Install rules
install(TARGETS polymarket_bot
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY config/ DESTINATION etc/polymarket_bot)

# Testing
enable_testing()

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Google Test executable
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_odds_api_client.cpp")
    add_executable(polymarket_bot_gtest tests/test_odds_api_client.cpp)
    target_link_libraries(polymarket_bot_gtest
        polymarket_bot_lib
        nlohmann_json::nlohmann_json
        SQLite3
        CURL::libcurl
        Threads::Threads
        gtest
        gtest_main
        gmock
        gmock_main
    )
    add_test(NAME GoogleTests COMMAND polymarket_bot_gtest)
endif()

# Simple test executable
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_odds_api_client_simple.cpp")
    add_executable(polymarket_bot_simple_tests tests/test_odds_api_client_simple.cpp)
    target_link_libraries(polymarket_bot_simple_tests
        polymarket_bot_lib
        nlohmann_json::nlohmann_json
        SQLite3
        CURL::libcurl
        Threads::Threads
    )
    add_test(NAME SimpleTests COMMAND polymarket_bot_simple_tests)
endif()

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
