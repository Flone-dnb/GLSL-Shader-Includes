# Combined Shader Language Parser

Since HLSL and GLSL are kind of similar, using this simple parser you can avoid duplicating shader code if you need to write both HLSL and GLSL shaders, here is a small example of what this parser does:

```
// input file

#hlsl cbuffer frameData : register(b0, space5){
#glsl layout(binding = 0) uniform frameData{
    mat4 viewProjectionMatrix;
    vec3 cameraPosition;
};
```

when `parseHlsl` is used you will get the following code:

```
cbuffer frameData : register(b0, space5){   // `#glsl` content was removed
    float4x4 viewProjectionMatrix;          // `mat4` was converted to `float4x4`
    float3 cameraPosition;                  // `vec3` was converted to `float3`
};
```

when `parseGlsl` is used you will get the following code:

```
layout(binding = 0) uniform frameData{     // `#hlsl` content was removed
    mat4 viewProjectionMatrix;
    vec3 cameraPosition;
};
```

Here are all forms of a language block:

```
#keyword // code (single line)

#keyword{
    // code (block)
}

#keyword
{
    // code (block)
}
```

Note:

- This parser does not have a fixed file extension and it just processes the file you give to it.
- This parser should be run to read the file from disk to then pass parsed text to your shader compiler.

What this parser does:

- Replaces `#include "relative/path"` with included file contents - might be useful in GLSL.
- Allows to specify additional include directories before parsing.
- Appends variables from blocks marked as `#additional_push_constants` to the initial (probably included) push constants layout definition to allow "extending" push constants from other files - might be useful in Vulkan applications.
- Has two modes: `parseHlsl` and `parseGlsl`:
    - `parseHlsl` parses the specified file and replaces simple GLSL types (not all, such as `vec3` or `mat3`) to HLSL types (such as `float3` and `float3x3`) in the memory while reading (source file will not be changed) - allows you to have 1 shader written with GLSL types and process it in both DirectX and Vulkan/OpenGL.
    - `parseGlsl` just parses the file as usual (no type conversion will be applied).
- Allows to specify `#hlsl` and `#glsl` block of code in your shader source file that will be processed differently in different modes:
    - When `parseHlsl` meets `#hlsl` block of code it removes the `#hlsl` keyword (in the memory while reading) and just reads the code that was specified in the `#hlsl` block without doing any type conversions.
    - When `parseHlsl` meets `#glsl` block of code it removes the keyword and all code from this block (in the memory while reading).
    - When `parseGlsl` meets `#hlsl` block of code it removes the keyword and all code from this block (in the memory while reading).
    - When `parseGlsl` meets `#glsl` block of code it removes the `#glsl` keyword (in the memory while reading) and just reads the code that was specified in the `#glsl` block without doing any type conversions.

As you can see from the list above this parser does some GLSL -> HLSL conversions but not the other way. Keep that in mind.

# Using this project

In your cmake file:

```cmake
set(CSL_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(CSL_ENABLE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(CSL_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD OFF CACHE BOOL "" FORCE) // optional
add_subdirectory(<some path here>/combined-shader-language-parser SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC CombinedShaderLanguageParserLib)
```

In your C++ code:

```cpp
#include "CombinedShaderLanguageParser.h"

auto result = CombinedShaderLanguageParser::parseGlsl("path/to/myfile.glsl");
// or                                       parseHlsl("path/to/myfile.glsl");
if (std::holds_alternative<CombinedShaderLanguageParser::Error>(result)){
    // handle error
    return;
}
const auto sFullSourceCode = std::get<std::string>(std::move(result));
```

## Optional features

`CSL_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD` is used to enable `#additional_push_constants` keyword which is used to append variables to a push constants struct (located in a separate shader file), for example:

```GLSL
// ----------------- SomePushConstants.glsl -----------------

layout(push_constant) uniform Indices
{
   uint arrayIndex;
} indices;

// ----------------- myfile.glsl -----------------

#include "SomePushConstants.glsl"

#additional_push_constants
{
    uint someIndex;
}

// ----------------- resulting myfile.glsl -----------------

layout(push_constant) uniform Indices
{
    uint arrayIndex;
    uint someIndex;
} indices;

```

# Building the project for development

Please note the instructions below are only needed if you want to modify this project, if you just want to use this project in your own project you just need to `add_subdirectory` and `target_link_libraries` in your project.

Prerequisites:

- [CMake](https://cmake.org/download/)
- [Doxygen](https://doxygen.nl/download.html)
- [LLVM](https://github.com/llvm/llvm-project/releases/latest)

First, clone this repository:

```
git clone <project URL>
cd <project directory name>
git submodule update --init --recursive
```

Then, if you've never used CMake before:

Create a `build` directory next to this file, open created `build` directory and type `cmd` in Explorer's address bar. This will open up a console in which you need to type this:

```
cmake -DCMAKE_BUILD_TYPE=Debug .. // for debug mode
cmake -DCMAKE_BUILD_TYPE=Release .. // for release mode
```

This will generate project files that you will use for development.

# Update

To update this repository:

```
git pull
git submodule update --init --recursive
```

# Documentation

In order to generate the documentation you need to have [Doxygen](https://www.doxygen.nl/index.html) installed.

The documentation can be generated by executing the `doxygen` command while being in the `docs` directory. If Doxygen is installed, this will be done automatically on each build.

The generated documentation will be located at `docs/gen/html`, open the `index.html` file from this directory to see the documentation.