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
target_link_libraries(opposite_force_test_engine PUBLIC ${engine_libraries})
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
    message(FATAL_ERROR "The test-only parser fixtures require Clang or GCC.")
endif()
target_compile_options(opposite_force_test_fixtures PRIVATE -fno-access-control)

add_executable(opposite_force_tests
    "${test_source_directory}/TestEnvironment.cpp"
    "${test_source_directory}/HiddenUfoTests.cpp"
    "${test_source_directory}/HangarTests.cpp"
    "${test_source_directory}/FundingNamesTests.cpp"
    "${test_source_directory}/AvatarTests.cpp")
target_link_libraries(opposite_force_tests PRIVATE opposite_force_test_fixtures opposite_force_catch2)
set_target_properties(opposite_force_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# The existing formatter traverses all of OppositeForce, including ignored output.
file(WRITE "${CMAKE_BINARY_DIR}/.clang-format" "DisableFormat: true\n")
