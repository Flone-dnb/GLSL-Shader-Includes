#include "CombinedShaderLanguageParser.h"

// Standard.
#include <fstream>
#include <format>

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::parseHlsl(
    const std::filesystem::path& pathToShaderSourceFile,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    NextBindingIndex nextBindingIndex{};
    return parse(pathToShaderSourceFile, true, nextBindingIndex, vAdditionalIncludeDirectories);
}

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::parseGlsl(
    const std::filesystem::path& pathToShaderSourceFile,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    NextBindingIndex nextBindingIndex{};
    return parse(pathToShaderSourceFile, false, nextBindingIndex, vAdditionalIncludeDirectories);
}

std::optional<CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::processKeywordCode(
    std::string_view sKeyword,
    std::string& sLineBuffer,
    std::ifstream& file,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::function<std::optional<Error>(std::string&)>& processContent) {
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
            std::string sBodyText = sLineBuffer.substr(iBodyStartPosition);
            return processContent(sBodyText);
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
    size_t iNestedScopeCount = 0;
    while (std::getline(file, sLineBuffer)) {
        // See if this line introduces another scope.
        if (sLineBuffer.find('{') != std::string::npos) {
            iNestedScopeCount += 1;
        } else if (sLineBuffer.find('}') != std::string::npos) {
            if (iNestedScopeCount == 0) {
                bFinishedProcessing = true;
                break;
            }

            iNestedScopeCount -= 1;
        }

        auto optionalError = processContent(sLineBuffer);
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError;
        }
    }

    // Make sure there was no unexpected EOF.
    if (!bFinishedProcessing) [[unlikely]] {
        return Error(
            std::format("reached unexpected end of file while processing keyword \"{}\"", sKeyword),
            pathToShaderSourceFile);
    }

    return {};
}

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::parse(
    const std::filesystem::path& pathToShaderSourceFile,
    bool bParseAsHlsl,
    NextBindingIndex& nextBindingIndex,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
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
            [&](std::string& sText) -> std::optional<Error> {
                if (bParseAsHlsl) {
                    convertGlslTypesToHlslTypes(sText);
                }
                vAdditionalPushConstants.push_back(sText);
                bFoundAdditionalPushConstants = true;
                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundAdditionalPushConstants) {
            continue;
        }
#endif

        // Process GLSL keyword (if found).
        bool bFoundLanguageKeyword = false;
        optionalError = processKeywordCode(
            sGlslKeyword,
            sLineBuffer,
            file,
            pathToShaderSourceFile,
            [&](std::string& sText) -> std::optional<Error> {
                if (bParseAsHlsl) {
                    // Ignore this block.
                    bFoundLanguageKeyword = true;
                    return {};
                }

                // Process this block's content.
                const auto optionalError = assignGlslBindingIndexIfFound(sText, nextBindingIndex);
                if (optionalError.has_value()) [[unlikely]] {
                    return Error(optionalError.value(), pathToShaderSourceFile);
                }
                sFullSourceCode += sText + "\n";
                bFoundLanguageKeyword = true;

                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundLanguageKeyword) {
            continue;
        }

        // Process HLSL keyword (if found).
        bFoundLanguageKeyword = false;
        optionalError = processKeywordCode(
            sHlslKeyword,
            sLineBuffer,
            file,
            pathToShaderSourceFile,
            [&](std::string& sText) -> std::optional<Error> {
                if (!bParseAsHlsl) {
                    // Ignore this block.
                    bFoundLanguageKeyword = true;
                    return {};
                }

                // Process this block's content.
                const auto optionalError = assignHlslBindingIndexIfFound(sText, nextBindingIndex);
                if (optionalError.has_value()) [[unlikely]] {
                    return Error(optionalError.value(), pathToShaderSourceFile);
                }
                sFullSourceCode += sText + "\n";
                bFoundLanguageKeyword = true;

                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundLanguageKeyword) {
            continue;
        }

        // Look for the include keyword.
        auto includeResult =
            findIncludePath(sLineBuffer, pathToShaderSourceFile, vAdditionalIncludeDirectories);
        if (std::holds_alternative<Error>(includeResult)) [[unlikely]] {
            return std::get<Error>(std::move(includeResult));
        }
        auto optionalIncludedPath = std::get<std::optional<std::filesystem::path>>(std::move(includeResult));

        if (!optionalIncludedPath.has_value()) {
            // Append the line to the final source code string.
            if (bParseAsHlsl) {
                const auto optionalError = assignHlslBindingIndexIfFound(sLineBuffer, nextBindingIndex);
                if (optionalError.has_value()) [[unlikely]] {
                    return Error(optionalError.value(), pathToShaderSourceFile);
                }
                convertGlslTypesToHlslTypes(sLineBuffer);
            } else {
                const auto optionalError = assignGlslBindingIndexIfFound(sLineBuffer, nextBindingIndex);
                if (optionalError.has_value()) [[unlikely]] {
                    return Error(optionalError.value(), pathToShaderSourceFile);
                }
            }
            sFullSourceCode += sLineBuffer + "\n";
            continue;
        }

        // Process included file.
        auto result = parse(
            optionalIncludedPath.value(), bParseAsHlsl, nextBindingIndex, vAdditionalIncludeDirectories);
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

void CombinedShaderLanguageParser::convertGlslTypesToHlslTypes(std::string& sGlslLine) {
    replaceSubstring(sGlslLine, "vec2", "float2");
    replaceSubstring(sGlslLine, "vec3", "float3");
    replaceSubstring(sGlslLine, "vec4", "float4");

    replaceSubstring(sGlslLine, "mat2", "float2x2");
    replaceSubstring(sGlslLine, "mat3", "float3x3");
    replaceSubstring(sGlslLine, "mat4", "float4x4");

    // Replacing `matnxm` will be wrong since GLSL and HLSL have different row/column specification.
    // TODO: think about this in the future

    if (sGlslLine.starts_with(
            "shared ")) { // avoid replacing `groupshared` to `groupgroupshared` because it matches `shared`
        replaceSubstring(sGlslLine, "shared ", "groupshared ");
    }
}

std::optional<std::string> CombinedShaderLanguageParser::assignGlslBindingIndexIfFound(
    std::string& sGlslLine, NextBindingIndex& nextBindingIndex) {
    // Prepare keywords to look for.
    const std::string sBindingKeyword = "binding";

    // Find binding keyword.
    auto iCurrentPos = sGlslLine.find(sBindingKeyword);
    if (iCurrentPos == std::string::npos) {
        return {};
    }

    // Jump to keyword end.
    iCurrentPos += sBindingKeyword.size();

    // Go forward until `=` is found (skip spaces).
    for (; iCurrentPos < sGlslLine.size(); iCurrentPos++) {
        if (sGlslLine[iCurrentPos] == '=') {
            break;
        }
    }

    // Make sure we found `=`.
    if (sGlslLine[iCurrentPos] != '=') [[unlikely]] {
        return std::format("found `{}` keyword but no `=` after it", sBindingKeyword);
    }
    iCurrentPos += 1;

    // Go forward until a non-space character is found.
    for (; iCurrentPos < sGlslLine.size(); iCurrentPos++) {
        if (sGlslLine[iCurrentPos] != ' ') {
            break;
        }
    }

    // Make sure we will now have our binding index keyword.
    if (sGlslLine.find(sAssignBindingIndexKeyword, iCurrentPos) == std::string::npos) {
        // Found hardcoded index.
        nextBindingIndex.bFoundHardcodedIndex = true;

        // Make sure we don't mix parser-assigned indices and hardcoded indices.
        if (nextBindingIndex.iGlslIndex > 0) {
            return std::format(
                "unexpected to find a hardcoded binding index at line \"{}\" because some parser-assigned "
                "indices were already specified",
                sGlslLine);
        }

        return {};
    }

    // Make sure we don't mix parser-assigned indices and hardcoded indices.
    if (nextBindingIndex.bFoundHardcodedIndex) {
        return std::format(
            "unexpected to find a parser-assigned binding index at line \"{}\" because some hardcoded "
            "indices were already specified",
            sGlslLine);
    }

    // Erase our keyword.
    sGlslLine.erase(iCurrentPos, sAssignBindingIndexKeyword.size());

    // Insert a new binding index.
    sGlslLine.insert(iCurrentPos, std::to_string(nextBindingIndex.iGlslIndex));

    // Increment next available binding index.
    nextBindingIndex.iGlslIndex += 1;

    return {};
}

std::optional<std::string> CombinedShaderLanguageParser::assignHlslBindingIndexIfFound(
    std::string& sHlslLine, NextBindingIndex& nextBindingIndex) {
    // Prepare keywords to look for.
    const std::string sBindingKeyword = "register(";
    const std::string_view sRegisterSpaceKeyword = "space";

    // Find binding keyword.
    auto iCurrentPos = sHlslLine.find(sBindingKeyword);
    if (iCurrentPos == std::string::npos) {
        return {};
    }

    // Jump to binding value.
    iCurrentPos += sBindingKeyword.size();

    // Next character should be register type.
    char registerType = sHlslLine[iCurrentPos];
    iCurrentPos += 1;

    // Make sure we will now have our binding index keyword.
    const auto iKeywordPos = sHlslLine.find(sAssignBindingIndexKeyword, iCurrentPos);
    if (iKeywordPos == std::string::npos) {
        // Found hardcoded index.
        nextBindingIndex.bFoundHardcodedIndex = true;

        // Make sure we don't mix parser-assigned indices and hardcoded indices.
        if (!nextBindingIndex.hlslIndex.empty()) {
            return std::format(
                "unexpected to find a hardcoded register index at line \"{}\" because some parser-assigned "
                "indices were already specified",
                sHlslLine);
        }

        return {};
    }

    // Make sure we don't mix parser-assigned indices and hardcoded indices.
    if (nextBindingIndex.bFoundHardcodedIndex) {
        return std::format(
            "unexpected to find a parser-assigned register index at line \"{}\" because some hardcoded "
            "indices were already specified",
            sHlslLine);
    }

    // Erase our keyword.
    sHlslLine.erase(iKeywordPos, sAssignBindingIndexKeyword.size());

    // Prepare variables to store register info.
    unsigned int iRegisterSpace = 0; // default space if not specified
    unsigned int iRegisterIndex = 0;

    // See if register space is specified.
    const auto iRegisterSpacePos = sHlslLine.find(sRegisterSpaceKeyword.data(), iKeywordPos);
    if (iRegisterSpacePos != std::string::npos) {
        // Process register space.
        const auto iRegisterSpaceEndPos = sHlslLine.find(')', iRegisterSpacePos);
        if (iRegisterSpaceEndPos == std::string::npos) [[unlikely]] {
            return "found register space but not found `)` after it";
        }

        // Read register space value.
        std::string sRegisterSpaceValue;
        for (size_t i = iRegisterSpacePos + sRegisterSpaceKeyword.size(); i < sHlslLine.size(); i++) {
            if (std::isdigit(static_cast<unsigned char>(sHlslLine[i])) == 0) {
                break;
            }
            sRegisterSpaceValue += sHlslLine[i];
        }

        // Make sure it's not empty.
        if (sRegisterSpaceValue.empty()) [[unlikely]] {
            return "found register `space` keyword but no digit after it";
        }

        // Convert to integer.
        try {
            iRegisterSpace = static_cast<unsigned int>(std::stoul(sRegisterSpaceValue));
        } catch (const std::exception& exception) {
            return std::format(
                "failed to convert register space string \"{}\" to integer, error: {}",
                sRegisterSpaceValue,
                exception.what());
        }
    }

    // Get binding info.
    auto& binding = nextBindingIndex.hlslIndex[registerType];

    // Get binding index.
    auto bindingIt = binding.find(iRegisterSpace);
    if (bindingIt == binding.end()) {
        iRegisterIndex = 0;

        // Increment next available binding index.
        binding[iRegisterSpace] = 1;
    } else {
        iRegisterIndex = bindingIt->second;

        // Increment next available binding index.
        bindingIt->second += 1;
    }

    // Insert a new binding index.
    sHlslLine.insert(iKeywordPos, std::to_string(iRegisterIndex));

    return {};
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

std::variant<std::optional<std::filesystem::path>, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::findIncludePath(
    std::string& sLineBuffer,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    // Look for the include keyword.
    if (sLineBuffer.find(sIncludeKeyword) == std::string::npos) {
        return {};
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
                "expected to find open/close quotes between the included path on line \"{}\"", sLineBuffer),
            pathToShaderSourceFile);
    }

    // Erase a space and a quote.
    sLineBuffer.erase(0, 2);

    // Erase last quote.
    sLineBuffer.pop_back();

    // Now the line buffer only has a relative include path.
    // Build a path to the included file.
    auto pathToIncludedFile = pathToShaderSourceFile.parent_path() / sLineBuffer;
    if (!std::filesystem::exists(pathToIncludedFile)) {
        // Attempt to use additional include directories.
        bool bFoundFilePath = false;
        for (const auto& pathToDirectory : vAdditionalIncludeDirectories) {
            // Construct new path.
            pathToIncludedFile = pathToDirectory / sLineBuffer;

            // Check if it exists.
            if (std::filesystem::exists(pathToIncludedFile)) {
                bFoundFilePath = true;
                break;
            }
        }

        // Make sure we found an existing path.
        if (!bFoundFilePath) [[unlikely]] {
            return Error(
                std::format("unable to find included file \"{}\"", sLineBuffer), pathToShaderSourceFile);
        }
    }

    return pathToIncludedFile;
}
