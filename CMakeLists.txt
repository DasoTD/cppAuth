cmake_minimum_required(VERSION 3.16)
project(cppAuth LANGUAGES CXX)

# Ensure a valid C++ standard is set
if(NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD STREQUAL "")
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Drogon config package; if not available, we'll fallback to linking 'drogon'
find_package(Drogon CONFIG QUIET)

# Required packages
find_package(Jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL QUIET)
find_package(Threads REQUIRED)

# Collect sources
file(GLOB_RECURSE CPPAUTH_SOURCES
    ${CMAKE_SOURCE_DIR}/main.cc
    ${CMAKE_SOURCE_DIR}/controllers/*.cc
)

add_executable(${PROJECT_NAME} ${CPPAUTH_SOURCES})

# Build bundled libbcrypt and link it so BCrypt wrapper symbols resolve at link time
if(EXISTS "${CMAKE_SOURCE_DIR}/external/libbcrypt/CMakeLists.txt")
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/libbcrypt)
    # The external project creates a target named 'bcrypt'
    target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/libbcrypt/include)
endif()

# Include dirs
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/controllers
    ${CMAKE_SOURCE_DIR}/external/libbcrypt/include
    ${CMAKE_SOURCE_DIR}/jwt-cpp/include
    ${CMAKE_SOURCE_DIR}/external/dotenv-cpp/include
    ${CMAKE_SOURCE_DIR}/external
)

# Link Drogon: prefer CONFIG target if found, otherwise link 'drogon'
if (TARGET Drogon::Drogon)
    target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
else()
    # Fallback: assume a system-installed drogon library is available
    target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
endif()

# Link common libs if available
if (PostgreSQL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARIES})
endif()

if (TARGET Jsoncpp::Jsoncpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE Jsoncpp::Jsoncpp)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${JSONCPP_LIBRARIES})
endif()

find_package(spdlog QUIET)

if (SPDLOG_FOUND)
    if (TARGET spdlog::spdlog)
        target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
    elseif(TARGET spdlog)
        target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)
    endif()
else()
    # Use local shim headers (external/spdlog/include)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/spdlog/include)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

# Place binary in project root for convenience
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

message(STATUS "cppAuth: sources=${CPPAUTH_SOURCES}")

