cmake_minimum_required(VERSION 3.21)

project(lon_stack_dx
    VERSION 4.1.1
    DESCRIPTION "LON Stack DX for LON/IP and U10/U60 LON USB interfaces"
    LANGUAGES C
)

# For cross-compilation: explicitly set lib dir to avoid architecture detection warning
if(NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Library installation directory")
endif()

# Allow this project to be used with FetchContent or as a top-level build
# enable_language(C)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

# Define the source of the STM32CubeG4 MCU Package
FetchContent_Declare(
  stm32cubeg4
  GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeG4.git
  GIT_TAG        v1.5.0 
)

# Only download and configure the SDK when compiling for the STM32
if(PROCESSOR_ID STREQUAL "PROCESSOR_ID_STM32")
    FetchContent_MakeAvailable(stm32cubeg4)

    # Create a variable to hold all the necessary STM32 include paths
    set(STM32_SDK_INCLUDES
        "${stm32cubeg4_SOURCE_DIR}/Drivers/STM32G4xx_HAL_Driver/Inc"
        "${stm32cubeg4_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32G4xx/Include"
        "${stm32cubeg4_SOURCE_DIR}/Drivers/CMSIS/Include"
        "${stm32cubeg4_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Core/Inc"
        "${stm32cubeg4_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc"
        "${stm32cubeg4_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include"
    )

    # Append the required SDK source files to your existing sources list
    list(APPEND LON_STACK_DX_SOURCES
        "${stm32cubeg4_SOURCE_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c"
        "${stm32cubeg4_SOURCE_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c"
        "${stm32cubeg4_SOURCE_DIR}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c"
        "${stm32cubeg4_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c"
        "${stm32cubeg4_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Src/usbh_cdc.c"
        # Add the specific FreeRTOS .c files (tasks.c, queue.c, port.c) here as well
    )
endif()

# Generate version header from the CMake project() VERSION
set(LON_STACK_DX_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(LON_STACK_DX_GENERATED_VERSION_HEADER "${LON_STACK_DX_GENERATED_INCLUDE_DIR}/common/lon_stack_dx_version.h")

math(EXPR LON_STACK_DX_VERSION_MINOR1 "${PROJECT_VERSION_MINOR} / 10")
math(EXPR LON_STACK_DX_VERSION_MINOR2 "${PROJECT_VERSION_MINOR} % 10")
math(EXPR LON_STACK_DX_VERSION_BUILD0 "${PROJECT_VERSION_PATCH} / 100")
math(EXPR LON_STACK_DX_VERSION_BUILD1 "(${PROJECT_VERSION_PATCH} / 10) % 10")
math(EXPR LON_STACK_DX_VERSION_BUILD2 "${PROJECT_VERSION_PATCH} % 10")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lon_stack_dx_version.h.in"
    "${LON_STACK_DX_GENERATED_VERSION_HEADER}"
    @ONLY
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Set build options
set(LON_STACK_EXPORT ON CACHE BOOL "Use LonStack as a library instead of a sub-project")
set(LON_STACK_BUILD_EXAMPLE ON CACHE BOOL "Build example executable")
set(ISI_ID "ISI_ID_NO_ISI" CACHE STRING "ISI implementation identifier")
set(IUP_ID "IUP_ID_NO_IUP" CACHE STRING "IUP implementation identifier")
set(LINK_ID "LINK_ID_USB_MIP" CACHE STRING "Data link identifier")
set(OS_ID "OS_ID_LINUX" CACHE STRING "Operating system identifier")
set(PHYSICAL_ID "PHYSICAL_ID_LON_FT" CACHE STRING "Physical layer identifier")
set(PLATFORM_ID "PLATFORM_ID_LINUX64_ARM_GCC" CACHE STRING "Platform identifier")
set(PROCESSOR_ID "PROCESSOR_ID_ARM64" CACHE STRING "Processor architecture identifier")
set(PRODUCT_ID "PRODUCT_ID_LON_STACK_DX" CACHE STRING "Product identifier")
set(PROTOCOL_ID "PROTOCOL_ID_LON_NATIVE_V0" CACHE STRING "LON protocol identifier")
set(SECURITY_ID "SECURITY_ID_V1" CACHE STRING "Security implementation identifier")
set(USB_SERVICE_ID "USB_SERVICE_ID_NA" CACHE STRING "USB service type identifier")
set(USB_UPLINK_ID "USB_UPLINK_ID_POLLING" CACHE STRING "USB uplink type identifier")
set(INITIAL_LOG_CATEGORIES "LOG_PACKET_TRACE" CACHE STRING "Initial log categories (bitfield)")
set(LON_DEV_NAME "LON1" CACHE STRING "Device name for USB LON interface (Linux)")
set(LON_USB_IFACE_TYPE "LON_USB_INTERFACE_U50" CACHE STRING "Interface type for USB LON interface (U10 or U60 links)")
set(MAX_IFACE_STATES 1 CACHE STRING "Maximum interface states")
set(NUM_LON_NI 1 CACHE STRING "Number of LON network interfaces")
set(USB_DEV_NAME "/dev/ttyACM0" CACHE STRING "Device name for USB LON interface (Linux)")
set(USB_LINE_DISCIPLINE 28 CACHE STRING "Line discipline for USB LON interface (Linux)")

# Define sources and headers
set(LON_STACK_DX_SOURCES
    IzotApi.c
    abstraction/IzotHal.c
    abstraction/IzotOsal.c
    lcs/lcs_app.c
    lcs/lcs_custom.c
    lcs/lcs_eeprom.c
    lcs/lcs_link.c
    lcs/lcs_netmgmt.c
    lcs/lcs_network.c
    lcs/lcs_node.c
    lcs/lcs_proxy.c
    lcs/lcs_queue.c
    lcs/lcs_tcs.c
    lcs/lcs_timer.c
    lcs/lcs_tsa.c
    lcs/lcs.c
    persistence/storage_persistence.c
    persistence/lon_persistence.c
)

set(LON_STACK_DX_HEADERS
    include/abstraction/IzotConfig.h
    include/abstraction/IzotEndian.h
    include/abstraction/IzotHal.h
    include/abstraction/IzotOsal.h
    include/common/bitfield.h
    include/izot/iap_types.h
    include/izot/IzotApi.h
    include/izot/IzotIsiApi.h
    include/izot/IzotIsiTypes.h
    include/izot/IzotPlatform.h
    include/izot/lon_types.h
    include/lcs/lcs_api.h
    include/lcs/lcs_app.h
    include/lcs/lcs_custom.h
    include/lcs/lcs_eia709_1.h
    include/lcs/lcs_link.h
    include/lcs/lcs_netmgmt.h
    include/lcs/lcs_network.h
    include/lcs/lcs_node.h
    include/lcs/lcs_platform.h
    include/lcs/lcs_proxy.h
    include/lcs/lcs_queue.h
    include/lcs/lcs_tcs.h
    include/lcs/lcs_timer.h
    include/lcs/lcs_tsa.h
    include/lcs/lcs.h
    include/persistence/storage_persistence.h
    include/persistence/lon_persistence.h
)

if(LINK_ID STREQUAL "LINK_ID_USB_MIP")
    list(APPEND LON_STACK_DX_SOURCES "lon_usb/lon_usb_link.c")
    list(APPEND LON_STACK_DX_HEADERS "include/lon_usb/lon_usb_link.h")
elseif(LINK_ID STREQUAL "LINK_ID_SPI_MIP")
    list(APPEND LON_STACK_DX_HEADERS "include/lcs/lcs_physical.h")
elseif(LINK_ID STREQUAL "LINK_ID_UDP")
    list(APPEND LON_STACK_DX_SOURCES "abstraction/IzotCal.c")
    list(APPEND LON_STACK_DX_HEADERS "include/abstraction/IzotCal.h")
    list(APPEND LON_STACK_DX_SOURCES "lon_udp/lon_udp_mapping_util.c")
endif()

if(PROTOCOL_ID STREQUAL "PROTOCOL_ID_LON_IPV4" OR PROTOCOL_ID STREQUAL "PROTOCOL_ID_LON_IPV6")
    list(APPEND LON_STACK_DX_SOURCES "lon_udp/ipv4_to_lon_udp.c")
    list(APPEND LON_STACK_DX_HEADERS "include/lon_udp/ipv4_to_lon_udp.h")
endif()

if (IUP_ID STREQUAL "IUP_ID_V1")
    list(APPEND LON_STACK_DX_SOURCES "lcs/lcs_iup.c")
    list(APPEND LON_STACK_DX_HEADERS "include/lcs/lcs_iup.h")
endif()

if (NOT ISI_ID STREQUAL "ISI_ID_NO_ISI")
    list(APPEND LON_STACK_DX_SOURCES "isi/isi_persistence.c")
    list(APPEND LON_STACK_DX_SOURCES "isi/isi_util.c")
    list(APPEND LON_STACK_DX_SOURCES "isi/isi_vars.c")
    list(APPEND LON_STACK_DX_HEADERS "include/isi/isi_int.h")
endif()

# Create the library target
add_library(lon_stack_dx STATIC
    ${LON_STACK_DX_SOURCES}
    ${LON_STACK_DX_HEADERS}
)

target_include_directories(lon_stack_dx PUBLIC
    $<BUILD_INTERFACE:${LON_STACK_DX_GENERATED_INCLUDE_DIR}> 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    ${STM32_SDK_INCLUDES}
)

target_link_libraries(lon_stack_dx PUBLIC Threads::Threads)

# Set build defaults at compile time
target_compile_definitions(lon_stack_dx
    PRIVATE
    ISI_ID=${ISI_ID}
    IUP_ID=${IUP_ID}
    LINK_ID=${LINK_ID}
    OS_ID=${OS_ID}
    PHYSICAL_ID=${PHYSICAL_ID}
    PLATFORM_ID=${PLATFORM_ID}
    PROCESSOR_ID=${PROCESSOR_ID}
    PRODUCT_ID=${PRODUCT_ID}
    PROTOCOL_ID=${PROTOCOL_ID}
    SECURITY_ID=${SECURITY_ID}
    USB_SERVICE_ID=${USB_SERVICE_ID}
    USB_UPLINK_ID=${USB_UPLINK_ID}
    INITIAL_LOG_CATEGORIES=${INITIAL_LOG_CATEGORIES}
    LON_DEV_NAME="${LON_DEV_NAME}"
    LON_USB_IFACE_TYPE=${LON_USB_IFACE_TYPE}
    MAX_IFACE_STATES=${MAX_IFACE_STATES}
    NUM_LON_NI=${NUM_LON_NI}
    USB_DEV_NAME="${USB_DEV_NAME}"
    USB_LINE_DISCIPLINE=${USB_LINE_DISCIPLINE}
)

# LON Stack DX is a C-only library, so comment out C++ features
#target_compile_features(lon_stack_dx PUBLIC cxx_std_17)

# Create namespaced alias target for linking
add_library(lon_stack_dx::lon_stack_dx ALIAS lon_stack_dx)

if(LON_STACK_EXPORT)
    # Installation rules
    install(TARGETS lon_stack_dx
        EXPORT lon_stack_dxTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(FILES
        "${LON_STACK_DX_GENERATED_VERSION_HEADER}"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common
    )

    install(EXPORT lon_stack_dxTargets
        FILE lon_stack_dxTargets.cmake
        NAMESPACE lon_stack_dx::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lon_stack_dx
    )

    # Make this available for find_package()
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/lon_stack_dxConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lon_stack_dxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lon_stack_dxConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lon_stack_dx
    )

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lon_stack_dxConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/lon_stack_dxConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lon_stack_dx
    )
endif() # end of LON_STACK_EXPORT

if(LON_STACK_BUILD_EXAMPLE)
    # Example executable (optional): builds example/LonStackExample1.c with main()
    add_executable(lon_stack_example1
        example/LonStackExample1.c
    )
    target_link_libraries(lon_stack_example1 PRIVATE lon_stack_dx)
    target_compile_definitions(lon_stack_example1 PRIVATE INCLUDE_EXAMPLE_MAIN)
    # Ensure the example's own headers are visible
    target_include_directories(lon_stack_example1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/example)
endif()