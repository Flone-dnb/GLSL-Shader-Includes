#include "include/root_constants.glsl"

#additional_root_constants
{
    uint someIndex1;
    uint someIndex2;
}

#additional_root_constants{
    uint additionalIndex1;
    uint additionalIndex2;
}

#additional_root_constants uint additionalIndex3;

#additional_push_constants uint THIS_LINE_IS_SKIPPED;

#additional_shader_constants uint newConstant;

void foo(){
    // some code here
}