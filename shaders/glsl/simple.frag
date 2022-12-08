#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inN;
layout(location = 2) in vec3 inWorldN;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;


layout(set = 1, binding = 1) uniform sampler2D colorSampler;


void main() 
{
    const vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);
    const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
    const vec3 lightDir0 = normalize(vec3(0.0, -1.0, -1.0));
    vec4 diffuseColor0 = lightColor * max(0.0, dot(inWorldN, -lightDir0));

    outColor = texture(colorSampler, inUV);
    outColor = (ambient + diffuseColor0) * outColor;
    if(outColor.a < 0.05)
    {
        discard;
    }
    // outColor = vec4(inWorldN, 1.0);
    // outColor = vec4(texture(normalSampler, inUV).xyz, 1.0);

}