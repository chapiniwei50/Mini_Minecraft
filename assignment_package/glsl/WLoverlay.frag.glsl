#version 150
// ^ Change this to version 130 if you have compatibility issues


// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec2 fs_UV;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.
uniform sampler2D u_RenderedTexture;
uniform int u_EffectType;
uniform int u_Time;

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                 dot(p, vec2(269.5,183.3))))
                 * 43758.5453);
}


float WorleyNoise(vec2 uv) {
    uv *= 10.0; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);
    float minDist = 1.0; // Minimum distance initialized to max.
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            vec2 neighbor = vec2(float(x), float(y)); // Direction in which neighbor cell lies
            vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

void main()
{

    vec3 original = texture(u_RenderedTexture, fs_UV).rgb;
    if(u_EffectType == 1){
        float time = u_Time / 5.0;
        const float frequency = 5.0;
        const float distortionAmount = 0.02;
        vec2 distortedUV = fs_UV * (1 - 2 * distortionAmount) + distortionAmount;//Prevent UV from crossing [0,1] boundary
        vec2 ripple = vec2(sin(fs_UV.x * frequency + time), sin(fs_UV.y * frequency + time));
        distortedUV += ripple * distortionAmount;
        vec3 original = texture(u_RenderedTexture, distortedUV).rgb;

        vec3 blueTint = vec3(0.0, 0.0, 1.0);
        out_Col = vec4(mix(original, blueTint, 0.5), 1.0);
        return;

    } else if(u_EffectType == 2){
        vec3 redTint = vec3(1.0, 0.0, 0.0); // Red tint color
        // Apply the blue tint to the original color
        out_Col = vec4(mix(original, redTint, 0.5), 1.);
        return;
    }

    out_Col = vec4(original,1.0);
}

