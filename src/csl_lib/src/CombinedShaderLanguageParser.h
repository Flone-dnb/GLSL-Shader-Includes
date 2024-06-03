#pragma once

// Standard.
#include <filesystem>
#include <string>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>

/** Parser. */
class CombinedShaderLanguageParser {
public:
    /** Groups error information. */
    struct Error {
        Error() = delete;

        /**
         * Initializes error.
         *
         * @param sErrorMessage   Full error message.
         * @param pathToErrorFile Path to the file that caused the error.
         */
        explicit Error(const std::string& sErrorMessage, const std::filesystem::path& pathToErrorFile) {
            this->sErrorMessage = sErrorMessage;
            this->pathToErrorFile = pathToErrorFile;
        }

        /** Full error message. */
        std::string sErrorMessage;

        /** Path to the file that caused the error. */
        std::filesystem::path pathToErrorFile;
    };

    /**
     * Parses the specified file as HLSL code (`#glsl` blocks are ignored and not included).
     *
     * @param pathToShaderSourceFile        Path to the file to process.
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, otherwise full (combined) source code.
     */
    static std::variant<std::string, Error> parseHlsl(
        const std::filesystem::path& pathToShaderSourceFile,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories = {});

    /**
     * Parses the specified file as GLSL code (`#hlsl` blocks are ignored and not included).
     *
     * @param pathToShaderSourceFile        Path to the file to process.
     * @param iBaseAutomaticBindingIndex    If you use `?` character to ask the parser to specify automatic
     * free (unused) binding indices, this value will be used as the smallest (starting) auto-generated
     * binding index counter so that all parser-generated binding indices will be equal or bigger than this
     * value. Using this index you can (for example) compile your vertex shaders with 0 base index and
     * fragment shaders with 100 index to guarantee that indices of shaders will be unique to combine
     * them later together in your renderer.
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, otherwise full (combined) source code.
     */
    static std::variant<std::string, Error> parseGlsl(
        const std::filesystem::path& pathToShaderSourceFile,
        unsigned int iBaseAutomaticBindingIndex = 0,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories = {});

private:
    /** Groups next available resource binding index to assign. */
    struct BindingIndicesInfo {
        /** Used (hardcoded) binding indices that were found while parsing existing GLSL code. */
        std::unordered_set<unsigned int> usedGlslIndices;

        /**
         * Used (hardcoded) binding indices that were found while parsing existing HLSL code.
         * Stores pairs of "register type" - ["register space" - "used binding indices"].
         */
        std::unordered_map<char, std::unordered_map<unsigned int, std::unordered_set<unsigned int>>>
            usedHlslIndices;

        /** `true` if a special keyword to insert a random free binding index was found, `false` otherwise. */
        bool bFoundBindingIndicesToAssign = false;
    };

    /**
     * Looks for the specified keyword in the specified line and calls your callback to process code
     * after keyword (i.e. body).
     *
     * @remark Will process various blocks/lines of keyword such as:
     * @code
     * #keyword CODE   // single line
     *
     * #keyword{       // curly bracket on the same line (block)
     *     CODE
     * }
     *
     * #keyword        // curly bracket on the next line (block)
     * {
     *     CODE
     * }
     * @endcode
     *
     * @param vKeywords              Different variants of the keyword to look for, for example: `#hlsl`.
     * @param sLineBuffer            Current line from the file.
     * @param file                   File to read additional lines if additional push constants were found.
     * @param pathToShaderSourceFile Path to file being processed.
     * @param processContent         Callback with text (whole file line or line after keyword) and a keyword
     * that was found.
     *
     * @return Error if something went wrong.
     */
    static std::optional<Error> processKeywordCode(
        const std::vector<std::string_view>& vKeywords,
        std::string& sLineBuffer,
        std::ifstream& file,
        const std::filesystem::path& pathToShaderSourceFile,
        const std::function<std::optional<Error>(std::string_view sKeyword, std::string& sText)>&
            processContent);

    /**
     * Parses a line of code that contains 2 or 3 keywords: HLSL, GLSL and a keyword for both languages,
     * then triggers a callback on the content of each keyword.
     *
     * @param sLineBuffer            Line of code.
     * @param pathToShaderSourceFile Path to file being processed.
     * @param processContent         Callback that will be given the code after a keyword
     * (empty keyword means that there is code before keywords).
     *
     * @return `false` if this line does not contain required keywords, `true` if contains and was processed
     * without errors, otherwise an error.
     */
    static std::variant<bool, Error> processMixedLanguageLine(
        std::string& sLineBuffer,
        const std::filesystem::path& pathToShaderSourceFile,
        const std::function<std::optional<Error>(std::string_view sKeyword, std::string& sText)>&
            processContent);

