#version 150
// ^ Change this to version 130 if you have compatibility issues


// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec2 fs_UV;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.
uniform sampler2D u_RenderedTexture;
uniform int u_EffectType;


void main()
{

    vec3 original = texture(u_RenderedTexture, fs_UV).rgb;
    if(u_EffectType == 1){
        vec3 blueTint = vec3(0.0, 0.0, 1.0); // Blue tint color
        // Apply the blue tint to the original color
        out_Col = vec4(mix(original, blueTint, 0.5), 1.);
        return;
    } else if(u_EffectType == 2){
        vec3 redTint = vec3(1.0, 0.0, 0.0); // Red tint color
        // Apply the blue tint to the original color
        out_Col = vec4(mix(original, redTint, 0.5), 1.);
        return;
    }

    out_Col = vec4(original,1.0);
}

