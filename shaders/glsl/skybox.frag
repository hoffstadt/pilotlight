#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap;


layout(set = 2, binding = 0)  uniform sampler2D tSkinningSampler;

layout(set = 3, binding = 0) uniform _plObjectInfo
{
    mat4 tModel;
} tObjectInfo;

// input
layout(location = 0) in struct plShaderOut {
    vec3 tPosition;
    vec3 tWorldPosition;
} tShaderIn;

// output
layout(location = 0) out vec4 outColor;

void main() 
{
    vec3 tVectorOut = normalize(tShaderIn.tWorldPosition);
    outColor = texture(samplerCubeMap, tVectorOut);
}