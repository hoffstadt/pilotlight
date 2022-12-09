#version 450
#extension GL_ARB_separate_shader_objects : enable

//-----------------------------------------------------------------------------
// [SECTION] shader input/output
//-----------------------------------------------------------------------------

// input
layout(location = 0) in vec3 inPos;

// output
layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outN;
layout(location = 2) out vec3 outWorldN;
layout(location = 3) out vec2 outUV;

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
    uint uVertexStride;

} tMaterialInfo;

//-----------------------------------------------------------------------------
// [SECTION] object
//-----------------------------------------------------------------------------

layout(set = 2, binding = 0) uniform _plObjectInfo
{
    mat4  tModel;
    uint  uVertexOffset;
    // ivec3 _unused0;
} tObjectInfo;

layout(set = 2, binding = 1) uniform sampler2D colorSampler;

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

const int PL_MESH_FORMAT_FLAG_NONE           = 0;
const int PL_MESH_FORMAT_FLAG_HAS_POSITION   = 1 << 0;
const int PL_MESH_FORMAT_FLAG_HAS_NORMAL     = 1 << 1;
const int PL_MESH_FORMAT_FLAG_HAS_TANGENT    = 1 << 2;
const int PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0 = 1 << 3;
const int PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_1 = 1 << 4;
const int PL_MESH_FORMAT_FLAG_HAS_COLOR_0    = 1 << 5;
const int PL_MESH_FORMAT_FLAG_HAS_COLOR_1    = 1 << 6;
const int PL_MESH_FORMAT_FLAG_HAS_JOINTS_0   = 1 << 7;
const int PL_MESH_FORMAT_FLAG_HAS_JOINTS_1   = 1 << 8;
const int PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_0  = 1 << 9;
const int PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_1  = 1 << 10;

//-----------------------------------------------------------------------------
// [SECTION] specialization constants
//-----------------------------------------------------------------------------

layout(constant_id = 0) const int MeshVariantFlags = PL_MESH_FORMAT_FLAG_NONE;
layout(constant_id = 1) const int VertexStride     = 0;

void main() 
{

    vec3 inPosition  = inPos;
    vec3 inNormal    = vec3(0.0, 0.0, 0.0);
    vec4 inTangent   = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 inTexCoord0 = vec2(0.0, 0.0);
    vec2 inTexCoord1 = vec2(0.0, 0.0);
    vec4 inColor0    = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inColor1    = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inJoints0   = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inJoints1   = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inWeights0  = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inWeights1  = vec4(0.0, 0.0, 0.0, 0.0);

    const mat4 tMVP = tGlobalInfo.tCameraViewProj * tObjectInfo.tModel;

    int iCurrentAttribute = 0;

    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_POSITION))  { inPosition  = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute].xyz; iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_NORMAL))    { inNormal    = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute].xyz; iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TANGENT))   { inTangent   = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0)){ inTexCoord0 = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute].xy;  iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_1)){ inTexCoord1 = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute].xy;  iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_COLOR_0))   { inColor0    = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_COLOR_1))   { inColor1    = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_JOINTS_0))  { inJoints0   = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_JOINTS_1))  { inJoints1   = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_0)) { inWeights0  = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_1)) { inWeights1  = tVertexBuffer.atVertexData[tObjectInfo.uVertexOffset + VertexStride * gl_VertexIndex + iCurrentAttribute];     iCurrentAttribute++;}

    gl_Position = tMVP * vec4(inPosition, 1.0);
    outPos = gl_Position.xyz;

    outUV = inTexCoord0;
    outN  = inNormal;
    outWorldN = normalize(tObjectInfo.tModel * vec4(inNormal, 0.0)).xyz;
}