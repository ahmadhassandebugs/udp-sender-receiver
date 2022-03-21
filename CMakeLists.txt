cmake_minimum_required(VERSION 3.10)
project("udp-client-server" VERSION 1.0.0
        DESCRIPTION "A mashup of linux networking utilities")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# set flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# create variable for common sources
set(sources timestamp.cpp packet.cpp)

# add executables
add_executable(client ${sources} client.cpp)
add_executable(server ${sources} server.cpp)
