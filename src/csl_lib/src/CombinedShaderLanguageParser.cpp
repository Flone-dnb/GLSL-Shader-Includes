#include "CombinedShaderLanguageParser.h"

// Standard.
#include <fstream>
#include <format>
#include <array>

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::runParsing(
    const std::filesystem::path& pathToShaderSourceFile,
    bool bParseAsHlsl,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories,
    unsigned int iBaseAutomaticBindingIndex) {
    // Prepare some variables.
    BindingIndicesInfo bindingIndicesInfo{};
    std::vector<std::string> vFoundAdditionalPushConstants;

    // Parse.
    auto result = parseFile(
        pathToShaderSourceFile,
        bParseAsHlsl,
        bindingIndicesInfo,
        vFoundAdditionalPushConstants,
        vAdditionalIncludeDirectories);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        return std::get<Error>(std::move(result));
    }
    auto sFullParsedSourceCode = std::get<std::string>(std::move(result));

    // Finalize.
    auto optionalError = finalizeParsingResults(
        pathToShaderSourceFile,
        bParseAsHlsl,
        bindingIndicesInfo,
        sFullParsedSourceCode,
        vFoundAdditionalPushConstants,
        iBaseAutomaticBindingIndex);
    if (optionalError.has_value()) [[unlikely]] {
        return optionalError.value();
    }

    return sFullParsedSourceCode;
}

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::parseHlsl(
    const std::filesystem::path& pathToShaderSourceFile,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    return runParsing(pathToShaderSourceFile, true, vAdditionalIncludeDirectories);
}

std::variant<std::string, CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::parseGlsl(
    const std::filesystem::path& pathToShaderSourceFile,
    unsigned int iBaseAutomaticBindingIndex,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    return runParsing(
        pathToShaderSourceFile, false, vAdditionalIncludeDirectories, iBaseAutomaticBindingIndex);
}

