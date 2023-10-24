#include "CombinedShaderLanguageParser.h"

// Standard.
#include <fstream>
#include <ranges>

std::variant<std::string, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::parseHlsl(const std::filesystem::path& pathToShaderSourceFile) {
    return parse(pathToShaderSourceFile, true);
}

std::variant<std::string, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::parseGlsl(const std::filesystem::path& pathToShaderSourceFile) {
    return parse(pathToShaderSourceFile, false);
}

std::optional<CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::processKeywordCode(
    std::string_view sKeyword,
    std::string& sLineBuffer,
    std::ifstream& file,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::function<void(const std::string&)>& processContent) {
    // Look for the keyword.
    const auto iKeywordPosition = sLineBuffer.find(sKeyword);
    if (iKeywordPosition == std::string::npos) {
        return {};
    }

    // Check if this line contains open curly bracket.
    bool bFoundBlockStart = false;
    for (size_t i = iKeywordPosition + sKeyword.size(); i < sLineBuffer.size(); i++) {
        // Skip spaces.
        if (sLineBuffer[i] == ' ') {
            continue;
        }

        if (sLineBuffer[i] == '{') {
            // Found block of code.
            bFoundBlockStart = true;
            break;
        }

        // Found single line of code.
        bFoundBlockStart = false;
        break;
    }

    if (!bFoundBlockStart) {
        // Check if the code (body) starts after the keyword.
        auto iBodyStartPosition = iKeywordPosition + sKeyword.size() + 1;
        for (; iBodyStartPosition < sLineBuffer.size(); iBodyStartPosition++) {
            if (sLineBuffer[iBodyStartPosition] != ' ') {
                break;
            }
        }

        if (iBodyStartPosition <
            sLineBuffer.size() - 1) { // `-1` to make sure there is at least 1 character to process
            // Trigger callback on text after keyword.
            processContent(sLineBuffer.substr(iBodyStartPosition));

            return {};
        }

        // Read next line.
        if (!std::getline(file, sLineBuffer)) [[unlikely]] {
            return Error(
                std::format("unexpected end of file while processing keyword \"{}\"", sKeyword),
                pathToShaderSourceFile);
        }

        // Expecting to find a curly bracket.
        if (!sLineBuffer.starts_with('{')) {
            return Error(
                std::format(
                    "expected to find a curly bracket on line \"{}\" while processing keyword \"{}\"",
                    sLineBuffer,
                    sKeyword),
                pathToShaderSourceFile);
        }
    }

    // Process body.
    bool bFinishedProcessing = false;
    while (std::getline(file, sLineBuffer)) {
        // See if we reached the end of the block.
        if (sLineBuffer.starts_with('}')) {
            bFinishedProcessing = true;
            break;
        }

        processContent(sLineBuffer);
    }

    // Make sure there was no unexpected EOF.
    if (!bFinishedProcessing) [[unlikely]] {
        return Error(
            std::format("reached unexpected end of file while processing keyword \"{}\"", sKeyword),
            pathToShaderSourceFile);
    }

    return {};
}

