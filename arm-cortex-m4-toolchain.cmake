#arm-cortex-m4-toolchain.cmake

# Target operating system and specific processor architecture
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)

# Specify the cross-compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Force CMake to skip trying to compile a test executable 
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Remove Mac-specific -arch arm64 from all relevant flag variables
# (Prevents "unrecognized command-line option" errors in the ARM GCC compiler)
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "-arch[ ]*arm64" "" CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}")

# Architecture specific compiler flags for the Cortex-M4
set(OBJECT_GEN_FLAGS "-mcpu=cortex-m4 -mthumb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OBJECT_GEN_FLAGS}" CACHE INTERNAL "C compiler flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OBJECT_GEN_FLAGS}" CACHE INTERNAL "CXX compiler flags")