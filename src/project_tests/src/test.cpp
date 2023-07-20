// Custom.
#include "ShaderIncluder.h"

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
    auto result = ShaderIncluder::parseFullSourceCode(pathToShaderSourceFile);
    if (std::holds_alternative<ShaderIncluder::Error>(result)) {
        REQUIRE(false);
    }
    const auto sResultingCode = std::get<std::string>(std::move(result));

    // Compare the resulting code with the expected code.
    // (pushing the file though the parser to have constant line endings and stuff)
    result = ShaderIncluder::parseFullSourceCode(pathToResultingShaderSourceFile);
    if (std::holds_alternative<ShaderIncluder::Error>(result)) {
        REQUIRE(false);
    }
    const auto sExpectedCode = std::get<std::string>(std::move(result));

    REQUIRE(sResultingCode == sExpectedCode);
}
