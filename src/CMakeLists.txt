include(GenerateExportHeader)


file(GLOB_RECURSE SAMPLE_LIBRARY_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(sample_library ${SAMPLE_LIBRARY_SOURCES})

add_library(pricer::sample_library ALIAS sample_library)

target_link_libraries(sample_library PRIVATE pricer_options pricer_warnings)

target_include_directories(sample_library ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                                                  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                                                           PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)

target_compile_features(sample_library PUBLIC cxx_std_20)

target_compile_options(sample_library
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-mavx2>
)

set_target_properties(
  sample_library
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(sample_library EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/pricer/sample_library_export.hpp)

get_target_property(LIBRARY_TYPE sample_library TYPE)

if(LIBRARY_TYPE STREQUAL "STATIC_LIBRARY")
  target_compile_definitions(sample_library PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()
