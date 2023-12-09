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
uniform float u_height;

in vec3 fs_UV;
in vec3 fs_Nor;
in vec3 fs_Pos;
in vec4 fs_PosLightSpace;

out vec4 out_Col;

const float u_FogDensity = 0.04;
const vec3 u_FogColor = vec3(0.37f, 0.74f, 1.0f);
const float u_FogStart = 190.0;
const float u_FogEnd = 210.0;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(u_ShadowMappingDepth, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // add larger bias when the face is more verticle to sunlight
    float min_bias = 0.00002;
    float max_bias = 0.00018;
    float dot_value = dot(normalize(fs_Nor), normalize(u_LightDirection));
    float abs_dot_value = abs(dot_value);  // take the abs so that opposite face should also have no bias
    float bias = max(min_bias, max_bias * (1 - abs_dot_value));


    float shadow = 0.0;
    if (dot_value < -0.000001)  // self shadow, always 1
        shadow = 1.0;
    else if (projCoords.x < 0 || projCoords.x > 1 || projCoords.y < 0 || projCoords.y > 1)
        {} // do not render shadow if it is out of the shadow depth map
    else if (abs_dot_value < -0.000001)
        {}   // the plane is too vertical to generate shadow
    else if (u_height > 10.f)  // if u_height > 20, no need to use PCF, just normal shadow mapping
        shadow = currentDepth - bias > closestDepth ? 1.f : 0.f;
    else {
        // player is close to the scene, then need to soft the shadow with its neighbours
        vec2 texelSize = 1.0 / textureSize(u_ShadowMappingDepth, 0);

        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(u_ShadowMappingDepth, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }

    if (dot(normalize(u_LightDirection), vec3(0.f, 1.f, 0.f)) < 0.1)  // the horizontal plane, and sunlight near horizon
        if (currentDepth - 0.001 < closestDepth)  // is definitely not in shadow
            shadow = 0.f;


    float day_time_value = dot(normalize(u_LightDirection), vec3(0.f, 1.f, 0.f));
    // the shadow intensity should be 0 if day_time_value < 0.12
    // should be the largest when day_time_value >= 0.2
    // but should be 0 if at noon
    float start_shadow_point = 0.05; //0.12;
    float end_shadow_point = 0.2;
    float intensity_factor;
    if (day_time_value < start_shadow_point)
        intensity_factor = 0.f;
    else if (day_time_value > end_shadow_point && day_time_value < 0.95)
        intensity_factor = 1.f;
    else if (day_time_value > 0.95)
        intensity_factor = mix(1, 0, (day_time_value - 0.95) / 0.05);
    else
        intensity_factor = mix(0, 1, (day_time_value - start_shadow_point) / (end_shadow_point - start_shadow_point));

    shadow *= intensity_factor;


    return shadow * 0.5;
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
    float ambientTerm;
//    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
//                                                        //to simulate ambient lighting. This ensures that faces that are not
//                                                        //lit by our point light are not completely black.
//    lightIntensity = clamp(lightIntensity, 0, 1);

    float day_time_value = dot(normalize(u_LightDirection), vec3(0.f, 1.f, 0.f));

    // determine the ambientTerm value
    // if day_time_value > 0.8, ambientTerm should be the largest
    // if day_time_value < 0, ambientTerm should be the smallest
    float max_ambient = 0.7;
    float min_ambient = 0.3;
    float max_ambient_threshold = 0.8;
    float min_ambient_threshold = 0.12;
    if (day_time_value > max_ambient_threshold)
        ambientTerm = max_ambient;
    else if (day_time_value < min_ambient_threshold)
        ambientTerm = min_ambient;
    else
        ambientTerm = mix(min_ambient, max_ambient, (day_time_value - min_ambient_threshold) / (max_ambient_threshold - min_ambient_threshold));

    // determine the diffuseTerm value
    // if day_time_value < 0, diffuseTerm should be the smallest
    // if day_time_value > 0.1, diffuseTerm should be the largest
    float min_diffuseTerm_factor = 0.2;
    float max_diffuseTerm_factor = 1.0;
    if (day_time_value < 0.12)
        diffuseTerm *= min_diffuseTerm_factor;  // if at night, the diffuse term value should also be smaller
    else if (day_time_value > 0.2)
        diffuseTerm *= max_diffuseTerm_factor;
    else
        diffuseTerm *= mix(min_diffuseTerm_factor, max_diffuseTerm_factor, (day_time_value - 0.12) / 0.08);

    float lightIntensity = clamp(ambientTerm + diffuseTerm, 0, 1);

    // add shadow
    float shadow = ShadowCalculation(fs_PosLightSpace);
    //float shadow = 0;
    vec3 pure_color = (1 - shadow) * diffuseColor.rgb * lightIntensity;

    // add fog in distance
    float depth = length(fs_Pos - u_CameraPos);
    float fogFactor = clamp((u_FogEnd - depth) / (u_FogEnd - u_FogStart), 0.0, 1.0);
    vec3 finalColor = mix(u_FogColor, pure_color, fogFactor);
    out_Col = vec4(finalColor, diffuseColor.a);
}