std::optional<CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::processKeywordCode(
    const std::vector<std::string_view>& vKeywords,
    std::string& sLineBuffer,
    std::ifstream& file,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::function<std::optional<Error>(std::string_view sKeyword, std::string& sText)>&
        processContent) {
    // Find a keyword.
    std::string sKeyword;
    size_t iKeywordPosition = std::string::npos;
    for (const auto& sTestKeyword : vKeywords) {
        // Look for the keyword.
        iKeywordPosition = sLineBuffer.find(sTestKeyword);
        if (iKeywordPosition == std::string::npos) {
            continue;
        }

        sKeyword = sTestKeyword;
        break;
    }
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
            return processContent(sKeyword, sBodyText);
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

        auto optionalError = processContent(sKeyword, sLineBuffer);
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

std::variant<bool, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::processMixedLanguageLine(
    std::string& sLineBuffer,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::function<std::optional<Error>(std::string_view, std::string&)>& processContent) {
    // Make sure all 3 keywords are on the same line.
    const auto iHlslKeywordPos = sLineBuffer.find(sHlslKeyword);
    const auto iGlslKeywordPos = sLineBuffer.find(sGlslKeyword);
    const auto iBothKeywordPos = sLineBuffer.find(sBothKeyword);

    if (iHlslKeywordPos == std::string::npos || iGlslKeywordPos == std::string::npos) {
        return false;
    }

    // Make sure keyword only occur once.
    if (sLineBuffer.find(sHlslKeyword, iHlslKeywordPos + 1) != std::string::npos) [[unlikely]] {
        return Error(
            std::format(
                "found keyword \"{}\" because repeated multiple times on line \"{}\" - this is not supported",
                sHlslKeyword,
                sLineBuffer),
            pathToShaderSourceFile);
    }
    if (sLineBuffer.find(sGlslKeyword, iGlslKeywordPos + 1) != std::string::npos) [[unlikely]] {
        return Error(
            std::format(
                "found keyword \"{}\" because repeated multiple times on line \"{}\" - this is not supported",
                sGlslKeyword,
                sLineBuffer),
            pathToShaderSourceFile);
    }

    // Prepare a struct to store info about tagged section on code (tagged as HLSL/GLSL or both).
    struct TaggedSection {
        TaggedSection() = delete;
        TaggedSection(std::string_view sKeyword, size_t iKeywordStartPos)
            : sKeyword(sKeyword), iKeywordStartPos(iKeywordStartPos) {
            iCodeStartPos = iKeywordStartPos + sKeyword.size() + 1; // `+1` for a space after the keyword
        }

        std::string_view sKeyword;
        size_t iKeywordStartPos = 0;
        size_t iCodeStartPos = 0;
    };

    std::vector<TaggedSection> vTaggedSections = {
        TaggedSection(sHlslKeyword, iHlslKeywordPos), TaggedSection(sGlslKeyword, iGlslKeywordPos)};
    if (iBothKeywordPos != std::string::npos) {
        vTaggedSections.push_back(TaggedSection(sBothKeyword, iBothKeywordPos));
    }

    // Sort them by their position.
    std::sort(
        vTaggedSections.begin(),
        vTaggedSections.end(),
        [](const TaggedSection& section1, const TaggedSection& section2) -> bool {
            return section1.iKeywordStartPos < section2.iKeywordStartPos;
        });

    const auto& section1 = vTaggedSections[0];
    const auto& section2 = vTaggedSections[1];

    std::optional<Error> optionalError = {};

    // Check if there is some code before the first keyword.
    if (section1.iKeywordStartPos != 0) {
        auto sCodeBeforeSections = sLineBuffer.substr(0, section1.iKeywordStartPos);
        optionalError = processContent("", sCodeBeforeSections);
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError.value();
        }
    }

    // Process section 1.
    {
        const auto iSection1CodeLength = section2.iKeywordStartPos - section1.iCodeStartPos;
        if (iSection1CodeLength == 0) [[unlikely]] {
            return Error(
                std::format("no code/space between keywords on line \"{}\"", sLineBuffer),
                pathToShaderSourceFile);
        }

        auto sCodeInSection1 = sLineBuffer.substr(section1.iCodeStartPos, iSection1CodeLength);

        optionalError = processContent(section1.sKeyword, sCodeInSection1);
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError.value();
        }
    }

    // Process section 2.
    {
        const auto iSectionEndPos =
            vTaggedSections.size() == 3 ? vTaggedSections[2].iKeywordStartPos : sLineBuffer.size();

        const auto iSection2CodeLength = iSectionEndPos - section2.iCodeStartPos;
        if (iSection2CodeLength == 0) [[unlikely]] {
            return Error(
                std::format("no code/space between keywords on line \"{}\"", sLineBuffer),
                pathToShaderSourceFile);
        }

        auto sCodeInSection2 = sLineBuffer.substr(section2.iCodeStartPos, iSection2CodeLength);

        optionalError = processContent(section2.sKeyword, sCodeInSection2);
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError.value();
        }
    }

    if (vTaggedSections.size() == 3) {
        const auto& section3 = vTaggedSections[2];

        // Process section 3.
        const auto iSection3CodeLength = sLineBuffer.size() - section3.iCodeStartPos;

        auto sCodeInSection3 = sLineBuffer.substr(section3.iCodeStartPos, iSection3CodeLength);

        optionalError = processContent(section3.sKeyword, sCodeInSection3);
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError.value();
        }
    }

    return true;
}

