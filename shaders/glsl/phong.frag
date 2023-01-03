#version 450
#extension GL_ARB_separate_shader_objects : enable

//-----------------------------------------------------------------------------
// [SECTION] shader input/output
//-----------------------------------------------------------------------------

// input
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inN;
layout(location = 2) in vec3 inWorldN;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inColor;

// output
layout(location = 0) out vec4 outColor;

//-----------------------------------------------------------------------------
// [SECTION] global
//-----------------------------------------------------------------------------

layout(set = 0, binding = 0) uniform _plGlobalInfo
{
    vec4 tAmbientColor;

    // camera info
    vec4 tCameraPos;
    mat4 tCameraView;
    mat4 tCameraViewProj;

} tGlobalInfo;

layout(std140, set = 0, binding = 1) readonly buffer _tVertexBuffer
{
	vec4 atVertexData[];
} tVertexBuffer;

//-----------------------------------------------------------------------------
// [SECTION] material
//-----------------------------------------------------------------------------

layout(set = 1, binding = 0) uniform _plMaterialInfo
{
    bool bHasColorTexture;
} tMaterialInfo;

//-----------------------------------------------------------------------------
// [SECTION] object
//-----------------------------------------------------------------------------

layout(set = 2, binding = 0) uniform _plObjectInfo
{
    mat4 tModel;
    uint  uVertexOffset;
    // ivec3 _unused0;
} tObjectInfo;

layout(set = 2, binding = 1) uniform sampler2D tColorSampler;


void main() 
{
    const vec3 tLightDir0 = normalize(vec3(0.0, -1.0, -1.0));
    const vec3 tEyePos = normalize(-tGlobalInfo.tCameraPos.xyz);
    const vec3 tReflected = normalize(reflect(-tLightDir0, inWorldN)); 
    const vec4 tLightColor = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 tDiffuseColor = tLightColor * max(0.0, dot(inWorldN, -tLightDir0));
    const float tSpecularPower = 0.25;
    const vec4 tSpecularColor = vec4(1.0, 1.0, 1.0, 1.0) * pow(max(dot(tReflected, tEyePos), 0.0), 0.8) * tSpecularPower; 

    if(tMaterialInfo.bHasColorTexture == true)
    {
        outColor = texture(tColorSampler, inUV);
    }
    else
    {
        outColor = inColor;
    }
    
    const float alpha = outColor.a;
    outColor = (tGlobalInfo.tAmbientColor + tDiffuseColor) * outColor + tSpecularColor;

    outColor.a = alpha;

    if(outColor.a < 0.01)
    {
        discard;
    }

    // outColor = vec4(inWorldN, 1.0);

}