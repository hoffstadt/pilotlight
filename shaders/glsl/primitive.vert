#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "common.glsl"

// input
layout(location = 0) in vec3 inPos;

// output
layout(location = 0) out struct plShaderOut {
    vec3 tPosition;
    vec3 tWorldPosition;
    vec3 tNormal;
    vec3 tWorldNormal;
    vec2 tUV;
    vec4 tColor;
    mat3 tTBN;
} tShaderOut;

void main() 
{

    vec3 inPosition  = inPos;
    vec3 inNormal    = vec3(0.0, 0.0, 0.0);
    vec4 inTangent   = vec4(0.0, 0.0, 0.0, 0.0);
    vec2 inTexCoord0 = vec2(0.0, 0.0);
    vec2 inTexCoord1 = vec2(0.0, 0.0);
    vec4 inColor0    = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 inColor1    = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inJoints0   = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inJoints1   = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inWeights0  = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 inWeights1  = vec4(0.0, 0.0, 0.0, 0.0);

    const mat4 tMVP = tGlobalInfo.tCameraViewProj * tObjectInfo.tModel;

    int iCurrentAttribute = 0;
    const uint iVertexDataOffset = VertexStride * (gl_VertexIndex - tObjectInfo.uVertexOffset) + tObjectInfo.uVertexDataOffset;

    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_POSITION))  { inPosition  = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute].xyz; iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_NORMAL))    { inNormal    = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute].xyz; iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TANGENT))   { inTangent   = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0)){ inTexCoord0 = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute].xy;  iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_1)){ inTexCoord1 = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute].xy;  iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_COLOR_0))   { inColor0    = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_COLOR_1))   { inColor1    = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_JOINTS_0))  { inJoints0   = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_JOINTS_1))  { inJoints1   = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_0)) { inWeights0  = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}
    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_WEIGHTS_1)) { inWeights1  = tVertexBuffer.atVertexData[iVertexDataOffset + iCurrentAttribute];     iCurrentAttribute++;}

    gl_Position = tMVP * vec4(inPosition, 1.0);
    tShaderOut.tPosition = gl_Position.xyz;

    tShaderOut.tUV = inTexCoord0;
    tShaderOut.tNormal  = inNormal;
    tShaderOut.tColor = inColor0;
    tShaderOut.tWorldNormal = normalize(tObjectInfo.tModel * vec4(inNormal, 0.0)).xyz;
    tShaderOut.tWorldPosition = (tObjectInfo.tModel * vec4(inPos, 1.0)).xyz;

    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TANGENT))
    {
        vec3 WorldTangent = mat3(tObjectInfo.tModel) * inTangent.xyz;
        vec3 WorldBitangent = cross(inNormal, inTangent.xyz) * inTangent.w;
        WorldBitangent = mat3(tObjectInfo.tModel) * WorldBitangent;
        tShaderOut.tTBN = mat3(WorldTangent, WorldBitangent, tShaderOut.tWorldNormal);
    }
    else
    {
        tShaderOut.tTBN = mat3(1.0);
    }
}