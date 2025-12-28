cmake_minimum_required(VERSION 3.10)
project(p25-hotspot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${YAML_CPP_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/Config.cpp
    src/Logger.cpp
    src/ModemSerial.cpp
    src/P25Protocol.cpp
    src/NetworkClient.cpp
    src/TrunkingController.cpp
)

# Executable
add_executable(p25-hotspot ${SOURCES})

# Link libraries
target_link_libraries(p25-hotspot
    ${CMAKE_THREAD_LIBS_INIT}
    ${YAML_CPP_LIBRARIES}
)

# Install
install(TARGETS p25-hotspot DESTINATION /usr/local/bin)
install(FILES config.example.yaml DESTINATION /etc RENAME p25-hotspot.yaml.example)