    /**
     * Starts parsing using the specified path.
     *
     * @param pathToShaderSourceFile        Path to the file to process.
     * @param bParseAsHlsl                  Whether to parse as HLSL or as GLSL.
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     * @param iBaseAutomaticBindingIndex    Used only if parsing as GLSL. If you use `?` character to ask the
     * parser to specify automatic free (unused) binding indices, this value will be used as the smallest
     * (starting) auto-generated binding index counter so that all parser-generated binding indices will be
     * equal or bigger than this value.
     *
     * @return Error if something went wrong, otherwise full (combined) source code.
     */
    static std::variant<std::string, Error> runParsing(
        const std::filesystem::path& pathToShaderSourceFile,
        bool bParseAsHlsl,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories,
        unsigned int iBaseAutomaticBindingIndex = 0);

    /**
     * Parses the specified file.
     *
     * @param pathToShaderSourceFile          Path to the file to parseFile.
     * @param bParseAsHlsl                    Whether to parseFile as HLSL or as GLSL.
     * @param bindingIndicesInfo              Information about binding indices.
     * @param vFoundAdditionalShaderConstants Additional shaders constants that were found during parsing.
     * @param vAdditionalIncludeDirectories   Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, otherwise parsed source code.
     */
    static std::variant<std::string, Error> parseFile(
        const std::filesystem::path& pathToShaderSourceFile,
        bool bParseAsHlsl,
        BindingIndicesInfo& bindingIndicesInfo,
        std::vector<std::string>& vFoundAdditionalShaderConstants,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories);

    /**
     * Called after a file and all of its includes were parsed to do final parsing logic.
     *
     * @param pathToShaderSourceFile     Path to the file to parse.
     * @param bParseAsHlsl               Whether to parse as HLSL or as GLSL.
     * @param bindingIndicesInfo         Information about binding indices.
     * @param sFullParsedSourceCode      Parsed source code.
     * @param vAdditionalShaderConstants Additional push constants that were found during parsing.
     * @param iBaseAutomaticBindingIndex Used only if parsing as GLSL. If you use `?` character to ask the
     * parser to specify automatic free (unused) binding indices, this value will be used as the smallest
     * (starting) auto-generated binding index counter so that all parser-generated binding indices will be
     * equal or bigger than this value.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<Error> finalizeParsingResults(
        const std::filesystem::path& pathToShaderSourceFile,
        bool bParseAsHlsl,
        BindingIndicesInfo& bindingIndicesInfo,
        std::string& sFullParsedSourceCode,
        std::vector<std::string>& vAdditionalShaderConstants,
        unsigned int iBaseAutomaticBindingIndex = 0);

    /**
     * Modifies the input string with GLSL types replaced to HLSL types (for example `vec3` to `float3`).
     *
     * @param sGlslLine Line of GLSL code.
     */
    static void convertGlslTypesToHlslTypes(std::string& sGlslLine);