std::variant<std::string, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::parseFile( // NOLINT: too complex
    const std::filesystem::path& pathToShaderSourceFile,
    bool bParseAsHlsl,
    BindingIndicesInfo& bindingIndicesInfo,
    std::vector<std::string>& vFoundAdditionalShaderConstants,
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

    // Open the file.
    std::ifstream file(pathToShaderSourceFile);
    if (!file.is_open()) [[unlikely]] {
        return Error("can't open file", pathToShaderSourceFile);
    }

    std::string sFullSourceCode;
    std::string sLineBuffer;
    while (std::getline(file, sLineBuffer)) {
#if defined(ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD)
        // Process additional push constants (if found).
        bool bFoundAdditionalPushConstants = false;
        std::vector<std::string_view> vShaderConstantsKeywords = {};
        auto optionalError = processKeywordCode(
            {sAdditionalShaderConstantsKeyword,
             sAdditionalRootConstantsKeyword,
             sAdditionalPushConstantsKeyword},
            sLineBuffer,
            file,
            pathToShaderSourceFile,
            [&](std::string_view sKeyword, std::string& sText) -> std::optional<Error> {
                // Skip this block after we finish processing it.
                bFoundAdditionalPushConstants = true;

                // Ignore variables if wrong keyword.
                if ((bParseAsHlsl && sKeyword == sAdditionalPushConstantsKeyword) ||
                    (!bParseAsHlsl && sKeyword == sAdditionalRootConstantsKeyword)) {
                    return {};
                }

                if (bParseAsHlsl) {
                    convertGlslTypesToHlslTypes(sText);
                }
                vFoundAdditionalShaderConstants.push_back(sText);
                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundAdditionalPushConstants) {
            continue;
        }
#endif

        bool bFoundLanguageKeyword = false;
        bool bAddNewLineAfterProcessingKeywordContent = true;

        // Prepare a lambda to process GLSL code.
        const auto processGlslCode = [&](std::string_view sKeyword,
                                         std::string& sText) -> std::optional<Error> {
            if (bParseAsHlsl) {
                // Ignore this block.
                bFoundLanguageKeyword = true;
                return {};
            }

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
            // Find hardcoded binding indices.
            auto optionalError = addHardcodedBindingIndexIfFound(bParseAsHlsl, sText, bindingIndicesInfo);
            if (optionalError.has_value()) [[unlikely]] {
                return Error(optionalError.value(), pathToShaderSourceFile);
            }
#endif

            sFullSourceCode += sText;
            if (bAddNewLineAfterProcessingKeywordContent) {
                sFullSourceCode += "\n";
            }
            bFoundLanguageKeyword = true;

            return {};
        };

        // Prepare a lambda to process HLSL code.
        const auto processHlslCode = [&](std::string_view sKeyword,
                                         std::string& sText) -> std::optional<Error> {
            if (!bParseAsHlsl) {
                // Ignore this block.
                bFoundLanguageKeyword = true;
                return {};
            }

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
            // Process this block's content.
            auto optionalError = addHardcodedBindingIndexIfFound(bParseAsHlsl, sText, bindingIndicesInfo);
            if (optionalError.has_value()) [[unlikely]] {
                return Error(optionalError.value(), pathToShaderSourceFile);
            }
#endif

            sFullSourceCode += sText;
            if (bAddNewLineAfterProcessingKeywordContent) {
                sFullSourceCode += "\n";
            }
            bFoundLanguageKeyword = true;

            return {};
        };

        // See if we have a line with mixed keywords.
        bAddNewLineAfterProcessingKeywordContent = false;
        auto mixedLineResult = processMixedLanguageLine(
            sLineBuffer,
            pathToShaderSourceFile,
            [&](std::string_view sKeyword, std::string& sText) -> std::optional<Error> {
                if (sKeyword == sHlslKeyword) {
                    return processHlslCode(sKeyword, sText);
                }

                if (sKeyword == sGlslKeyword) {
                    return processGlslCode(sKeyword, sText);
                }

                if (sKeyword == sBothKeyword || sKeyword.empty()) {
                    sFullSourceCode += sText;
                    return {};
                }

                return Error(
                    std::format("unexpected keyword received \"{}\"", sKeyword), pathToShaderSourceFile);
            });
        if (std::holds_alternative<Error>(mixedLineResult)) [[unlikely]] {
            return std::get<Error>(std::move(mixedLineResult));
        }
        if (std::get<bool>(mixedLineResult)) {
            sFullSourceCode += "\n";
            continue;
        }
        bAddNewLineAfterProcessingKeywordContent = true;

        // Process GLSL keyword (if found).
        bFoundLanguageKeyword = false;
        optionalError =
            processKeywordCode({sGlslKeyword}, sLineBuffer, file, pathToShaderSourceFile, processGlslCode);
        if (optionalError.has_value()) [[unlikely]] {
            return std::move(optionalError.value());
        }
        if (bFoundLanguageKeyword) {
            continue;
        }

        // Process HLSL keyword (if found).
        bFoundLanguageKeyword = false;
        optionalError =
            processKeywordCode({sHlslKeyword}, sLineBuffer, file, pathToShaderSourceFile, processHlslCode);
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
#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
            // Detect hardcoded binding indices.
            auto optionalError =
                addHardcodedBindingIndexIfFound(bParseAsHlsl, sLineBuffer, bindingIndicesInfo);
            if (optionalError.has_value()) [[unlikely]] {
                return Error(optionalError.value(), pathToShaderSourceFile);
            }
#endif

            // Convert GLSL types to HLSL.
            if (bParseAsHlsl) {
                convertGlslTypesToHlslTypes(sLineBuffer);
            }

            // Append the line to the final source code string.
            sFullSourceCode += sLineBuffer + "\n";
            continue;
        }

        // Parse included file.
        auto result = parseFile(
            optionalIncludedPath.value(),
            bParseAsHlsl,
            bindingIndicesInfo,
            vFoundAdditionalShaderConstants,
            vAdditionalIncludeDirectories);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            return std::get<Error>(result);
        }
        sFullSourceCode += std::get<std::string>(std::move(result));
    }

    file.close();

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

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
std::optional<std::string> CombinedShaderLanguageParser::assignBindingIndices( // NOLINT: slightly complex
    bool bParseAsHlsl,
    std::string& sFullSourceCode,
    BindingIndicesInfo& bindingIndicesInfo,
    unsigned int iBaseAutomaticBindingIndex) {
    if (bParseAsHlsl) {
        // Prepare some variables.
        size_t iCurrentPos = 0;
        std::unordered_map<char, std::unordered_map<unsigned int, unsigned int>> nextFreeBindingIndex;

        // Initialize base binding indices.
        const std::array<char, 4> vRegisterTypes = {'t', 's', 'u', 'b'};
        for (const auto& registerType : vRegisterTypes) {
            auto& registerSpaceIndexIt = nextFreeBindingIndex[registerType];
            for (unsigned int i = 0; i < 9; i++) { // NOLINT: iterate over all spaces
                registerSpaceIndexIt[i] = 0;
            }
        }

        do {
            // Prepare some variables.
            bool bSkipCurrentRegister = false;
            char registerType = '0';
            unsigned int iRegisterSpace = 0; // default space if not specified
            size_t iRegisterIndexPositionToReplace = 0;

            // Find register values.
            auto optionalError = findHlslRegisterInfo(
                sFullSourceCode,
                iCurrentPos,
                [&]() -> std::optional<std::string> {
                    // Reached register type.
                    registerType = sFullSourceCode[iCurrentPos];

                    return {};
                },
                [&]() -> std::optional<std::string> {
                    // Reached register index.

                    // Make sure it's our keyword.
                    if (sFullSourceCode[iCurrentPos] != assignBindingIndexCharacter) {
                        // Found hardcoded index.
                        bSkipCurrentRegister = true;
                        return {};
                    }

                    // Save this position.
                    iRegisterIndexPositionToReplace = iCurrentPos;

                    return {};
                },
                [&]() -> std::optional<std::string> {
                    if (bSkipCurrentRegister) {
                        return {};
                    }

                    if (sFullSourceCode[iCurrentPos] == assignBindingIndexCharacter) [[unlikely]] {
                        return "`space?` is not supported";
                    }

                    // Reached register space.
                    auto readResult = readNumberFromString(sFullSourceCode, iCurrentPos);
                    if (std::holds_alternative<std::string>(readResult)) [[unlikely]] {
                        return std::get<std::string>(std::move(readResult));
                    }
                    iRegisterSpace = std::get<unsigned int>(readResult);

                    return {};
                });
            if (optionalError.has_value()) [[unlikely]] {
                return optionalError;
            }

            if (iCurrentPos == std::string::npos) {
                // Found nothing.
                return {};
            }

            if (bSkipCurrentRegister) {
                continue;
            }

            // Get space/index from free indices.
            const auto registerSpaceIndexIt = nextFreeBindingIndex.find(registerType);
            if (registerSpaceIndexIt == nextFreeBindingIndex.end()) [[unlikely]] {
                return std::format("found unexpected register type `{}`", registerType);
            }

            // Get free index.
            auto registerIndexIt = registerSpaceIndexIt->second.find(iRegisterSpace);
            if (registerIndexIt == registerSpaceIndexIt->second.end()) [[unlikely]] {
                return std::format("found unexpected register space {}", iRegisterSpace);
            }
            auto& iNextFreeRegisterIndex = registerIndexIt->second;

            // Make sure this index was not used before.
            const auto& usedIndices = bindingIndicesInfo.usedHlslIndices[registerType][iRegisterSpace];

            // Update our index to assign to unused (free) index.
            bool bFoundUnusedIndex = true;
            do {
                bFoundUnusedIndex = true;

                for (const auto iUsedIndex : usedIndices) {
                    if (iUsedIndex == iNextFreeRegisterIndex) {
                        iNextFreeRegisterIndex += 1;
                        bFoundUnusedIndex = false;
                        break;
                    }
                }
            } while (!bFoundUnusedIndex);

            // Replace our special character with this index.
            sFullSourceCode.erase(iRegisterIndexPositionToReplace, 1);
            sFullSourceCode.insert(iRegisterIndexPositionToReplace, std::to_string(iNextFreeRegisterIndex));

            // Update current position (because we might have changed string length).
            iCurrentPos = iRegisterIndexPositionToReplace;

            // Increment next free binding index (because the current one was assigned just now).
            iNextFreeRegisterIndex += 1;
        } while (iCurrentPos < sFullSourceCode.size());

        return {};
    }

    // Parse as GLSL.
    size_t iCurrentPos = 0;
    unsigned int iNextFreeBindingIndex = iBaseAutomaticBindingIndex;

    do {
        size_t iBindingIndexPositionToReplace = 0;
        bool bSkipThisBinding = false;

        auto optionalError =
            findGlslBindingIndex(sFullSourceCode, iCurrentPos, [&]() -> std::optional<std::string> {
                // Make sure this is our special assignment character.
                if (sFullSourceCode[iCurrentPos] != assignBindingIndexCharacter) {
                    bSkipThisBinding = true;
                    return {};
                }

                // Remember this position.
                iBindingIndexPositionToReplace = iCurrentPos;

                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError;
        }

        if (iCurrentPos == std::string::npos) {
            // Found nothing.
            return {};
        }

        if (bSkipThisBinding) {
            continue;
        }

        // Make sure our index is not used.
        while (bindingIndicesInfo.usedGlslIndices.find(iNextFreeBindingIndex) !=
               bindingIndicesInfo.usedGlslIndices.end()) {
            iNextFreeBindingIndex += 1;
        }

        // Replace our special character with this index.
        sFullSourceCode.erase(iBindingIndexPositionToReplace, 1);
        sFullSourceCode.insert(iBindingIndexPositionToReplace, std::to_string(iNextFreeBindingIndex));

        // Update current position (because we might have changed string length).
        iCurrentPos = iBindingIndexPositionToReplace;

        // Increment next free binding index (because the current one was assigned just now).
        iNextFreeBindingIndex += 1;
    } while (iCurrentPos < sFullSourceCode.size());

    return {};
}
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
std::optional<std::string> CombinedShaderLanguageParser::findHlslRegisterInfo(
    std::string& sSourceCode,
    size_t& iCurrentPos,
    const std::function<std::optional<std::string>()>& onReachedRegisterType,
    const std::function<std::optional<std::string>()>& onReachedRegisterIndex,
    const std::function<std::optional<std::string>()>& onReachedRegisterSpaceIndex) {
    // Find binding keyword.
    iCurrentPos = sSourceCode.find(sHlslBindingKeyword, iCurrentPos);
    if (iCurrentPos == std::string::npos) {
        return {};
    }

    // Jump to binding value.
    iCurrentPos += sHlslBindingKeyword.size();

    // Skip spaces (if found).
    for (; iCurrentPos < sSourceCode.size(); iCurrentPos++) {
        if (sSourceCode[iCurrentPos] != ' ') {
            break;
        }
    }
    if (iCurrentPos >= sSourceCode.size()) [[unlikely]] {
        return std::format("found \"{}\" but not found register type", sHlslBindingKeyword);
    }

    // Reached register type.
    auto optionalError = onReachedRegisterType();
    if (optionalError.has_value()) [[unlikely]] {
        return optionalError;
    }

    // Check if user wants to quit.
    if (iCurrentPos == std::string::npos) {
        return {};
    }
    iCurrentPos += 1;

    // Skip spaces (if found).
    for (; iCurrentPos < sSourceCode.size(); iCurrentPos++) {
        if (sSourceCode[iCurrentPos] != ' ') {
            break;
        }
    }
    if (iCurrentPos >= sSourceCode.size()) [[unlikely]] {
        return "found register type but no register index";
    }

    // Reached register index.
    optionalError = onReachedRegisterIndex();
    if (optionalError.has_value()) [[unlikely]] {
        return optionalError;
    }

    // Check if user wants to quit.
    if (iCurrentPos == std::string::npos) {
        return {};
    }

    // Find closing bracket position.
    const auto iClosingBracketPosition = sSourceCode.find(')', iCurrentPos);

    // Find register space keyword.
    const auto iRegisterSpacePos = sSourceCode.find(sHlslRegisterSpaceKeyword.data(), iCurrentPos);
    if (iRegisterSpacePos != std::string::npos &&
        iRegisterSpacePos < iClosingBracketPosition) { // Make sure this register space is for our register
                                                       // (not some next register).
        iCurrentPos = iRegisterSpacePos + sHlslRegisterSpaceKeyword.size();
        optionalError = onReachedRegisterSpaceIndex();
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError;
        }

        // Check if user wants to quit.
        if (iCurrentPos == std::string::npos) {
            return {};
        }
    }

    iCurrentPos = iClosingBracketPosition;

    return {};
}
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
std::optional<std::string> CombinedShaderLanguageParser::findGlslBindingIndex(
    std::string& sSourceCode,
    size_t& iCurrentPos,
    const std::function<std::optional<std::string>()>& onReachedBindingIndex) {
    // Find binding keyword.
    iCurrentPos = sSourceCode.find(sGlslBindingKeyword, iCurrentPos);
    if (iCurrentPos == std::string::npos) {
        return {};
    }

    // Jump to keyword end.
    iCurrentPos += sGlslBindingKeyword.size();

    // Go forward until `=` is found.
    for (; iCurrentPos < sSourceCode.size(); iCurrentPos++) {
        if (sSourceCode[iCurrentPos] == '=') {
            break;
        }
    }
    if (iCurrentPos >= sSourceCode.size()) [[unlikely]] {
        return std::format("found \"{}\" but not found `=` after it", sGlslBindingKeyword);
    }

    // Skip `=`.
    iCurrentPos += 1;

    // Go forward until a non-space character is found.
    for (; iCurrentPos < sSourceCode.size(); iCurrentPos++) {
        if (sSourceCode[iCurrentPos] != ' ') {
            break;
        }
    }
    if (iCurrentPos >= sSourceCode.size()) [[unlikely]] {
        return std::format("found \"{}\" but not found binding index after it", sGlslBindingKeyword);
    }

    onReachedBindingIndex();

    return {};
}
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
std::optional<std::string> CombinedShaderLanguageParser::addHardcodedBindingIndexIfFound(
    bool bParseAsHlsl, std::string& sCodeLine, BindingIndicesInfo& bindingIndicesInfo) {
    if (bParseAsHlsl) {
        // Prepare some variables.
        size_t iCurrentPos = 0;
        char registerType = '0';
        unsigned int iRegisterIndex = 0;
        unsigned int iRegisterSpace = 0; // default space if not specified

        // Find register values.
        auto optionalError = findHlslRegisterInfo(
            sCodeLine,
            iCurrentPos,
            [&]() -> std::optional<std::string> {
                // Reached register type.
                registerType = sCodeLine[iCurrentPos];

                return {};
            },
            [&]() -> std::optional<std::string> {
                // Reached register index.

                // Make sure it's not our keyword.
                if (sCodeLine[iCurrentPos] == assignBindingIndexCharacter) {
                    // Found our keyword, skip this line to assign a binding index later.
                    iCurrentPos = std::string::npos;
                    bindingIndicesInfo.bFoundBindingIndicesToAssign = true;
                    return {};
                }

                // Read index.
                auto readResult = readNumberFromString(sCodeLine, iCurrentPos);
                if (std::holds_alternative<std::string>(readResult)) [[unlikely]] {
                    return std::get<std::string>(std::move(readResult));
                }
                iRegisterIndex = std::get<unsigned int>(readResult);

                return {};
            },
            [&]() -> std::optional<std::string> {
                if (sCodeLine[iCurrentPos] == assignBindingIndexCharacter) [[unlikely]] {
                    return "`space?` is not supported";
                }

                // Reached register space.
                auto readResult = readNumberFromString(sCodeLine, iCurrentPos);
                if (std::holds_alternative<std::string>(readResult)) [[unlikely]] {
                    return std::get<std::string>(std::move(readResult));
                }
                iRegisterSpace = std::get<unsigned int>(readResult);

                return {};
            });
        if (optionalError.has_value()) [[unlikely]] {
            return optionalError;
        }

        if (iCurrentPos == std::string::npos) {
            // Found nothing.
            return {};
        }

        // Don't check if register was already specified or not because some includes might be
        // hidden behind #ifdef which we don't expand.

        // Add index as used.
        bindingIndicesInfo.usedHlslIndices[registerType][iRegisterSpace].insert(iRegisterIndex);

        return {};
    }

    // Parse as GLSL.

    size_t iCurrentPos = 0;
    auto optionalError = findGlslBindingIndex(sCodeLine, iCurrentPos, [&]() -> std::optional<std::string> {
        // Check for our special assignment character.
        if (sCodeLine[iCurrentPos] == assignBindingIndexCharacter) {
            bindingIndicesInfo.bFoundBindingIndicesToAssign = true;
            return {};
        }

        // Found hardcoded index.

        // Read index value.
        auto readResult = readNumberFromString(sCodeLine, iCurrentPos);
        if (std::holds_alternative<std::string>(readResult)) [[unlikely]] {
            return std::get<std::string>(std::move(readResult));
        }
        const auto iHardcodedBindingIndex = std::get<unsigned int>(readResult);

        // Don't check if index was already specified or not because some includes might be
        // hidden behind #ifdef which we don't expand.

        // Add index as used.
        bindingIndicesInfo.usedGlslIndices.insert(iHardcodedBindingIndex);

        return {};
    });
    if (optionalError.has_value()) [[unlikely]] {
        return optionalError;
    }

    if (iCurrentPos == std::string::npos) {
        // Found nothing.
        return {};
    }

    return {};
}
#endif

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

