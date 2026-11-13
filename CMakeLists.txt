cmake_minimum_required(VERSION 3.25)

project(english_listening_backend
        VERSION 0.1.0
        DESCRIPTION "Local backend service for English Listening desktop app"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_library(SQLITE3_LIBRARY NAMES sqlite3 REQUIRED)

add_executable(el-backend
        src/main.cpp
        src/Server.cpp
        src/config/Config.cpp
        src/core/Errors.cpp
        src/core/Http.cpp
        src/core/Logger.cpp
        src/core/Handshake.cpp
        src/db/Database.cpp
        src/modules/wordbank/WordBankRepository.cpp
        src/modules/wordbank/WordBankService.cpp
        src/modules/wordbank/WordBankController.cpp
        src/modules/health/HealthRoutes.cpp
        src/modules/tasks/TaskService.cpp
        src/ws/ProgressHub.cpp)

target_include_directories(el-backend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

target_compile_definitions(el-backend PRIVATE ASIO_STANDALONE)

target_link_libraries(el-backend PRIVATE
        spdlog::spdlog
        fmt::fmt
        OpenSSL::Crypto
        ${SQLITE3_LIBRARY}
        Threads::Threads)

if(WIN32)
    target_link_libraries(el-backend PRIVATE ws2_32 wsock32 crypt32)
endif()

if(MSVC)
    target_compile_options(el-backend PRIVATE /W4 /permissive-)
else()
    target_compile_options(el-backend PRIVATE -Wall -Wextra -Wpedantic)
endif()
