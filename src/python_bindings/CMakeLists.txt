include(GenerateExportHeader)


add_library(pypricer MODULE bindings.cpp)

add_library(pricer::pypyricer ALIAS pypricer)

# Make sure there is no "lib" prefix and the name matches the Python module
set_target_properties(pypricer PROPERTIES PREFIX "")

set_target_properties(pypricer PROPERTIES OUTPUT_NAME "pypricer")

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

target_link_libraries(pypricer
    PRIVATE pybind11::module
    PRIVATE Python3::Python
    PRIVATE sample_library
    PRIVATE pricer_options
    PRIVATE pricer_warnings)

target_include_directories(pypricer ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                                            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(sample_library PUBLIC cxx_std_20)

set_target_properties(
  pypricer
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)
