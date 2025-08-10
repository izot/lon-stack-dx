cmake_minimum_required(VERSION 3.21)

project(lon_stack_dx
    VERSION 1.0.0
    DESCRIPTION "LON Stack DX for U10/U60 LON FT-compatible USB interfaces"
    LANGUAGES C CXX
)

# Allow this project to be used with FetchContent or as a top-level build
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Define sources and headers
set(LON_STACK_DX_SOURCES
    abstraction/IzotCal.c
    abstraction/IzotHal.c
    abstraction/IzotOsal.c
    isi/Persistence.c
    isi/util.c
    isi/vars.c
    lcs/iup.c
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
    ls_udp/IPv4ToLsUdp.c
    ls_udp/LsMappingUtil.c
    persistence/IzotPersistentFlashDirect.c
    persistence/Persistent.c
)

set(LON_STACK_DX_HEADERS
    include/abstraction/IzotCal.h
    include/abstraction/IzotConfig.h
    include/abstraction/IzotEndian.h
    include/abstraction/IzotHal.h
    include/abstraction/IzotOsal.h
    include/abstraction/vldv.h
    include/common/bitfield.h
    include/common/EchelonStandardDefinitions.h
    include/common/echstd.h
    include/common/echversion.h
    include/common/err.h
    include/common/modnvlen.h
    include/common/module_platform.h
    include/common/vermacro.h
    include/isi/isi_int.h
    include/izot/IapTypes.h
    include/izot/IzotApi.h
    include/izot/IzotIsiApi.h
    include/izot/IzotIsiTypes.h
    include/izot/IzotPlatform.h
    include/izot/IzotTypes.h
    include/lcs/iup.h
    include/lcs/lcs_api.h
    include/lcs/lcs_app.h
    include/lcs/lcs_custom.h
    include/lcs/lcs_eia709_1.h
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
    include/ls_udp/IPv4ToLsUdp.h
    include/persistence/IzotPersistentFlashDirect.h
    include/persistence/Persistent.h
)

# Create the library target
add_library(lon_stack_dx STATIC
    ${LON_STACK_DX_SOURCES}
    ${LON_STACK_DX_HEADERS}
)

target_include_directories(lon_stack_dx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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