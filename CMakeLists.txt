#
#  This is a file to be interpreted by cmake (https://cmake.org/)
#  Please note that all dependencies have to be available first.
#

# To uninstall:
#
# sudo xargs --interactive rm < install_manifest.txt
#
# Please note that this will remove also the libraries that might be
# used by other programs.

cmake_minimum_required(VERSION 3.9)

project(nd2tool
  DESCRIPTION "convert Nikon nd2 files to tif"
  LANGUAGES C)

set (CMAKE_C_STANDARD 11)

option (ENABLE_NATIVE_OPTIMIZATION "Enable non-portable optimizations" OFF)

#
# Default build type is RELEASE
#
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#
# Add source files
#
add_executable(nd2tool
  src/main.c
  src/nd2tool.c
  src/tiff_util.c
  src/json_util.c
  src/srgb_from_lambda.c
  src/nd2tool_util.c)

#
# Add headers
#
target_include_directories(nd2tool PRIVATE include/)


find_package(Git)

set(ND2TOOL_GIT_VERSION "unknown")
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --broken
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE ND2TOOL_GIT_VERSION
    RESULT_VARIABLE ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
target_compile_definitions(nd2tool PUBLIC "ND2TOOL_GIT_VERSION=\"${ND2TOOL_GIT_VERSION}\"")

#
# Parse version number from src/nd2tool.h
#
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/src/nd2tool.h ND2TOOL_VERSION_MAJOR
  REGEX "#define[ ]+ND2TOOL_VERSION_MAJOR[ ]+\"[0-9]+\"")
string(REGEX MATCH "\"([0-9]+)\"" _ ${ND2TOOL_VERSION_MAJOR})
set(ND2TOOL_VERSION_MAJOR "${CMAKE_MATCH_1}")

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/src/nd2tool.h ND2TOOL_VERSION_MINOR
  REGEX "#define[ ]+ND2TOOL_VERSION_MINOR[ ]+\"[0-9]+\"")
string(REGEX MATCH "\"([0-9]+)\"" _ ${ND2TOOL_VERSION_MINOR})
set(ND2TOOL_VERSION_MINOR "${CMAKE_MATCH_1}")

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/src/nd2tool.h ND2TOOL_VERSION_PATCH
  REGEX "#define[ ]+ND2TOOL_VERSION_PATCH[ ]+\"[0-9]+\"")
string(REGEX MATCH "\"([0-9]+)\"" _ ${ND2TOOL_VERSION_PATCH})
set(ND2TOOL_VERSION_PATCH "${CMAKE_MATCH_1}")

project(nd2tool VERSION "${ND2TOOL_VERSION_MAJOR}.${ND2TOOL_VERSION_MINOR}.${ND2TOOL_VERSION_PATCH}")


#
# Add Nikon's library
#
target_link_libraries(nd2tool "${CMAKE_SOURCE_DIR}/lib/liblimfile-shared.so")
target_link_libraries(nd2tool "${CMAKE_SOURCE_DIR}/lib/libnd2readsdk-shared.so")
# Copy it to the build dir upon compilation
add_custom_command(TARGET nd2tool POST_BUILD        # Adds a post-build event to MyTest
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
  "${PROJECT_SOURCE_DIR}/lib/*.so"      # <--this is in-file
  $<TARGET_FILE_DIR:nd2tool>)                 # <--this is out-file path

#
# Link time optimization
#

# https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
  message("Enabling IPO")
  set_property(TARGET nd2tool PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_JOBS 8)
else()
  message(WARNING "IPO is not supported: ${output}")
endif()

#
# Architecture optimizations
#
if(ENABLE_NATIVE_OPTIMIZATION)
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
endif()
CHECK_C_COMPILER_FLAG("-mtune=native" COMPILER_SUPPORTS_MTUNE_NATIVE)
if(COMPILER_SUPPORTS_MTUNE_NATIVE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=native")
endif()
endif()


#
# Math library, if needed
#

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(nd2tool ${MATH_LIBRARY})
endif()

#
# TIFF
#
target_link_libraries(nd2tool tiff)
# On Ubuntu 24, first get libtiff.so.5 into the library
# path, then use this line instead:
#target_link_libraries(nd2tool -l:libtiff.so.5)

#
# cJSON
#
target_link_libraries(nd2tool cjson)

# Enable -flto (GCC) and similar if available, see
# https://cmake.org/cmake/help/latest/policy/CMP0069.html
check_ipo_supported(RESULT result)
if(result)
  message("Enabling IPO")
  set_property(DIRECTORY PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include(GNUInstallDirs)
install(TARGETS nd2tool)
install(FILES "${CMAKE_SOURCE_DIR}/doc/nd2tool.1"
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1/" )
install(FILES
  "${CMAKE_SOURCE_DIR}/lib/liblimfile-shared.so"
  "${CMAKE_SOURCE_DIR}/lib/libnd2readsdk-shared.so"
  TYPE LIB )

# https://cmake.org/cmake/help/book/mastering-cmake/chapter/Packaging%20With%20CPack.html
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "ELGW") #required
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # resolve dependencies automatically
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.github.com/elgw/nd2tool/")

INCLUDE(CPack)