std::variant<std::string, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::parse(const std::filesystem::path& pathToShaderSourceFile, bool bParseAsHlsl) {
    // Make sure the specified path exists.
    if (!std::filesystem::exists(pathToShaderSourceFile)) [[unlikely]] {
        return Error("can't open file", pathToShaderSourceFile);
    }

    // Make sure the specified file is a file.
    if (std::filesystem::is_directory(pathToShaderSourceFile)) [[unlikely]] {
        return Error("not a file", pathToShaderSourceFile);
    }

    // Make sure the specified path has a parent path.
    if (!pathToShaderSourceFile.has_parent_path()) [[unlikely]] {
        return Error("no parent path", pathToShaderSourceFile);
    }

    // Define some variables.
    constexpr std::string_view sIncludeKeyword = "#include";
#if defined(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
    std::vector<std::string> vAdditionalPushConstants;
#endif

    // Open the file.
    std::ifstream file(pathToShaderSourceFile);
    if (!file.is_open()) [[unlikely]] {
        return Error("can't open file", pathToShaderSourceFile);
    }

    std::string sFullSourceCode;
    std::string sLineBuffer;
    while (std::getline(file, sLineBuffer)) {
#if defined(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
        // Process additional push constants (if found).
        bool bFoundAdditionalPushConstants = false;
        auto optionalError = processKeywordCode(
            sAdditionalPushConstantsKeyword,
            sLineBuffer,
            file,
            pathToShaderSourceFile,
            [&](const std::string& sText) {
                vAdditionalPushConstants.push_back(bParseAsHlsl ? convertGlslTypesToHlslTypes(sText) : sText);
                bFoundAdditionalPushConstants = true;
            });
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundAdditionalPushConstants) {
            continue;
        }
#endif

        // Process GLSL keyword.
        bool bFoundLanguageKeyword = false;
        if (bParseAsHlsl) {
            optionalError = processKeywordCode(
                sGlslKeyword, sLineBuffer, file, pathToShaderSourceFile, [&](const std::string& sText) {
                    // Ignore this block.
                    bFoundLanguageKeyword = true;
                });
        } else {
            optionalError = processKeywordCode(
                sGlslKeyword, sLineBuffer, file, pathToShaderSourceFile, [&](const std::string& sText) {
                    // Just copy-paste this block's content.
                    sFullSourceCode += sText + "\n";
                    bFoundLanguageKeyword = true;
                });
        }
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundLanguageKeyword) {
            continue;
        }

        // Process HLSL keyword.
        bFoundLanguageKeyword = false;
        if (bParseAsHlsl) {
            optionalError = processKeywordCode(
                sHlslKeyword, sLineBuffer, file, pathToShaderSourceFile, [&](const std::string& sText) {
                    // Just copy-paste this block's content.
                    sFullSourceCode += sText + "\n";
                    bFoundLanguageKeyword = true;
                });
        } else {
            optionalError = processKeywordCode(
                sHlslKeyword, sLineBuffer, file, pathToShaderSourceFile, [&](const std::string& sText) {
                    // Ignore this block.
                    bFoundLanguageKeyword = true;
                });
        }
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundLanguageKeyword) {
            continue;
        }

        // Look for the include keyword.
        if (sLineBuffer.find(sIncludeKeyword) == std::string::npos) {
            // Just append the line to the final source code string.
            sFullSourceCode += (bParseAsHlsl ? convertGlslTypesToHlslTypes(sLineBuffer) : sLineBuffer) + "\n";
            continue;
        }

        // Remove the include keyword from the buffer.
        sLineBuffer.erase(0, sIncludeKeyword.size());

        // Make sure that the line is not empty now.
        // Even if the line has some character (probably a space) it's still not enough.
        // Check for `2` in order to not crash when we check for the first quote.
        if (sLineBuffer.size() < 2) [[unlikely]] {
            return Error(
                std::format("expected to find path after #include on line \"{}\"", sLineBuffer),
                pathToShaderSourceFile);
        }

        // The first character should be a space now.
        if (sLineBuffer[0] != ' ') [[unlikely]] {
            return Error(
                std::format(
                    "expected to find 1 empty space character after the keyword on line \"{}\"", sLineBuffer),
                pathToShaderSourceFile);
        }

        // The second character and the last one should be quotes.
        if (sLineBuffer[1] != '\"' || !sLineBuffer.ends_with("\"")) [[unlikely]] {
            return Error(
                std::format(
                    "expected to find open/close quotes between the included path on line \"{}\"",
                    sLineBuffer),
                pathToShaderSourceFile);
        }

        // Erase a space and a quote.
        sLineBuffer.erase(0, 2);

        // Erase last quote.
        sLineBuffer.pop_back();

        // Now the line buffer only has a relative include path.
        // Build a path to the included file.
        const auto pathToIncludedFile = pathToShaderSourceFile.parent_path() / sLineBuffer;

        // Process included file.
        auto result = parse(pathToIncludedFile, bParseAsHlsl);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            return std::get<Error>(result);
        }
        sFullSourceCode += std::get<std::string>(std::move(result));
    }

    file.close();

#if defined(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
    // Now insert additional push constants.
    if (!vAdditionalPushConstants.empty()) {
        // Find where push constants start.
        const auto iPushConstantsStartPos = sFullSourceCode.find("layout(push_constant)");
        if (iPushConstantsStartPos == std::string::npos) [[unlikely]] {
            return Error(
                "additional push constants were found and includes of the file were processed "
                "but initial push constants layout was not found in the included files",
                pathToShaderSourceFile);
        }

        // Look for '}' after push constants.
        const auto iPushConstantsEndPos = sFullSourceCode.find('}', iPushConstantsStartPos);
        if (iPushConstantsEndPos == std::string::npos) [[unlikely]] {
            return Error(
                "expected to find a closing bracket after push constants definition", pathToShaderSourceFile);
        }

        const size_t iAdditionalPushConstantsInsertPos = iPushConstantsEndPos;

        // Insert additional push constants (insert in reserve order because of how `std::string::insert`
        // below works, to make the resulting order is correct).
        for (auto reverseIt = vAdditionalPushConstants.rbegin(); reverseIt != vAdditionalPushConstants.rend();
             ++reverseIt) {
            auto& sPushConstant = *reverseIt;
            if (!sPushConstant.ends_with('\n')) {
                sPushConstant += '\n';
            }
            sFullSourceCode.insert(iAdditionalPushConstantsInsertPos, sPushConstant);
        }
    }
#endif

    return sFullSourceCode;
}

std::string CombinedShaderLanguageParser::convertGlslTypesToHlslTypes(const std::string& sGlslCode) {
    auto sConvertedCode = sGlslCode;

    replaceSubstring(sConvertedCode, "vec2 ", "float2 ");
    replaceSubstring(sConvertedCode, "vec3 ", "float3 ");
    replaceSubstring(sConvertedCode, "vec4 ", "float4 ");

    replaceSubstring(sConvertedCode, "mat2 ", "float2x2 ");
    replaceSubstring(sConvertedCode, "mat3 ", "float3x3 ");
    replaceSubstring(sConvertedCode, "mat4 ", "float4x4 ");

    // Replacing `matnxm` will be wrong since GLSL and HLSL have different row/column specification.
    // TODO: think about this in the future

    return sConvertedCode;
}

void CombinedShaderLanguageParser::replaceSubstring(
    std::string& sText, std::string_view sReplaceFrom, std::string_view sReplaceTo) {
    // Prepare initial position.
    size_t iCurrentPosition = 0;

    do {
        // Find text to replace.
        iCurrentPosition = sText.find(sReplaceFrom, iCurrentPosition);
        if (iCurrentPosition == std::string::npos) {
            return;
        }

        // Erase old text.
        sText.erase(iCurrentPosition, sReplaceFrom.size());

        // Replace with new text.
        sText.insert(iCurrentPosition, sReplaceTo);

        // Increment current position.
        iCurrentPosition += sReplaceTo.size();
    } while (iCurrentPosition < sText.size());
}
