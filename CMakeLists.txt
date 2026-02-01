cmake_minimum_required(VERSION 3.20)

project(PolySeg
    VERSION 1.0
    DESCRIPTION "Secure offline polygon annotation tool for instance segmentation datasets"
    LANGUAGES CXX
)

# Set C++17 standard (matching CONFIG += c++17 strict_c++ from qmake)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6 components (matching QT += core gui network widgets)
# Set CMAKE_PREFIX_PATH to help find Qt6 in common locations
if(NOT Qt6_FOUND AND NOT CMAKE_PREFIX_PATH)
    # Try common Qt installation paths
    if(UNIX AND NOT APPLE)
        list(APPEND CMAKE_PREFIX_PATH
            "/usr/lib/cmake"
            "/usr/lib64/cmake"
            "/opt/qt6"
            "$ENV{HOME}/Qt/6.8.3/gcc_64"
            "$ENV{HOME}/Qt/*/gcc_64"
        )
    elseif(APPLE)
        list(APPEND CMAKE_PREFIX_PATH
            "/opt/homebrew/lib/cmake"
            "/usr/local/lib/cmake"
            "$ENV{HOME}/Qt/6.8.3/clang_64"
        )
    elseif(WIN32)
        list(APPEND CMAKE_PREFIX_PATH
            "$ENV{HOME}/Qt/6.8.3/msvc2019_64"
            "$ENV{HOME}/Qt/6.8.3/mingw_64"
            "C:/Qt/6.8.3/msvc2019_64"
            "C:/Qt/6.8.3/mingw_64"
        )
    endif()
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets Network)

if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt6 not found. Please install Qt6 or set CMAKE_PREFIX_PATH to your Qt6 installation.\n"
        "For example:\n"
        "  cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64 ..\n"
        "or set the Qt6_DIR variable:\n"
        "  cmake -DQt6_DIR=/path/to/Qt/6.8.3/gcc_64/lib/cmake/Qt6 ..\n"
        "or install Qt6 system-wide:\n"
        "  sudo apt install qt6-base-dev qt6-tools-dev  # Ubuntu/Debian\n"
        "  brew install qt6  # macOS with Homebrew"
    )
endif()

# Qt6 policies (if needed, uncomment and adjust version-specific policies)
# qt_policy(SET QTP0001 NEW)

# Include directories (matching INCLUDEPATH += src)
include_directories(src)

# Collect all source files
set(POLYSEG_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/modelcomparisondialog.cpp
    src/modeldownloadmanager.cpp
    src/modelregistrationdialog.cpp
    src/polygoncanvas.cpp
    src/projectconfig.cpp
    src/pythonenvironmentmanager.cpp
    src/settingstabbase.cpp
    src/projectsettingstab.cpp
    src/aimodelsettingstab.cpp
    src/importexportsettingstab.cpp
    src/settingsdialog.cpp
    src/shortcuteditdialog.cpp
    src/shortcutssettingstab.cpp
    src/aipluginmanager.cpp
    src/pluginwizard.cpp
    src/metadataimporter.cpp
    src/metadataimportsettingsdialog.cpp
    src/wizardpages/welcomepage.cpp
    src/wizardpages/pluginselectionpage.cpp
    src/wizardpages/custompluginpage.cpp
    src/wizardpages/modelselectionpage.cpp
    src/wizardpages/modelselectionpage_detectron2.cpp
    src/wizardpages/modelselectionpage_smp.cpp
    src/wizardpages/pretrainedmodelpage.cpp
    src/wizardpages/pretrainedmodelpage_detectron2.cpp
    src/wizardpages/pretrainedmodelpage_smp.cpp
    src/wizardpages/downloadpage.cpp
    src/wizardpages/configurationpage.cpp
    src/wizardpages/customconfigurationpage.cpp
    src/wizardpages/summarypage.cpp
)

# Collect all headers
set(POLYSEG_HEADERS
    src/mainwindow.h
    src/modelcomparisondialog.h
    src/modeldownloadmanager.h
    src/modelregistrationdialog.h
    src/polygoncanvas.h
    src/projectconfig.h
    src/pythonenvironmentmanager.h
    src/settingstabbase.h
    src/projectsettingstab.h
    src/aimodelsettingstab.h
    src/importexportsettingstab.h
    src/settingsdialog.h
    src/shortcuteditdialog.h
    src/shortcutssettingstab.h
    src/aipluginmanager.h
    src/pluginwizard.h
    src/metadataimporter.h
    src/metadataimportsettingsdialog.h
    src/wizardpages/welcomepage.h
    src/wizardpages/pluginselectionpage.h
    src/wizardpages/custompluginpage.h
    src/wizardpages/modelselectionpage.h
    src/wizardpages/modelselectionpage_detectron2.h
    src/wizardpages/modelselectionpage_smp.h
    src/wizardpages/pretrainedmodelpage.h
    src/wizardpages/pretrainedmodelpage_detectron2.h
    src/wizardpages/pretrainedmodelpage_smp.h
    src/wizardpages/downloadpage.h
    src/wizardpages/configurationpage.h
    src/wizardpages/customconfigurationpage.h
    src/wizardpages/summarypage.h
)

# Collect all UI files
set(POLYSEG_UI_FILES
    src/mainwindow.ui
    src/settingsdialog.ui
    src/aimodelsettingstab.ui
    src/projectsettingstab.ui
    src/importexportsettingstab.ui
    src/shortcutssettingstab.ui
    src/shortcuteditdialog.ui
    src/modelcomparisondialog.ui
    src/modelregistrationdialog.ui
    src/metadataimportsettingsdialog.ui
    src/wizardpages/welcomepage.ui
    src/wizardpages/pluginselectionpage.ui
    src/wizardpages/custompluginpage.ui
    src/wizardpages/modelselectionpage.ui
    src/wizardpages/modelselectionpage_detectron2.ui
    src/wizardpages/modelselectionpage_smp.ui
    src/wizardpages/pretrainedmodelpage.ui
    src/wizardpages/pretrainedmodelpage_detectron2.ui
    src/wizardpages/pretrainedmodelpage_smp.ui
    src/wizardpages/downloadpage.ui
    src/wizardpages/configurationpage.ui
    src/wizardpages/customconfigurationpage.ui
    src/wizardpages/summarypage.ui
)

