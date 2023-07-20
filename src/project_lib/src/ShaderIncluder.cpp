#include "ShaderIncluder.h"

// Standard.
#include <fstream>

std::variant<std::string, ShaderIncluder::Error>
ShaderIncluder::parseFullSourceCode(const std::filesystem::path& pathToShaderSourceFile) {
    // Make sure the specified path exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) [[unlikely]] {
        return Error::CANT_OPEN_FILE;
    }

    // Make sure the specified file is a file.
    if (std::filesystem::is_directory(pathToShaderSourceFile)) [[unlikely]] {
        return Error::PATH_IS_NOT_A_FILE;
    }

    // Make sure the specified path has a parent path.
    if (!pathToShaderSourceFile.has_parent_path()) [[unlikely]] {
        return Error::PATH_HAS_NO_PARENT_PATH;
    }

    // Define some variables.
    constexpr std::string_view sIncludeKeyword = "#include";

    // Open the file.
    std::ifstream file(pathToShaderSourceFile);
    if (!file.is_open()) [[unlikely]] {
        return Error::CANT_OPEN_FILE;
    }

    std::string sFullSourceCode;
    std::string sLineBuffer;
    while (std::getline(file, sLineBuffer)) {
        // Look for the include keyword.
        if (sLineBuffer.find(sIncludeKeyword) == std::string::npos) {
            // Just append the line to the final source code string.
            sFullSourceCode += sLineBuffer + "\n";
            continue;
        }

        // Remove the include keyword from the buffer.
        sLineBuffer.erase(0, sIncludeKeyword.size());

        // Make sure that the line is not empty now.
        // Even if the line has some character (probably a space) it's still not enough.
        // Check for `2` in order to not crash when we check for the first quote.
        if (sLineBuffer.size() < 2) [[unlikely]] {
            return Error::NOTHING_AFTER_INCLUDE;
        }

        // The first character should be a space now.
        if (sLineBuffer[0] != ' ') [[unlikely]] {
            return Error::NO_SPACE_AFTER_KEYWORD;
        }

        // The second character and the last one should be quotes.
        if (sLineBuffer[1] != '\"' || !sLineBuffer.ends_with("\"")) [[unlikely]] {
            return Error::MISSING_QUOTES;
        }

        // Erase a space and a quote.
        sLineBuffer.erase(0, 2);

        // Erase last quote.
        sLineBuffer.pop_back();

        // Now the line buffer only has a relative include path.
        // Build a path to the included file.
        const auto pathToIncludedFile = pathToShaderSourceFile.parent_path() / sLineBuffer;

        // Process included file.
        auto result = parseFullSourceCode(pathToIncludedFile);
        if (std::holds_alternative<Error>(result)) {
            return std::get<Error>(result);
        }
        sFullSourceCode += std::get<std::string>(std::move(result));
    }

    file.close();

    return sFullSourceCode;
}
