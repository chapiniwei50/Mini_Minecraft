#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments
uniform sampler2D u_Texture;
uniform int u_Time;

in vec3 fs_UV;
in vec4 fs_Nor;
in vec4 fs_LightVec;

out vec4 out_Col;

void main()
{
    vec4 diffuseColor;

    if (abs(fs_UV.z - 1.0) < 0.001)  // WATER
    {
        float time_offset = (u_Time % 500) / 500.0;
        diffuseColor = texture(u_Texture, fs_UV.xy + vec2(1.0/16.0, 1.0/16.0) * time_offset);
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
    float diffuseTerm = dot(normalize(fs_Nor), normalize(fs_LightVec));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.5;

    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
                                                        //to simulate ambient lighting. This ensures that faces that are not
                                                        //lit by our point light are not completely black.
    lightIntensity = clamp(lightIntensity, 0, 1);

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb * lightIntensity, diffuseColor.a);
}