# Qt resource files
set(POLYSEG_RESOURCES
    resources/resources.qrc
)

# Enable Qt's automoc, autouic, and autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Create a library with core functionality (excluding main.cpp for testability)
set(POLYSEG_LIB_SOURCES ${POLYSEG_SOURCES})
list(REMOVE_ITEM POLYSEG_LIB_SOURCES src/main.cpp)

add_library(PolySeg_lib
    ${POLYSEG_LIB_SOURCES}
    ${POLYSEG_HEADERS}
    ${POLYSEG_UI_FILES}
    ${POLYSEG_RESOURCES}
)

# Link Qt libraries to the library
target_link_libraries(PolySeg_lib PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Create the main PolySeg executable
add_executable(PolySeg
    src/main.cpp
)

# Link the library to the executable
target_link_libraries(PolySeg PRIVATE
    PolySeg_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Compiler-specific warning flags (matching qmake configuration)
set(POLYSEG_COMPILE_OPTIONS "")
set(POLYSEG_COMPILE_DEFINITIONS "")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND POLYSEG_COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    # GCC-specific warnings (when not using Clang)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND POLYSEG_COMPILE_OPTIONS
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
        )
    endif()
elseif(MSVC)
    list(APPEND POLYSEG_COMPILE_OPTIONS
        /W4
        /permissive-
    )
endif()

# Configuration-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND POLYSEG_COMPILE_OPTIONS
            -g3
            -fno-omit-frame-pointer
        )
    elseif(MSVC)
        list(APPEND POLYSEG_COMPILE_OPTIONS /Zi)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND POLYSEG_COMPILE_OPTIONS -O2)
    elseif(MSVC)
        list(APPEND POLYSEG_COMPILE_OPTIONS /O2)
    endif()
    list(APPEND POLYSEG_COMPILE_DEFINITIONS NDEBUG)
endif()

# Apply compile options to both library and executable
target_compile_options(PolySeg_lib PRIVATE ${POLYSEG_COMPILE_OPTIONS})
target_compile_definitions(PolySeg_lib PRIVATE ${POLYSEG_COMPILE_DEFINITIONS})
target_compile_options(PolySeg PRIVATE ${POLYSEG_COMPILE_OPTIONS})
target_compile_definitions(PolySeg PRIVATE ${POLYSEG_COMPILE_DEFINITIONS})

# Code Coverage Configuration
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags
        set(COVERAGE_FLAGS --coverage -fprofile-arcs -ftest-coverage)
        target_compile_options(PolySeg_lib PRIVATE ${COVERAGE_FLAGS})
        target_link_libraries(PolySeg_lib PRIVATE ${COVERAGE_FLAGS})

        message(STATUS "Code coverage enabled")

        # Find gcov/llvm-cov
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            find_program(GCOV_PATH gcov)
            if(GCOV_PATH)
                message(STATUS "Found gcov: ${GCOV_PATH}")
            endif()
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            find_program(LLVM_COV_PATH llvm-cov)
            if(LLVM_COV_PATH)
                message(STATUS "Found llvm-cov: ${LLVM_COV_PATH}")
            endif()
        endif()
    else()
        message(WARNING "Code coverage only supported for GCC and Clang")
    endif()
endif()

# Enable testing and add Google Test
enable_testing()

# Add Google Test from external directory
add_subdirectory(external/googletest)

# Create unit test executable
add_executable(PolySeg_tests
    tests/main_test.cpp
    # Add additional test files here as you create them
)

# Link Google Test libraries and PolySeg library to test executable
target_link_libraries(PolySeg_tests PRIVATE
    gtest
    gtest_main
    PolySeg_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Apply coverage flags to test executable if enabled
if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    target_compile_options(PolySeg_tests PRIVATE ${COVERAGE_FLAGS})
    target_link_libraries(PolySeg_tests PRIVATE ${COVERAGE_FLAGS})
endif()

# Include test directories
target_include_directories(PolySeg_tests PRIVATE
    src
    external/googletest/googletest/include
)

# Add tests to CTest
add_test(NAME PolySeg_unit_tests COMMAND PolySeg_tests)

# Coverage target for easy usage
if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --zerocounters
            COMMAND ${LCOV_PATH} --directory . --capture --initial --output-file coverage_base.info --ignore-errors mismatch
            COMMAND $<TARGET_FILE:PolySeg_tests>
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_test.info --ignore-errors mismatch
            COMMAND ${LCOV_PATH} --add-tracefile coverage_base.info --add-tracefile coverage_test.info --output-file coverage.info
            COMMAND ${LCOV_PATH} --extract coverage.info '*src/*' --output-file coverage_src_only.info
            COMMAND ${GENHTML_PATH} coverage_src_only.info --output-directory coverage_html
            COMMAND echo "Coverage report generated in coverage_html/index.html"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS PolySeg_tests
            COMMENT "Generating code coverage report (src/ only)"
        )
    else()
        message(STATUS "lcov or genhtml not found, coverage target not available")
    endif()
endif()

# Installation rules (matching qmake deployment rules)
if(UNIX AND NOT ANDROID)
    install(TARGETS PolySeg
        RUNTIME DESTINATION bin
    )
endif()