// Custom.
#include "CombinedShaderLanguageParser.h"

// External.
#include "catch2/catch_test_macros.hpp"

void testCompareParsingResults(
    const std::filesystem::path& pathToDirectory, unsigned int iBaseAutomaticBindingIndex = 0) {
    // Make sure the path exists.
    if (!std::filesystem::exists(pathToDirectory)) [[unlikely]] {
        INFO("expected the path \"" + pathToDirectory.string() + "\" to exist");
        REQUIRE(false);
    }

    // Prepare path to parse.
    const auto pathToParse = pathToDirectory / "to_parse.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToParse)) [[unlikely]] {
        INFO("expected the file \"" + pathToParse.string() + "\" to exist");
        REQUIRE(false);
    }

    // Prepare paths to results.
    const auto pathToResultAsHlsl = pathToDirectory / "result.hlsl";
    const auto pathToResultAsGlsl = pathToDirectory / "result.glsl";
    const auto bHlslResultExists = std::filesystem::exists(pathToResultAsHlsl);
    const auto bGlslResultExists = std::filesystem::exists(pathToResultAsGlsl);

    // Make sure at least one result exists.
    if (!bHlslResultExists && !bGlslResultExists) [[unlikely]] {
        INFO(
            "expected at least one result file (.glsl or .hlsl) to exist in the directory \"" +
            pathToDirectory.string() + "\"");
        REQUIRE(false);
    }

    // Check if additional include directory is specified.
    std::vector<std::filesystem::path> vAdditionalIncludeDirectories;
    const auto pathToAdditionalIncludes = pathToDirectory / "additional_include";
    if (std::filesystem::exists(pathToAdditionalIncludes)) {
        vAdditionalIncludeDirectories.push_back(pathToAdditionalIncludes);
    }

    // Parse the source code.
    std::variant<std::string, CombinedShaderLanguageParser::Error> result;
    if (bHlslResultExists) {
        result = CombinedShaderLanguageParser::parseHlsl(pathToParse, vAdditionalIncludeDirectories);
    } else {
        result = CombinedShaderLanguageParser::parseGlsl(
            pathToParse, iBaseAutomaticBindingIndex, vAdditionalIncludeDirectories);
    }
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) [[unlikely]] {
        const auto error = std::get<CombinedShaderLanguageParser::Error>(std::move(result));
        INFO(std::format("{} | path: {}", error.sErrorMessage, error.pathToErrorFile.string()));
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    if (bHlslResultExists) {
        result = CombinedShaderLanguageParser::parseHlsl(pathToResultAsHlsl);
    } else {
        result = CombinedShaderLanguageParser::parseGlsl(pathToResultAsGlsl);
    }
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) [[unlikely]] {
        const auto error = std::get<CombinedShaderLanguageParser::Error>(std::move(result));
        INFO(std::format("{} | path: {}", error.sErrorMessage, error.pathToErrorFile.string()));
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    INFO("checking directory: " + pathToDirectory.filename().string());

    REQUIRE(sResultingCode == sExpectedCode);

    INFO("[TEST PASSED] directory: " + pathToDirectory.filename().string());
}

#if defined(ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD)
TEST_CASE("parse a sample file with additional push constants") {
    testCompareParsingResults("res/test/additional_push_constants");
}

TEST_CASE("parse a sample file with additional root constants") {
    testCompareParsingResults("res/test/additional_root_constants");
}
#endif

TEST_CASE("parse combined file") { testCompareParsingResults("res/test/combined"); }

TEST_CASE("parse using additional include directories") {
    testCompareParsingResults("res/test/additional_include_directories");
}

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
TEST_CASE("parse a file with hardcoded binding indices after parser-assigned") {
    testCompareParsingResults("res/test/hardcoded_binding_indices_after_auto");
}

TEST_CASE("parse a file with hardcoded binding indices before parser-assigned") {
    testCompareParsingResults("res/test/hardcoded_binding_indices_before_auto");
}

TEST_CASE("parse a file with mixed indices and non-zero auto binding index") {
    testCompareParsingResults("res/test/non_zero_base_auto_binding_index", 100);
}
#endif
