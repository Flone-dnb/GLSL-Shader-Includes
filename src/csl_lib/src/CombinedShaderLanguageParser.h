#pragma once

// Standard.
#include <filesystem>
#include <string>
#include <variant>
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
        Error(const std::string& sErrorMessage, const std::filesystem::path& pathToErrorFile) {
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
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, otherwise full (combined) source code.
     */
    static std::variant<std::string, Error> parseGlsl(
        const std::filesystem::path& pathToShaderSourceFile,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories = {});

private:
    /** Groups next available resource binding index to assign. */
    struct NextBindingIndex {
        /** Binding index for GLSL resources. */
        unsigned int iGlslIndex = 0;

        /**
         * Binding indices for HLSL resources.
         * Stores pairs of "register type" - ["register space" - "binding index"].
         */
        std::unordered_map<char, std::unordered_map<unsigned int, unsigned int>> hlslIndex;

        /** `true` if non-parser assigned binding index was found. */
        bool bFoundHardcodedIndex = false;
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
     * @param sKeyword               Keyword to look for, for example: `#hlsl`.
     * @param sLineBuffer            Current line from the file.
     * @param file                   File to read additional lines if additional push constants were found.
     * @param pathToShaderSourceFile Path to file being processed.
     * @param processContent         Callback with text (whole file line or line after keyword).
     *
     * @return Error if something went wrong.
     */
    static std::optional<Error> processKeywordCode(
        std::string_view sKeyword,
        std::string& sLineBuffer,
        std::ifstream& file,
        const std::filesystem::path& pathToShaderSourceFile,
        const std::function<std::optional<Error>(std::string&)>& processContent);

    /**
     * Parses the specified file.
     *
     * @param pathToShaderSourceFile        Path to the file to parse.
     * @param bParseAsHlsl                  Whether to parse as HLSL or as GLSL.
     * @param nextBindingIndex              Stores next available binding index for shader resources.
     * @param vAdditionalIncludeDirectories Paths to directories in which included files can be found.
     *
     * @return Error if something went wrong, otherwise parsed source code.
     */
    static std::variant<std::string, Error> parse(
        const std::filesystem::path& pathToShaderSourceFile,
        bool bParseAsHlsl,
        NextBindingIndex& nextBindingIndex,
        const std::vector<std::filesystem::path>& vAdditionalIncludeDirectories = {});

    /**
     * Modifies the input string with GLSL types replaced to HLSL types (for example `vec3` to `float3`).
     *
     * @param sGlslLine Line of GLSL code.
     */
    static void convertGlslTypesToHlslTypes(std::string& sGlslLine);

    /**
     * Modifies the input string with binding indices assigned (if there are any bindings declared in this
     * line).
     *
     * @param sGlslLine        Line of GLSL code.
     * @param nextBindingIndex Stores next available binding index for shader resources.
     *
     * @return Error message if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string>
    assignGlslBindingIndexIfFound(std::string& sGlslLine, NextBindingIndex& nextBindingIndex);

    /**
     * Modifies the input string with binding indices assigned (if there are any bindings declared in this
     * line).
     *
     * @param sHlslLine        Line of HLSL code.
     * @param nextBindingIndex Stores next available binding index for shader resources.
     *
     * @return Error message if something went wrong.
     */
    [[nodiscard]] static std::optional<std::string>
    assignHlslBindingIndexIfFound(std::string& sHlslLine, NextBindingIndex& nextBindingIndex);

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

    /** Keyword used to include other files. */
    static constexpr std::string_view sIncludeKeyword = "#include";

    /** Keyword used to tell the parser that it needs to assign a binding index. */
    static constexpr std::string_view sAssignBindingIndexKeyword = "?";

#if defined(ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD)
    /**
     * Keyword used to specify variables that should be appended to the actual push constants struct
     * located in a separate file.
     */
    static inline const std::string sAdditionalPushConstantsKeyword = "#additional_push_constants";
#endif
};
