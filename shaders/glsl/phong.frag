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
    uint  uVertexStride;

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

layout(set = 2, binding = 1) uniform sampler2D colorSampler;


void main() 
{
    const vec3 lightDir0 = normalize(vec3(0.0, -1.0, -1.0));
    vec3 Eye = normalize(-tGlobalInfo.tCameraPos.xyz);
    vec3 Reflected = normalize(reflect(-lightDir0, inWorldN)); 

    const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);

    vec4 diffuseColor = lightColor * max(0.0, dot(inWorldN, -lightDir0));

    float specular = 0.25;
    vec4 specularColor = vec4(1.0, 1.0, 1.0, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 0.8) * specular; 

    outColor = texture(colorSampler, inUV);
    const float alpha = outColor.a;
    // outColor = (tGlobalInfo.tAmbientColor + diffuseColor) * outColor + specularColor;
    outColor = (tGlobalInfo.tAmbientColor + diffuseColor) * outColor;
    outColor.a = alpha;
    if(outColor.a < 0.01)
    {
        discard;
    }
    // outColor = vec4(inWorldN, 1.0);
    // outColor = vec4(texture(normalSampler, inUV).xyz, 1.0);

}