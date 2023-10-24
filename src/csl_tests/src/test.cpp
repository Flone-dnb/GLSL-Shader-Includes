// Custom.
#include "CombinedShaderLanguageParser.h"

// External.
#include "catch2/catch_test_macros.hpp"

TEST_CASE("parse a sample file") {
    // Prepare path to the shader file we will test.
    const std::filesystem::path pathToShaderSourceFile = "res/test/shaderA.glsl";
    const std::filesystem::path pathToResultingShaderSourceFile = "res/test/result.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) {
        INFO("expected the file \"" + pathToShaderSourceFile.string() + "\" to exist");
        REQUIRE(false);
    }

    // Parse the source code.
    auto result = CombinedShaderLanguageParser::parseGlsl(pathToShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = CombinedShaderLanguageParser::parseGlsl(pathToResultingShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}

#if defined(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
TEST_CASE("parse a sample file with additional push constants") {
    // Prepare path to the shader file we will test.
    const std::filesystem::path pathToShaderSourceFile = "res/test/additionalPushConstants.glsl";
    const std::filesystem::path pathToResultingShaderSourceFile = "res/test/pushConstantsResult.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) {
        INFO("expected the file \"" + pathToShaderSourceFile.string() + "\" to exist");
        REQUIRE(false);
    }

    // Parse the source code.
    auto result = CombinedShaderLanguageParser::parseGlsl(pathToShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = CombinedShaderLanguageParser::parseGlsl(pathToResultingShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}
#endif

TEST_CASE("parse combined file as GLSL") {
    // Prepare path to the shader file we will test.
    const std::filesystem::path pathToShaderSourceFile = "res/test/combined.glsl";
    const std::filesystem::path pathToResultingShaderSourceFile = "res/test/combined_as_glsl_result.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) {
        INFO("expected the file \"" + pathToShaderSourceFile.string() + "\" to exist");
        REQUIRE(false);
    }

    // Parse the source code.
    auto result = CombinedShaderLanguageParser::parseGlsl(pathToShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = CombinedShaderLanguageParser::parseGlsl(pathToResultingShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}

TEST_CASE("parse combined file as HLSL") {
    // Prepare path to the shader file we will test.
    const std::filesystem::path pathToShaderSourceFile = "res/test/combined.glsl";
    const std::filesystem::path pathToResultingShaderSourceFile = "res/test/combined_as_hlsl_result.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) {
        INFO("expected the file \"" + pathToShaderSourceFile.string() + "\" to exist");
        REQUIRE(false);
    }

    // Parse the source code.
    auto result = CombinedShaderLanguageParser::parseHlsl(pathToShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = CombinedShaderLanguageParser::parseHlsl(pathToResultingShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}

TEST_CASE("parse using additional include directories") {
    // Prepare path to the shader file we will test.
    const std::filesystem::path pathToShaderSourceFile = "res/test/additional_include_directories.glsl";
    const std::filesystem::path pathToResultingShaderSourceFile =
        "res/test/additional_include_directories_result.glsl";

    // Make sure the shader file exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) {
        INFO("expected the file \"" + pathToShaderSourceFile.string() + "\" to exist");
        REQUIRE(false);
    }

    // Parse the source code.
    auto result = CombinedShaderLanguageParser::parseGlsl(pathToShaderSourceFile, {"res/test/additional"});
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = CombinedShaderLanguageParser::parseGlsl(pathToResultingShaderSourceFile);
    if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}
