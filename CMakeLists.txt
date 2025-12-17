cmake_minimum_required(VERSION 3.21)

project(lon_stack_dx
    VERSION 2.0.0
    DESCRIPTION "LON Stack DX for LON/IP and U10/U60 LON FT-compatible USB interfaces"
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

# Define sources and headers
set(LON_STACK_DX_SOURCES
    IzotApi.c
    abstraction/IzotCal.c
    abstraction/IzotHal.c
    abstraction/IzotOsal.c
    isi/isi_persistence.c
    isi/isi_util.c
    isi/isi_vars.c
    lcs/lcs_app.c
    lcs/lcs_custom.c
    lcs/lcs_eeprom.c
    lcs/lcs_iup.c
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
    lon_udp/ipv4_to_lon_udp.c
    lon_udp/lon_udp_mapping_util.c
    lon_usb/lon_usb_link.c
    persistence/flash_persistence.c
    persistence/lon_persistence.c
)

set(LON_STACK_DX_HEADERS
    include/abstraction/IzotCal.h
    include/abstraction/IzotConfig.h
    include/abstraction/IzotEndian.h
    include/abstraction/IzotHal.h
    include/abstraction/IzotOsal.h
    include/abstraction/vldv.h
    include/common/bitfield.h
    include/common/modnvlen.h
    include/isi/isi_int.h
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
    include/lcs/lcs_iup.h
    include/lcs/lcs_link.h
    include/lcs/lcs_netmgmt.h
    include/lcs/lcs_network.h
    include/lcs/lcs_node.h
    include/lcs/lcs_physical.h
    include/lcs/lcs_platform.h
    include/lcs/lcs_proxy.h
    include/lcs/lcs_queue.h
    include/lcs/lcs_tcs.h
    include/lcs/lcs_timer.h
    include/lcs/lcs_tsa.h
    include/lcs/lcs.h
    include/lon_udp/ipv4_to_lon_udp.h
    include/persistence/flash_persistence.h
    include/persistence/lon_persistence.h
)

# Create the library target
add_library(lon_stack_dx STATIC
    ${LON_STACK_DX_SOURCES}
    ${LON_STACK_DX_HEADERS}
)

target_include_directories(lon_stack_dx
    PUBLIC
    $<BUILD_INTERFACE:${LON_STACK_DX_GENERATED_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(lon_stack_dx PUBLIC Threads::Threads)

# Set build defaults at compile time
target_compile_definitions(lon_stack_dx
    PRIVATE
    OS_ID=OS_ID_LINUX
    LINK_ID=LINK_ID_USB
    PROTOCOL_ID=PROTOCOL_ID_LON_NATIVE
    ISI_ID=ISI_ID_NO_ISI
)

# LON Stack DX is a C-only library, so comment out C++ features
#target_compile_features(lon_stack_dx PUBLIC cxx_std_17)

# Create namespaced alias target for modern linking
add_library(lon_stack_dx::lon_stack_dx ALIAS lon_stack_dx)

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

# Example executable (optional): builds example/LonStackExample1.c with main()
add_executable(lon_stack_example1
    example/LonStackExample1.c
)
target_link_libraries(lon_stack_example1 PRIVATE lon_stack_dx)
target_compile_definitions(lon_stack_example1 PRIVATE INCLUDE_EXAMPLE_MAIN)
# Ensure the example's own headers are visible
target_include_directories(lon_stack_example1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/example)