std::variant<unsigned int, std::string>
CombinedShaderLanguageParser::readNumberFromString(const std::string& sText, size_t iReadStartPosition) {
    // Read value.
    std::string sValue;
    for (size_t i = iReadStartPosition; i < sText.size(); i++) {
        if (std::isdigit(static_cast<unsigned char>(sText[i])) == 0) {
            break;
        }
        sValue += sText[i];
    }

    // Make sure it's not empty.
    if (sValue.empty()) [[unlikely]] {
        return "no digit was found";
    }

    // Convert to integer.
    unsigned int iValue = 0;
    try {
        iValue = static_cast<unsigned int>(std::stoul(sValue));
    } catch (const std::exception& exception) {
        return std::format("failed to convert string \"{}\" to integer, error: {}", sValue, exception.what());
    }

    return iValue;
}

std::variant<std::optional<std::filesystem::path>, CombinedShaderLanguageParser::Error>
CombinedShaderLanguageParser::findIncludePath(
    std::string& sLineBuffer,
    const std::filesystem::path& pathToShaderSourceFile,
    const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories) {
    // Look for the include keyword.
    const auto iIncludeKeywordStartPos = sLineBuffer.find(sIncludeKeyword);
    if (iIncludeKeywordStartPos == std::string::npos) {
        return {};
    }

    // Remove the include keyword from the buffer.
    sLineBuffer.erase(0, iIncludeKeywordStartPos + sIncludeKeyword.size());

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

    // The second character should be `"`.
    if (sLineBuffer[1] != '\"') [[unlikely]] {
        return Error(
            std::format(
                "expected to find open quote in the beginning of the included path on line \"{}\"",
                sLineBuffer),
            pathToShaderSourceFile);
    }

    // Erase a space and a quote.
    sLineBuffer.erase(0, 2);

    // Now find closing quote.
    const auto iClosingQuotePos = sLineBuffer.find('"');
    if (iClosingQuotePos == std::string::npos) [[unlikely]] {
        return Error(
            std::format("expected to find a closing quote in the included path on line \"{}\"", sLineBuffer),
            pathToShaderSourceFile);
    }

    // Cut included path.
    const auto sIncludedPath = sLineBuffer.substr(0, iClosingQuotePos);

    // Now the line buffer only has a relative include path.
    // Build a path to the included file.
    auto pathToIncludedFile = pathToShaderSourceFile.parent_path() / sIncludedPath;
    if (!std::filesystem::exists(pathToIncludedFile)) {
        // Attempt to use additional include directories.
        bool bFoundFilePath = false;
        for (const auto& pathToDirectory : vAdditionalIncludeDirectories) {
            // Construct new path.
            pathToIncludedFile = pathToDirectory / sIncludedPath;

            // Check if it exists.
            if (std::filesystem::exists(pathToIncludedFile)) {
                bFoundFilePath = true;
                break;
            }
        }

        // Make sure we found an existing path.
        if (!bFoundFilePath) [[unlikely]] {
            return Error(
                std::format("unable to find included file \"{}\"", sIncludedPath), pathToShaderSourceFile);
        }
    }

    return pathToIncludedFile;
}

