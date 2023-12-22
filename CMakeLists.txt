cmake_minimum_required(VERSION 3.20)

project(CombinedShaderLanguageParser)

set(BUILD_DIRECTORY_NAME OUTPUT)

# Tests.
option(CSL_ENABLE_TESTS "Defines whether to add tests target or not." ON)

# Optional feature.
option(CSL_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD "Defines whether contents of `#additional_push_constants` will be removed and added to push constants or not." ON)
if (CSL_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
    add_compile_definitions(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
endif()

# Optional feature.
option(CSL_ENABLE_AUTOMATIC_BINDING_INDEX_ASSIGNMENT_KEYWORD "Defined whether the parser is allowed to replace special keywords with free binding indices." ON)
if (CSL_ENABLE_AUTOMATIC_BINDING_INDEX_ASSIGNMENT_KEYWORD)
    add_compile_definitions(ENABLE_AUTOMATIC_BINDING_INDICES)
endif()

# Add project lib.
set(PROJECT_LIB_DIRECTORY csl_lib)
message(STATUS "Adding target ${PROJECT_LIB_DIRECTORY}...")
add_subdirectory(src/${PROJECT_LIB_DIRECTORY} ${BUILD_DIRECTORY_NAME}/${PROJECT_LIB_DIRECTORY})

if (CSL_ENABLE_TESTS)
    # Add project tests target.
    set(PROJECT_TESTS_DIRECTORY csl_tests)
    message(STATUS "Adding target ${PROJECT_LIB_DIRECTORY}...")
    add_subdirectory(src/${PROJECT_TESTS_DIRECTORY} ${BUILD_DIRECTORY_NAME}/${PROJECT_TESTS_DIRECTORY})
endif()
