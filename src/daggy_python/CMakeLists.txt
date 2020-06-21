cmake_minimum_required(VERSION 3.16.4)

project(daggy_python)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(TARGET daggy_python)

set(wrapped_header ${CMAKE_CURRENT_SOURCE_DIR}/bindings.h)
set(typesystem_file ${CMAKE_CURRENT_SOURCE_DIR}/bindings.xml)

set(generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/command_wrapper.cpp)

if(NOT python_interpreter)
    find_program(python_interpreter "python")
endif()
message(STATUS "Using python interpreter: ${python_interpreter}")

# Macro to get various pyside / python include / link flags and paths.
# Uses the not entirely supported utils/pyside2_config.py file.
macro(pyside2_config option output_var)
    if(${ARGC} GREATER 2)
        set(is_list ${ARGV2})
    else()
        set(is_list "")
    endif()

    execute_process(
        COMMAND ${python_interpreter} "${CMAKE_CURRENT_SOURCE_DIR}/pyside2_config.py"
        ${option}
        OUTPUT_VARIABLE ${output_var}
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if ("${${output_var}}" STREQUAL "")
        message(FATAL_ERROR "Error: Calling pyside2_config.py ${option} returned no output.")
    endif()
    if(is_list)
        string (REPLACE " " ";" ${output_var} "${${output_var}}")
    endif()
endmacro()

# Query for the shiboken generator path, Python path, include paths and linker flags.
pyside2_config(--shiboken2-module-path shiboken2_module_path)
pyside2_config(--shiboken2-generator-path shiboken2_generator_path)
pyside2_config(--python-include-path python_include_dir)
pyside2_config(--shiboken2-generator-include-path shiboken_include_dir 1)
pyside2_config(--shiboken2-module-shared-libraries-cmake shiboken_shared_libraries 0)
pyside2_config(--python-link-flags-cmake python_linking_data 0)
pyside2_config(--pyside2-path pyside2_path)

set(shiboken_path "${shiboken2_generator_path}/shiboken2${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT EXISTS ${shiboken_path})
    message(FATAL_ERROR "Shiboken executable not found at path: ${shiboken_path}")
else()
    message("Shiboken executable path: ${shiboken_path}")
endif()

link_directories(${pyside2_qt_libs})
set(shiboken_options --generator-set=shiboken --enable-parent-ctor-heuristic
    --enable-return-value-heuristic --use-isnull-as-nb_nonzero
    --avoid-protected-hack
    --enable-pyside-extensions
    --language-level=c++17
    --include-paths=${CMAKE_SOURCE_DIR}/src/:${CMAKE_BINARY_DIR}/exports/:${CONAN_INCLUDE_DIRS_QT}:${CONAN_INCLUDE_DIRS_QT}/QtCore
    --typesystem-paths=${pyside2_path}/typesystems
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
    )

message("shiboken_path ${shiboken_path}")

set(generated_sources_dependencies ${wrapped_header} ${typesystem_file})
add_custom_command(OUTPUT ${generated_sources}
    COMMAND ${shiboken_path}
    ${shiboken_options} ${wrapped_header} ${typesystem_file}
    DEPENDS ${generated_sources_dependencies}
    IMPLICIT_DEPENDS CXX ${wrapped_header}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running generator for ${typesystem_file}.")

set(${TARGET}_sources ${generated_sources})

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_RPATH ${shiboken2_module_path} ${pyside2_qt_libs})

add_library(${TARGET} MODULE ${${TARGET}_sources})

target_link_libraries(${TARGET} PRIVATE ${shiboken_shared_libraries})
target_link_libraries(${TARGET} PRIVATE DaggyCore)

if (MSVC OR MINGW)
    set( PYTHON_EXTENSION_SUFFIX ".pyd" )
else (MSVC OR MINGW)
    if (CYGWIN)
        set( PYTHON_EXTENSION_SUFFIX ".dll" )
    else (CYGWIN)
        set( PYTHON_EXTENSION_SUFFIX ".so" )
    endif (CYGWIN)
endif (MSVC OR MINGW)

# Adjust the name of generated module.
set_property(TARGET ${TARGET} PROPERTY PREFIX "")
set_property(TARGET ${TARGET} PROPERTY OUTPUT_NAME
    "${TARGET}${PYTHON_EXTENSION_SUFFIX}")

# Make sure the linker doesn't complain about not finding Python symbols on macOS.
if(APPLE)
    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

# Find and link to the python import library only on Windows.
# On Linux and macOS, the undefined symbols will get resolved by the dynamic linker
# (the symbols will be picked up in the Python executable).
if (WIN32)
    list(GET python_linking_data 0 python_libdir)
    list(GET python_linking_data 1 python_lib)
    find_library(python_link_flags ${python_lib} PATHS ${python_libdir} HINTS ${python_libdir})
    target_link_libraries(${TARGET} PRIVATE ${python_link_flags})
endif()
