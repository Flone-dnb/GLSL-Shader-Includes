# Combined Shader Language Parser

Since HLSL and GLSL are kind of similar, using this simple parser you can avoid duplicating some shader code if you need to write both HLSL and GLSL shaders, plus this parser also has some additional features that you might find helpful, here is a small example of what this parser can do:

```
// input file

#hlsl cbuffer frameData : register(b?, space5){   // <- `?` here tells the parser to assign a free index
#glsl layout(binding = ?) uniform sampler2D diffuseTexture[]; // <- `?` will be replaced
#glsl layout(binding = ?) uniform frameData{
    mat4 viewProjectionMatrix;
    vec3 cameraPosition;
};
```

when `parseHlsl` is used you will get the following code:

```
cbuffer frameData : register(b0, space5){   // `#glsl` content was removed, index `0` was assigned
    float4x4 viewProjectionMatrix;          // `mat4` was converted to `float4x4`
    float3 cameraPosition;                  // `vec3` was converted to `float3`
};
```

when `parseGlsl` is used you will get the following code:

```
// `#hlsl` content was removed
layout(binding = 0) uniform sampler2D diffuseTexture[]; // index `0` was assigned
layout(binding = 1) uniform frameData{                  // index `1` was assigned
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

There is also an experimental and somewhat limited support for mixed keywords on the same line:

```
#glsl layout(binding = 0) uniform #hlsl struct #both ComputeInfo {
    uint iThreadGroupCountX;
} #glsl computeInfo; #hlsl ; ConstantBuffer<ComputeInfo> computeInfo : register(b0, space5);

// as GLSL:
layout(binding = 0) uniform ComputeInfo {
    uint iThreadGroupCountX;
} computeInfo;

// as HLSL:
struct ComputeInfo {
    uint iThreadGroupCountX;
} ; ConstantBuffer<ComputeInfo> computeInfo : register(b0, space5);
```

Note:

- This parser does not have a fixed file extension and it just processes the file you give to it.
- This parser should be run to read the file from disk to then pass parsed text to your shader compiler.

What this parser does:

- Replaces `#include "relative/path"` with included file contents - might be useful in GLSL.
- Allows to specify additional include directories before parsing.
- Appends variables from blocks marked as `#additional_push_constants` or `#additional_root_constants` to the initial (probably included) push constants layout definition to allow "extending" root/push constants from other files - might be useful in some applications.
- Has two modes: `parseHlsl` and `parseGlsl`:
    - `parseHlsl` parses the specified file and replaces simple GLSL types (see the full list of conversions performed by the parser below) to HLSL types (such as `float3` and `float3x3`) in the memory while reading (source file will not be changed) - allows you to have 1 shader written with GLSL types and process it in both DirectX and Vulkan/OpenGL.
    - `parseGlsl` just parses the file as usual (no type conversion will be applied).
- Allows to specify `#hlsl` and `#glsl` block of code in your shader source file that will be processed differently in different modes:
    - When `parseHlsl` meets `#hlsl` block of code it removes the `#hlsl` keyword (in the memory while reading) and just reads the code that was specified in the `#hlsl` block without doing any type conversions.
    - When `parseHlsl` meets `#glsl` block of code it removes the keyword and all code from this block (in the memory while reading).
    - When `parseGlsl` meets `#hlsl` block of code it removes the keyword and all code from this block (in the memory while reading).
    - When `parseGlsl` meets `#glsl` block of code it removes the `#glsl` keyword (in the memory while reading) and just reads the code that was specified in the `#glsl` block without doing any type conversions.
- Allows specifying `binding = ?` in GLSL code and `register(b?)` in HLSL code (`b` register is used as an example, you can use any: `b`, `u`, etc) to assign a free (unused) binding index while parsing.

# Performed conversions

Here is the list of all conversions that this parser does:

- GLSL to HLSL:
    - `vecN` to `floatN`
    - `matN` to `floatNxN`
    - `shared` to `groupshared` (for compute shaders)
    - cast functions:
        - `floatBitsToUint` to `asuint`
        - `uintBitsToFloat` to `asfloat`
    - atomic functions:
        - `atomicMin` to `InterlockedMin`
        - `atomicMax` to `InterlockedMax`
- HLSL to GLSL:
    - `mul` to `operator*`

# Using this project

In your cmake file:

```cmake
set(CSL_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(CSL_ENABLE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(CSL_ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD OFF CACHE BOOL "" FORCE)        // optional
set(CSL_ENABLE_AUTOMATIC_BINDING_INDEX_ASSIGNMENT_KEYWORD OFF CACHE BOOL "" FORCE) // optional
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

### Additional push/root constants

`CSL_ENABLE_ADDITIONAL_SHADER_CONSTANTS_KEYWORD` is used to enable `#additional_push_constants` / `#additional_root_constants` / `#additional_shader_constants` keyword which is used to append variables to a push/root constants struct (located in a separate shader file), for example:

```GLSL
// ----------------- SomePushConstants.glsl -----------------

layout(push_constant) uniform Indices
{
    uint arrayIndex;
} indices;

struct RootConstants{
    uint arrayIndex;
}; ConstantBuffer<RootConstants> rootConstants : register(b0);

// ----------------- myfile.glsl -----------------

#include "SomePushConstants.glsl"

#additional_push_constants uint someIndex;
#additional_root_constants uint someIndex;

// or just:
#additional_shader_constants uint someIndex;

// ----------------- resulting myfile.glsl -----------------

layout(push_constant) uniform Indices
{
    uint arrayIndex;
    uint someIndex;
} indices;

struct RootConstants{
    uint arrayIndex;
    uint someIndex;
}; ConstantBuffer<RootConstants> rootConstants : register(b0);

```

### Automatic binding indices

`CSL_ENABLE_AUTOMATIC_BINDING_INDEX_ASSIGNMENT_KEYWORD` is used to enable special `?` character which is used to tell the parser to assign free (unused) binding indices, for example:

```GLSL
// ----------------- myfile.glsl -----------------

#hlsl cbuffer frameData : register(b0, space5);               // hardcoded index             
#hlsl cbuffer someOtherData : register(b?);                   // <- `?` here tells the parser to assign a free index
#hlsl Texture2D diffuseTexture : register(t?, space5);
#hlsl Texture2D normalTexture : register(t?, space5);

#glsl layout(binding = ?) uniform sampler2D diffuseTexture[]; // <- `?` will be replaced
#glsl layout(binding = ?) uniform sampler2D normalTexture[];

// ----------------- myfile.glsl as HLSL -----------------

cbuffer frameData : register(b0, space5); 
cbuffer someOtherData : register(b0);                     // `b0` because `space0` not `space5`
Texture2D diffuseTexture : register(t0, space5);
Texture2D normalTexture : register(t1, space5);

// ----------------- myfile.glsl as GLSL -----------------     

layout(binding = 0) uniform sampler2D diffuseTexture[];
layout(binding = 1) uniform sampler2D normalTexture[];

```

# Building the project for development

Please note the instructions below are only needed if you want to modify this project.

Prerequisites:

- Compiler that supports C++20.
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