    /**
     * Modifies the input string with HLSL types replaced to GLSL types (for example `mul` to operator*).
     *
     * @param sHlslLine Line of HLSL code.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> convertHlslTypesToGlslTypes(std::string& sHlslLine);

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
    /**
     * Replaces all occurrences of @ref sAssignBindingIndexKeyword with an unused binding index.
     *
     * @param bParseAsHlsl       `true` to treat the specified code line as HLSL, `false` as GLSL.
     * @param sFullSourceCode    Full source code (may contain multiple lines) to scan for keywords.
     * @param bindingIndicesInfo Information about used (hardcoded) binding indices.
     * @param iBaseAutomaticBindingIndex Used only if parsing as GLSL. If you use `?` character to ask the
     * parser to specify automatic free (unused) binding indices, this value will be used as the smallest
     * (starting) auto-generated binding index counter so that all parser-generated binding indices will be
     * equal or bigger than this value.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> assignBindingIndices(
        bool bParseAsHlsl,
        std::string& sFullSourceCode,
        BindingIndicesInfo& bindingIndicesInfo,
        unsigned int iBaseAutomaticBindingIndex = 0);
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
    /**
     * Looks if there is a hardcoded binding index and adds it to the specified binding indices info.
     *
     * @param bParseAsHlsl       `true` to treat the specified code line as HLSL, `false` as GLSL.
     * @param sCodeLine          Line of code.
     * @param bindingIndicesInfo Information about binding indices.
     *
     * @return Error message if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> addHardcodedBindingIndexIfFound(
        bool bParseAsHlsl, std::string& sCodeLine, BindingIndicesInfo& bindingIndicesInfo);
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
    /**
     * Increments the specified position counter in the specified source code string
     * until reached HLSL register type, index and optionally space index.
     *
     * @warning Do not erase source code parts or replace them with text that has less characters in
     * callbacks.
     *
     * @warning Do not modify current position in callbacks, if you need to quit the function and stop
     * set `npos` to position.
     *
     * @param sSourceCode                 Source code to process (may contain multiple lines of code).
     * @param iCurrentPos                 Position to start searching from. Will be incremented or changed to
     * `npos` if found nothing.
     * @param onReachedRegisterType       Called after current position reached register type (`b`, `s`, `t`,
     * etc.).
     * @param onReachedRegisterIndex      Called after current position reached register index.
     * @param onReachedRegisterSpaceIndex Called after current position reached register space index (digit).
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> findHlslRegisterInfo(
        std::string& sSourceCode,
        size_t& iCurrentPos,
        const std::function<std::optional<std::string>()>& onReachedRegisterType,
        const std::function<std::optional<std::string>()>& onReachedRegisterIndex,
        const std::function<std::optional<std::string>()>& onReachedRegisterSpaceIndex);
#endif

#if defined(ENABLE_AUTOMATIC_BINDING_INDICES)
    /**
     * Increments the specified position counter in the specified source code string
     * until reached GLSL binding index.
     *
     * @param sSourceCode           Source code to process (may contain multiple lines of code).
     * @param iCurrentPos           Position to start searching from. Will be incremented or changed to
     * `npos` if found nothing.
     * @param onReachedBindingIndex Called after current position reached binding index value.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> findGlslBindingIndex(
        std::string& sSourceCode,
        size_t& iCurrentPos,
        const std::function<std::optional<std::string>()>& onReachedBindingIndex);
#endif

    /**
     * Replaces all occurrences of the specified "replace from" text to "replace to" text.
     *
     * @param sText        Text to modify.
     * @param sReplaceFrom Text to replace.
     * @param sReplaceTo   Text to place instead of replaced.
     */
    static void
    replaceSubstring(std::string& sText, std::string_view sReplaceFrom, std::string_view sReplaceTo);

    /**
     * Looks for `mul` in the specified code and replaces it with GLSL operator*.
     *
     * @param sHlslCode Line of HLSL code.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string> replaceHlslMulToGlsl(std::string& sHlslCode);

    /**
     * Reads digits from the specified position of the specified string until non-digit character is found.
     *
     * @param sText              Text to read digits from.
     * @param iReadStartPosition Position in the text to start reading digits.
     *
     * @return Error message if something went wrong, otherwise read number.
     */
    static std::variant<unsigned int, std::string>
    readNumberFromString(const std::string& sText, size_t iReadStartPosition);

    /**
     * Looks for `#include` keyword and if found returns the path it contains.
     *
     * @param sLineBuffer                   Line of code.
     * @param pathToShaderSourceFile        File currently being processed.
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, empty if keyword was not found, otherwise included path
     * that exists.
     */
    static std::variant<std::optional<std::filesystem::path>, Error> findIncludePath(
        std::string& sLineBuffer,
        const std::filesystem::path& pathToShaderSourceFile,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories = {});

    /** Keyword used to specify GLSL code block/line. */
    static constexpr std::string_view sGlslKeyword = "#glsl";

    /** Keyword used to specify HLSL code block/line. */
    static constexpr std::string_view sHlslKeyword = "#hlsl";

    /** Keyword used to specify code for both GLSL and HLSL languages. */
    static constexpr std::string_view sBothKeyword = "#both";

    /** Keyword used to include other files. */
    static constexpr std::string_view sIncludeKeyword = "#include";

    /** Keyword character used to tell the parser that it needs to assign a binding index. */
    static constexpr char assignBindingIndexCharacter = '?';

    /** GLSL keyword used to specify shader resource binding index. */
    static constexpr std::string_view sGlslBindingKeyword = "binding";

    /** HLSL keyword used to specify shader resource binding index. */
    static constexpr std::string_view sHlslBindingKeyword = "register(";

    /** HLSL keyword used to specify shader resource binding space. */
    static constexpr std::string_view sHlslRegisterSpaceKeyword = "space";

#if defined(ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD)
    /**
     * Keyword used to specify variables that should be appended to the actual push constants struct
     * located in a separate file.
     */
    static inline const std::string sAdditionalPushConstantsKeyword = "#additional_push_constants";

    /**
     * Keyword used to specify variables that should be appended to the actual root constants struct
     * located in a separate file.
     */
    static inline const std::string sAdditionalRootConstantsKeyword = "#additional_root_constants";

    /**
     * Keyword used to specify variables that should be appended to the actual push/root constants struct
     * located in a separate file.
     */
    static inline const std::string sAdditionalShaderConstantsKeyword = "#additional_shader_constants";
#endif
};
