include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

set(INCLUDE_INSTALL_DIR include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})


set(CMAKE_BUILD_WITH_INSTALL_RPATH true)

if(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR};$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/daggy_deps;$ORIGIN/daggy_deps")
endif()
