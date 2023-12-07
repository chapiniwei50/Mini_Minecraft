#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments
uniform sampler2D u_Texture;
uniform sampler2D u_DepthTexture;
uniform sampler2D u_ShadowMappingDepth;
uniform int u_Time;
uniform vec3 u_CameraPos;
uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform mat4 u_ModelInvTr;
uniform vec3 u_LightDirection;

in vec3 fs_UV;
in vec3 fs_Nor;
in vec3 fs_Pos;
in vec4 fs_PosLightSpace;

out vec4 out_Col;

const float u_FogDensity = 0.04;
const vec3 u_FogColor = vec3(0.37f, 0.74f, 1.0f);
const float u_FogStart = 110.0;
const float u_FogEnd = 140.0;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(u_ShadowMappingDepth, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float min_bias = 0.0001;
    float max_bias = 0.001;
    float bias = max(min_bias, max_bias * (1 - dot(normalize(fs_Nor), normalize(u_LightDirection))));
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}


void main()
{
    vec4 diffuseColor;

    if (abs(fs_UV.z - 1.0) < 0.001)  // WATER
    {
        vec3 viewDir = normalize(fs_Pos - u_CameraPos);
        vec3 reflectedDir = reflect(viewDir, normalize(fs_Nor));

        vec3 sunDirection = normalize(u_LightDirection);
        float angle = dot(reflectedDir, sunDirection);
        angle = acos(clamp(angle, -1.0, 1.0));

        const float thresholdMin = radians(1.0);
        const float thresholdMax = radians(5.0);
        float blendFactor = smoothstep(thresholdMax, thresholdMin, angle);

        vec4 sunColor = vec4(1.0, 1.0, 1.0, 1.0);
        float time_offset = (u_Time % 500) / 500.0;
        vec4 textureColor = texture(u_Texture, fs_UV.xy + vec2(1.0/16.0, 1.0/16.0) * time_offset);

        diffuseColor = mix(textureColor, sunColor, blendFactor);
    }
    else if (abs(fs_UV.z - 0.5) < 0.001)  // LAVA
    {
        float time_offset = (u_Time % 500) / 500.0;
        diffuseColor = texture(u_Texture, fs_UV.xy + vec2(1.0/16.0, 1.0/16.0) * time_offset);
    }
    else{
        diffuseColor = texture(u_Texture, fs_UV.xy);
    }

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_Nor), normalize(u_LightDirection));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);
    float ambientTerm = 0.5;
    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
                                                        //to simulate ambient lighting. This ensures that faces that are not
                                                        //lit by our point light are not completely black.
    lightIntensity = clamp(lightIntensity, 0, 1);

    // add shadow
    float shadow = ShadowCalculation(fs_PosLightSpace);
    vec3 pure_color = (1 - shadow) * diffuseColor.rgb * lightIntensity;

    // add fog in distance
    float depth = length(fs_Pos - u_CameraPos);
    float fogFactor = clamp((u_FogEnd - depth) / (u_FogEnd - u_FogStart), 0.0, 1.0);
    vec3 finalColor = mix(u_FogColor, pure_color, fogFactor);
    out_Col = vec4(finalColor, diffuseColor.a);
}
