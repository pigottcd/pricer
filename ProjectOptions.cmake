include(cmake/SystemLink.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(pricer_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(pricer_setup_options)
  option(pricer_ENABLE_HARDENING "Enable hardening" ON)
  option(pricer_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    pricer_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    pricer_ENABLE_HARDENING
    OFF)

  pricer_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR pricer_PACKAGING_MAINTAINER_MODE)
    option(pricer_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(pricer_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(pricer_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(pricer_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(pricer_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(pricer_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(pricer_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(pricer_ENABLE_PCH "Enable precompiled headers" OFF)
    option(pricer_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(pricer_ENABLE_IPO "Enable IPO/LTO" ON)
    option(pricer_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(pricer_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(pricer_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(pricer_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(pricer_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(pricer_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(pricer_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(pricer_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(pricer_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(pricer_ENABLE_PCH "Enable precompiled headers" OFF)
    option(pricer_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      pricer_ENABLE_IPO
      pricer_WARNINGS_AS_ERRORS
      pricer_ENABLE_USER_LINKER
      pricer_ENABLE_SANITIZER_ADDRESS
      pricer_ENABLE_SANITIZER_LEAK
      pricer_ENABLE_SANITIZER_UNDEFINED
      pricer_ENABLE_SANITIZER_THREAD
      pricer_ENABLE_SANITIZER_MEMORY
      pricer_ENABLE_UNITY_BUILD
      pricer_ENABLE_CLANG_TIDY
      pricer_ENABLE_CPPCHECK
      pricer_ENABLE_COVERAGE
      pricer_ENABLE_PCH
      pricer_ENABLE_CACHE)
  endif()

endmacro()

macro(pricer_global_options)
  if(pricer_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    pricer_enable_ipo()
  endif()

  pricer_supports_sanitizers()

  if(pricer_ENABLE_HARDENING AND pricer_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR pricer_ENABLE_SANITIZER_UNDEFINED
       OR pricer_ENABLE_SANITIZER_ADDRESS
       OR pricer_ENABLE_SANITIZER_THREAD
       OR pricer_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${pricer_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${pricer_ENABLE_SANITIZER_UNDEFINED}")
    pricer_enable_hardening(pricer_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(pricer_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(pricer_warnings INTERFACE)
  add_library(pricer_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  pricer_set_project_warnings(
    pricer_warnings
    ${pricer_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(pricer_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    pricer_configure_linker(pricer_options)
  endif()

  include(cmake/Sanitizers.cmake)
  pricer_enable_sanitizers(
    pricer_options
    ${pricer_ENABLE_SANITIZER_ADDRESS}
    ${pricer_ENABLE_SANITIZER_LEAK}
    ${pricer_ENABLE_SANITIZER_UNDEFINED}
    ${pricer_ENABLE_SANITIZER_THREAD}
    ${pricer_ENABLE_SANITIZER_MEMORY})

  set_target_properties(pricer_options PROPERTIES UNITY_BUILD ${pricer_ENABLE_UNITY_BUILD})

  if(pricer_ENABLE_PCH)
    target_precompile_headers(
      pricer_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(pricer_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    pricer_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(pricer_ENABLE_CLANG_TIDY)
    pricer_enable_clang_tidy(pricer_options ${pricer_WARNINGS_AS_ERRORS})
  endif()

  if(pricer_ENABLE_CPPCHECK)
    pricer_enable_cppcheck(${pricer_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(pricer_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    pricer_enable_coverage(pricer_options)
  endif()

  if(pricer_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(pricer_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(pricer_ENABLE_HARDENING AND NOT pricer_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR pricer_ENABLE_SANITIZER_UNDEFINED
       OR pricer_ENABLE_SANITIZER_ADDRESS
       OR pricer_ENABLE_SANITIZER_THREAD
       OR pricer_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    pricer_enable_hardening(pricer_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
