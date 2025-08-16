# aarch64-toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
#set(CMAKE_C_COMPILER /opt/homebrew/bin/aarch64-linux-gnu-gcc)
set(CMAKE_C_COMPILER /opt/homebrew/bin/aarch64-linux-gnu-gcc CACHE FILEPATH "" FORCE)
set(CMAKE_C_COMPILER_FORCED TRUE CACHE BOOL "" FORCE)
set(CMAKE_CXX_COMPILER /opt/homebrew/bin/aarch64-linux-gnu-g++)

# Set the sysroot for correct target headers and libraries
set(CMAKE_SYSROOT /opt/homebrew/opt/aarch64-unknown-linux-gnu/toolchain/aarch64-unknown-linux-gnu/sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Remove Mac-specific -arch arm64 from all relevant flag variables
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")
