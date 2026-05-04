# Unit tests for PolySeg

set(POLYSEG_TEST_SOURCES
    projectconfig_test.cpp
    metadataimporter_test.cpp
)

add_executable(PolySeg_tests ${POLYSEG_TEST_SOURCES})

target_link_libraries(PolySeg_tests PRIVATE
    gtest_main
    PolySeg_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(PolySeg_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/googletest/googletest/include
)

# Apply coverage flags if enabled
if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    target_compile_options(PolySeg_tests PRIVATE ${COVERAGE_FLAGS})
    target_link_libraries(PolySeg_tests PRIVATE ${COVERAGE_FLAGS})
endif()

add_test(NAME PolySeg_unit_tests COMMAND PolySeg_tests)

add_executable(segcore_tests
    segcore/history_test.cpp
    segcore/types_test.cpp
    segcore/geometry_test.cpp
    segcore/normalized_format_serializer_test.cpp
    segcore/display_test.cpp
    segcore/annotation_set_test.cpp
    segcore/project_test.cpp
)

target_link_libraries(segcore_tests PRIVATE gtest_main segcore)

target_include_directories(segcore_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/external/googletest/googletest/include
)

add_test(NAME segcore_unit_tests COMMAND segcore_tests)

find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(polygoncanvas_tests
    polygoncanvas_test.cpp
)

target_link_libraries(polygoncanvas_tests PRIVATE
    gtest
    gmock
    PolySeg_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

target_include_directories(polygoncanvas_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/googletest/googletest/include
    ${CMAKE_SOURCE_DIR}/external/googletest/googlemock/include
)

add_test(NAME polygoncanvas_unit_tests COMMAND polygoncanvas_tests)

set_tests_properties(polygoncanvas_unit_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
