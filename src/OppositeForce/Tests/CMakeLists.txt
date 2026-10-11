set(test_source_directory "${CMAKE_CURRENT_LIST_DIR}")
get_target_property(engine_source_directory openxcom SOURCE_DIR)
get_target_property(engine_sources openxcom SOURCES)
set(test_engine_sources)
foreach(source IN LISTS engine_sources)
    get_filename_component(absolute_source "${source}" ABSOLUTE BASE_DIR "${engine_source_directory}")
    if(absolute_source MATCHES "/main\\.cpp$|/apple/SDLMain\\.m$" OR NOT absolute_source MATCHES "\\.(c|cpp)$")
        continue()
    endif()
    list(APPEND test_engine_sources "${absolute_source}")
    foreach(property COMPILE_FLAGS COMPILE_OPTIONS COMPILE_DEFINITIONS)
        get_source_file_property(value "${absolute_source}" DIRECTORY "${engine_source_directory}" "${property}")
        if(NOT value STREQUAL "NOTFOUND")
            set_property(SOURCE "${absolute_source}" PROPERTY "${property}" "${value}")
        endif()
    endforeach()
endforeach()

add_library(opposite_force_test_engine STATIC ${test_engine_sources} "${test_source_directory}/EngineEntryPoint.cpp")
foreach(property INCLUDE_DIRECTORIES COMPILE_OPTIONS COMPILE_DEFINITIONS LINK_DIRECTORIES LINK_OPTIONS)
    get_target_property(value openxcom "${property}")
    if(value AND NOT value MATCHES "-NOTFOUND$")
        set_property(TARGET opposite_force_test_engine PROPERTY "${property}" "${value}")
    endif()
endforeach()
get_target_property(engine_libraries openxcom LINK_LIBRARIES)
# The runner uses Catch2's console entry point, not SDL's GUI startup shim.
list(FILTER engine_libraries EXCLUDE REGEX "(^|[/\\\\])(-l|lib)?SDLmain(\\.lib|\\.a)?$|^-mwindows$|^-Wl,--subsystem,windows$")
if(MINGW)
    # The engine lists these Win32 import libraries using MSVC's .lib spelling.
    list(TRANSFORM engine_libraries REPLACE "^(advapi32|shell32|shlwapi|wininet|urlmon)\\.lib$" "\\1")
endif()
target_link_libraries(opposite_force_test_engine PUBLIC ${engine_libraries})
get_target_property(engine_link_directories openxcom LINK_DIRECTORIES)
if(engine_link_directories)
    target_link_directories(opposite_force_test_engine PUBLIC ${engine_link_directories})
else()
    set(engine_link_directories)
endif()
# Directory definitions can be added by the engine after add_executable().
get_property(engine_definitions DIRECTORY "${engine_source_directory}" PROPERTY COMPILE_DEFINITIONS)
target_compile_definitions(opposite_force_test_engine PRIVATE ${engine_definitions})
target_include_directories(opposite_force_test_engine PUBLIC "${engine_source_directory}")
get_target_property(engine_includes openxcom INCLUDE_DIRECTORIES)
target_include_directories(opposite_force_test_engine PUBLIC ${engine_includes})

add_library(opposite_force_catch2 STATIC "${test_source_directory}/vendor/Catch2/catch_amalgamated.cpp")
target_include_directories(opposite_force_catch2 PUBLIC "${test_source_directory}/vendor/Catch2")

add_library(opposite_force_test_fixtures STATIC "${test_source_directory}/Fixtures.cpp")
target_link_libraries(opposite_force_test_fixtures PUBLIC opposite_force_test_engine)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "The test-only parser fixtures require Clang or GCC. On Windows, use clang-cl (Ninja or the Visual Studio ClangCL toolset), not cl.exe.")
endif()
set(fixture_access_flag -fno-access-control)
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(fixture_access_flag /clang:-fno-access-control)
endif()
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "${fixture_access_flag}")
check_cxx_source_compiles("class AccessProbe { static int getValue() { return 0; } }; int main() { return AccessProbe::getValue(); }" OPPOSITE_FORCE_FIXTURE_ACCESS_WORKS)
unset(CMAKE_REQUIRED_FLAGS)
if(NOT OPPOSITE_FORCE_FIXTURE_ACCESS_WORKS)
    message(FATAL_ERROR "The selected compiler cannot disable access checks for test fixtures using ${fixture_access_flag}.")
endif()
target_compile_options(opposite_force_test_fixtures PRIVATE "${fixture_access_flag}")

add_executable(opposite_force_tests
    "${test_source_directory}/TestEnvironment.cpp"
    "${test_source_directory}/HiddenUfoTests.cpp"
    "${test_source_directory}/HangarTests.cpp"
    "${test_source_directory}/FundingNamesTests.cpp"
    "${test_source_directory}/AvatarTests.cpp")
target_link_libraries(opposite_force_tests PRIVATE opposite_force_test_fixtures opposite_force_catch2)
set_target_properties(opposite_force_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# Include configured dependency directories in the runner's library search path.
set(test_runtime_directories ${engine_link_directories}
    ${PKG_SDL_LIBRARY_DIRS} ${PKG_ZLIB_LIBRARY_DIRS} ${PKG_SDLIMAGE_LIBRARY_DIRS}
    ${PKG_SDLGFX_LIBRARY_DIRS} ${PKG_SDLMIXER_LIBRARY_DIRS})
if(WIN32 AND DEPS_DIR)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND test_runtime_directories "${DEPS_DIR}/lib/x64")
    else()
        list(APPEND test_runtime_directories "${DEPS_DIR}/lib/Win32")
    endif()
    list(APPEND test_runtime_directories "${DEPS_DIR}/bin")
endif()
list(REMOVE_DUPLICATES test_runtime_directories)
list(JOIN test_runtime_directories "\n" test_runtime_paths)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/test-runner-$<CONFIG>.txt"
    CONTENT "$<TARGET_FILE:opposite_force_tests>\n${test_runtime_paths}\n")

# The existing formatter traverses all of OppositeForce, including ignored output.
file(WRITE "${CMAKE_BINARY_DIR}/.clang-format" "DisableFormat: true\n")