std::optional<CombinedShaderLanguageParser::Error> CombinedShaderLanguageParser::finalizeParsingResults(
    const std::filesystem::path& pathToShaderSourceFile,
    bool bParseAsHlsl,
    BindingIndicesInfo& bindingIndicesInfo,
    std::string& sFullParsedSourceCode,
    std::vector<std::string>& vAdditionalShaderConstants,
    unsigned int iBaseAutomaticBindingIndex) {
#if defined(ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD)
    // Now insert additional shader constants.
    if (!vAdditionalShaderConstants.empty()) {
        // Find where push constants start.
        size_t iShaderConstantsStartPos = 0;
        if (bParseAsHlsl) {
            iShaderConstantsStartPos = sFullParsedSourceCode.find("struct RootConstants");
        } else {
            iShaderConstantsStartPos = sFullParsedSourceCode.find("layout(push_constant)");
        }
        if (iShaderConstantsStartPos == std::string::npos) [[unlikely]] {
            return Error(
                "additional push constants were found and includes of the file were processed "
                "but initial push constants layout was not found in the included files",
                sFullParsedSourceCode);
        }

        // Look for '}' after shader constants.
        const auto iShaderConstantsEndPos = sFullParsedSourceCode.find('}', iShaderConstantsStartPos);
        if (iShaderConstantsEndPos == std::string::npos) [[unlikely]] {
            return Error(
                "expected to find a closing bracket after push constants definition", pathToShaderSourceFile);
        }

        const size_t iAdditionalShaderConstantsInsertPos = iShaderConstantsEndPos;

        // Insert additional push constants (insert in reserve order because of how `std::string::insert`
        // below works, to make the resulting order is correct).
        for (auto reverseIt = vAdditionalShaderConstants.rbegin();
             reverseIt != vAdditionalShaderConstants.rend();
             ++reverseIt) {
            auto& sPushConstant = *reverseIt;
            if (!sPushConstant.ends_with('\n')) {
                sPushConstant += '\n';
            }
            sFullParsedSourceCode.insert(iAdditionalShaderConstantsInsertPos, sPushConstant);
        }
    }
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
    if (bindingIndicesInfo.bFoundBindingIndicesToAssign) {
        // Assign binding indices.
        auto optionalError = assignBindingIndices(
            bParseAsHlsl, sFullParsedSourceCode, bindingIndicesInfo, iBaseAutomaticBindingIndex);
        if (optionalError.has_value()) [[unlikely]] {
            return Error(optionalError.value(), pathToShaderSourceFile);
        }
    }
#endif

    return {};
}
