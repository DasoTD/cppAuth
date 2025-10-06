cmake_minimum_required(VERSION 3.16)
project(cppAuth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ======================================================
# Dependencies
# ======================================================

# Drogon
find_package(Drogon CONFIG REQUIRED)

# PostgreSQL
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

# dotenv-cpp (header-only)
include_directories(${CMAKE_SOURCE_DIR}/dotenv-cpp/include)

# bcrypt (local header-only or compiled)
include_directories(${CMAKE_SOURCE_DIR}/external/Bcrypt.cpp/include)
#include_directories(dotenv-cpp/include)


# jwt-cpp (header-only)
include_directories(${CMAKE_SOURCE_DIR}/external/jwt-cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/include)


# JsonCpp (required by Drogon)
set(JSONCPP_INCLUDE_DIRS "/usr/include/jsoncpp")
set(JSONCPP_LIBRARIES "/usr/lib/x86_64-linux-gnu/libjsoncpp.so")


# ======================================================
# Source files (exclude test files)
# ======================================================
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/*.cc"
    "${CMAKE_SOURCE_DIR}/controllers/*.cc"
)
# Remove test files from SRC_FILES
list(FILTER SRC_FILES EXCLUDE REGEX ".*/test/.*")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/tests/.*")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/base_test.cc$")

# Add bcrypt implementation if needed
list(APPEND SRC_FILES "${CMAKE_SOURCE_DIR}/external/Bcrypt.cpp/src/bcrypt.cpp")
list(APPEND SRC_FILES "${CMAKE_SOURCE_DIR}/external/Bcrypt.cpp/src/blowfish.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})

# ======================================================
# Link libraries
# ======================================================
target_link_libraries(${PROJECT_NAME}
    PRIVATE Drogon::Drogon
    PRIVATE ${PostgreSQL_LIBRARIES}
    PRIVATE ${JSONCPP_LIBRARIES}
)

# ======================================================
# Include directories
# ======================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${JSONCPP_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/controllers
    ${CMAKE_SOURCE_DIR}/external/dotenv-cpp/include
    ${CMAKE_SOURCE_DIR}/external/bcrypt/include
    ${CMAKE_SOURCE_DIR}/external/jwt-cpp/include
    ${CMAKE_SOURCE_DIR}/external/dotenv-cpp/include
)

target_include_directories(cppAuth PRIVATE /path/to/Bcrypt.cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/dotenv-cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/external/jwt-cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/external/Bcrypt.cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/external/jwt-cpp/include)

# ======================================================
# Output binary in project root (optional)
# ======================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
