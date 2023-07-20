#pragma once

// Standard.
#include <filesystem>
#include <string>
#include <variant>

/*
LICENCE
MIT License

Copyright (c) [2018] [Tahar Meijs]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * Provides static functions for reading shader source file and replacing `#include "..."` keywords
 * with included file's contents.
 */
class ShaderIncluder {
public:
    /** Types of errors. */
    enum class Error {
        CANT_OPEN_FILE,          //< The specified path or some included path cannot be opened.
        PATH_IS_NOT_A_FILE,      //< The specified path or some included path is not a file.
        PATH_HAS_NO_PARENT_PATH, //< `std::filesystem::has_parent_path` returned `false` on some path.
        NOTHING_AFTER_INCLUDE,   //< A file has `#include` and nothing else on the same line.
        NO_SPACE_AFTER_KEYWORD,  //< A file has `#include` and no space after this keyword.
        MISSING_QUOTES,          //< A file has `#include` keyword with missing double quotes.
    };

    /**
     * Reads text from the specified file and recursively replaces all `#include "..."` keywords with
     * included file's contents.
     *
     * @param pathToShaderSourceFile Path to shader source file that should be read.
     *
     * @return Error if something went wrong, otherwise contents of the specified file with all
     * `#include "..."` keywords replaced with included file's contents.
     */
    static std::variant<std::string, Error>
    parseFullSourceCode(const std::filesystem::path& pathToShaderSourceFile);
};
