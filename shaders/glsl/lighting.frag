#version 450
#extension GL_ARB_separate_shader_objects : enable

struct tMaterial
{
    vec4 tColor;
};

layout(set = 0, binding = 0) uniform _plGlobalInfo
{
    vec4 tCameraPos;
    mat4 tCameraView;
    mat4 tCameraProjection;
    mat4 tCameraViewProjection;
} tGlobalInfo;

layout(std140, set = 0, binding = 1) readonly buffer _tVertexBuffer
{
	vec4 atVertexData[];
} tVertexBuffer;

layout(set = 0, binding = 2) readonly buffer plMaterialInfo
{
    tMaterial atMaterials[];
} tMaterialInfo;

layout(set = 1, binding = 0)  uniform sampler2D tAlbedoSampler;
layout(set = 1, binding = 1)  uniform sampler2D tNormalSampler;
layout(set = 1, binding = 2)  uniform sampler2D tPositionSampler;
layout(set = 1, binding = 3)  uniform sampler2D tDepthSampler;

layout(set = 2, binding = 0)  uniform sampler2D tSkinningSampler;

layout(set = 3, binding = 0) uniform _plObjectInfo
{
    int iDataOffset;
    int iVertexOffset;
} tObjectInfo;

const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linearTosRGB(vec3 color)
{
    return pow(color, vec3(INV_GAMMA));
}

layout(location = 0) out vec4 outColor;

// output
layout(location = 0) in struct plShaderIn {
    vec2 tUV;
} tShaderIn;


void main() 
{
    vec4 tBaseColor = texture(tAlbedoSampler, tShaderIn.tUV);
    float fDepth = texture(tDepthSampler, tShaderIn.tUV).r;
    vec3 tSunlightColor = vec3(1.0, 1.0, 1.0);
    vec3 tNormal = texture(tNormalSampler, tShaderIn.tUV).xyz;
    vec3 tSunLightDirection = vec3(-1.0, -1.0, -1.0);
    float fDiffuseIntensity = max(0.0, dot(normalize(tNormal), -normalize(tSunLightDirection)));
    outColor = tBaseColor * vec4(tSunlightColor * (0.05 + fDiffuseIntensity), 1.0);

    outColor = vec4(linearTosRGB(outColor.rgb), tBaseColor.a);
    outColor.b = fDepth;
}