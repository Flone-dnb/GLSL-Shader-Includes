# Fork information

This fork adds the following changes:
- Upgrade to C++20.
- Code refactoring.
- Better error handling.
- Add unit tests.
- Minor project, code style and documentation improvements.
- Require double quotes for include paths (for example: `#include "Base.glsl"`).
- Added optional `#additional_push_constants` keyword (see description below).

# GLSL Shader Includer

A utility class which allows the end user to make use of the include statement in a shader file.
This is a C++ class but any programmer that has basic knowledge of his / her programming language of choice, should be able to quickly convert the code to another language within a couple minutes. It is that simple.

### Introduction

The sole purpose of this class is to load a file and extract the text that is in it. In theory, this class could be used for a variety of text-processing purposes, but it was initially designed to be used to load shader source code for GLSL, as it does not have a built-in function that lets you include another source files easily.

### Using this project

In your cmake file:

```cmake
set(GLSL_SHADER_INCLUDER_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(GLSL_SHADER_INCLUDER_ENABLE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(GLSL_SHADER_INCLUDER_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD OFF CACHE BOOL "" FORCE) // optional
add_subdirectory(<some path here>/GLSL-Shader-Includes SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC GlslShaderIncluderLib)
```

`GLSL_SHADER_INCLUDER_ENABLE_ADDITIONAL_PUSH_CONSTANTS_KEYWORD` used to enable `#additional_push_constants` keyword which is used to append variables to a push constants struct (located in a separate shader file), for example:

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

Then in your C++ code:

```cpp
#include "ShaderIncluder.h"

auto result = ShaderIncluder::parseFullSourceCode("path/to/shader.extension");
if (std::holds_alternative<ShaderIncluder::Error>(result)){
    // handle error
    return;
}
const auto sFullSourceCode = std::get<std::string>(std::move(result));
```

This will (recursively) extract the source code from the first shader file.

Now, you might be wondering, what is the point of using your code for something so trivial as to loading a file and calling the "std::getline()" function on it?

Well, besides loading the shader source code from a single file, the loader also supports custom keywords that allow you to include external files inside your shader source code! Since all of this loading is still fairly trivial but cumbersome to write over and over again, it has been uploaded to this repository for you to use.

### Example

*Vertex shader [./resources/shaders/basic.vs]*
```glsl
#version 330 core
layout (location = 0) in vec3 position;

// Include other files
#include "include/functions.incl"
#include "include/uniforms.incl"

void main()
{
    position += doFancyCalculationA() * offsetA;
    position += doFancyCalculationB() * offsetB;
    position += doFancyCalculationC() * offsetC;

    gl_Position = vec4(position, 1.0);
}
```

*Utility functions [./resources/shaders/include/functions.incl]*
```glsl
vec3 doFancyCalculationA()
{
    return vec3(1.0, 0.0, 1.0);
}

vec3 doFancyCalculationB()
{
    return vec3(0.0, 1.0, 0.0);
}

vec3 doFancyCalculationC()
{
    return vec3(0.0, 0.0, 1.0);
}
```

*Uniforms [./resources/shaders/include/uniforms.incl]*
```glsl
uniform vec3 offsetA;
uniform vec3 offsetB;
uniform vec3 offsetC;
```

*Result*
```glsl
#version 330 core
layout (location = 0) in vec3 position;

vec3 doFancyCalculationA()
{
    return vec3(1.0, 0.0, 1.0);
}

vec3 doFancyCalculationB()
{
    return vec3(0.0, 1.0, 0.0);
}

vec3 doFancyCalculationC()
{
    return vec3(0.0, 0.0, 1.0);
}

uniform vec3 offsetA;
uniform vec3 offsetB;
uniform vec3 offsetC;

void main()
{
    position += doFancyCalculationA() * offsetA;
    position += doFancyCalculationB() * offsetB;
    position += doFancyCalculationC() * offsetC;

    gl_Position = vec4(position, 1.0);
}
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