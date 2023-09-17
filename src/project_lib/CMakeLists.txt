cmake_minimum_required(VERSION 3.20)

project(GlslShaderIncluderLib)

# Define some relative paths.
set(RELATIVE_EXT_PATH "../../ext")
set(RELATIVE_CMAKE_HELPERS_PATH "../.cmake")

# Include essential stuff.
include(${RELATIVE_CMAKE_HELPERS_PATH}/essential.cmake)

# Include helper functions.
include(${RELATIVE_CMAKE_HELPERS_PATH}/utils.cmake)

# -------------------------------------------------------------------------------------------------
#                                            OPTIONS
# -------------------------------------------------------------------------------------------------

option(GLSL_SHADER_INCLUDER_ENABLE_DOXYGEN "Defines whether to enable documentation generation on build or not." ON)

# -------------------------------------------------------------------------------------------------
#                                          TARGET SOURCES
# -------------------------------------------------------------------------------------------------

# Sources.
set(PROJECT_SOURCES
    src/ShaderIncluder.h
    src/ShaderIncluder.cpp
    # add your .h/.cpp files here
)

# Define target.
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

# -------------------------------------------------------------------------------------------------
#                                         CONFIGURE TARGET
# -------------------------------------------------------------------------------------------------

# Set target folder.
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_FOLDER})

# Enable more warnings and warnings as errors.
enable_more_warnings()

# Enable doxygen.
if (GLSL_SHADER_INCLUDER_ENABLE_DOXYGEN)
    enable_doxygen(${CMAKE_CURRENT_SOURCE_DIR}/../../docs)
endif()

# Set C++ standard.
set(PROJECT_CXX_STANDARD_VERSION 20)
set(CMAKE_CXX_STANDARD ${PROJECT_CXX_STANDARD_VERSION})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_${PROJECT_CXX_STANDARD_VERSION})
message(STATUS "${PROJECT_NAME}: using the following C++ standard: ${CMAKE_CXX_STANDARD}")

# Add includes.
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ext)
target_include_directories(${PROJECT_NAME} PUBLIC src)

# -------------------------------------------------------------------------------------------------
#                                           TOOLS
# -------------------------------------------------------------------------------------------------

# Enable Address Sanitizer in `Debug` builds on non-Windows OS.
if(NOT IS_RELEASE_BUILD AND NOT WIN32)
    enable_address_sanitizer()
endif()

# Enable Clang-tidy.
enable_clang_tidy(${CMAKE_CURRENT_LIST_DIR}/../../.clang-tidy)

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# nothing here yet
