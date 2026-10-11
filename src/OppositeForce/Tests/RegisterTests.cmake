if(CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "Opposite Force tests require CMake 3.19 or newer.")
endif()

# Invoked only in the separate test build through CMAKE_PROJECT_OpenXcom_INCLUDE.
# Wait until the unchanged engine target has been defined before inspecting it.
function(addOppositeForceTests)
    # CMake forbids add_subdirectory() during deferred execution. Including the
    # test definitions registers their targets without creating a subdirectory.
    include("${CMAKE_SOURCE_DIR}/src/OppositeForce/Tests/CMakeLists.txt")
endfunction()

cmake_language(DEFER CALL addOppositeForceTests)
