add_library(segcore STATIC
    src/artifact.cpp
    src/history.cpp
    src/geometry.cpp
    src/normalized_format_serializer.cpp
    src/display.cpp
    src/annotation_set.cpp
    src/project.cpp
)

target_include_directories(segcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(segcore PUBLIC imgproc)
set_target_properties(segcore PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)
