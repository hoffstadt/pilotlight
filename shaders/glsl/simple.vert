#version 450
#extension GL_ARB_separate_shader_objects : enable
#define VULKAN 100

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outN;
layout(location = 2) out vec3 outWorldN;
layout(location = 3) out vec2 outUV;

layout(std140, set = 1, binding = 0) readonly buffer VertexBuffer
{
	vec4 vertexData[];
} vertexBuffer;

layout(set = 0, binding = 0) uniform tnConstantBuffer
{
    mat4 Model;
    mat4 ModelView;
    mat4 ModelViewProjection;
} cbuf;



const int PL_MESH_FORMAT_FLAG_NONE           = 0;
const int PL_MESH_FORMAT_FLAG_HAS_POSITION   = 1 << 0;
const int PL_MESH_FORMAT_FLAG_HAS_NORMAL     = 1 << 1;
const int PL_MESH_FORMAT_FLAG_HAS_TEXCOORD   = 1 << 2;
const int PL_MESH_FORMAT_FLAG_HAS_COLOR      = 1 << 3;
const int PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_2 = 1 << 4;
const int PL_MESH_FORMAT_FLAG_HAS_TANGENT    = 1 << 5;

layout(constant_id = 0) const int MeshVariantFlags = PL_MESH_FORMAT_FLAG_NONE;

void main() 
{

    vec3 inN = vec3(0.0, 0.0, 0.0);
    vec3 inT = vec3(0.0, 0.0, 0.0);
    vec2 inUV = vec2(0.0, 0.0);

    if(bool(MeshVariantFlags & PL_MESH_FORMAT_FLAG_HAS_TANGENT))
    {
        inN = vertexBuffer.vertexData[3 * gl_VertexIndex].xyz;
        inT = vertexBuffer.vertexData[3 * gl_VertexIndex + 1].xyz;
        inUV = vertexBuffer.vertexData[3 * gl_VertexIndex + 2].xy;
    }
    else
    {
        inN = vertexBuffer.vertexData[2 * gl_VertexIndex].xyz;
        inUV = vertexBuffer.vertexData[2 * gl_VertexIndex + 1].xy; 
    }

    gl_Position = cbuf.ModelViewProjection * vec4(inPos, 1.0);
    outPos = gl_Position.xyz;

    outUV = inUV;
    outN  = inN;
    outWorldN = normalize(cbuf.Model * vec4(inN, 0.0)).xyz;
}