#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform mat4 u_ModelInvTr;
uniform mat4 u_LightSpaceMatrix;
uniform int u_Time;

in vec4 vs_Pos;
in vec4 vs_Nor;
in vec4 vs_UV;

out vec3 fs_UV;
out vec3 fs_Nor;
out vec3 fs_Pos;
out vec4 fs_PosLightSpace;

void main()
{
    fs_UV = vs_UV.xyz;

    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = invTranspose * vec3(vs_Nor);

    vec4 modelposition = u_Model * vs_Pos;

    //WATER WAVE
    if (abs(vs_UV.z - 1.0) < 0.001) {
        float waveAmplitude = 0.1;
        float waveFrequency = 2.0;
        float waveTime = u_Time * 0.1;
        modelposition.y += sin(vs_Pos.x * waveFrequency * 1.3 + vs_Pos.z * waveFrequency * 0.7  + waveTime) * waveAmplitude;
    }

    vec4 worldPos = u_Model * vs_Pos;
    fs_Pos = worldPos.xyz;

    fs_PosLightSpace = u_LightSpaceMatrix * modelposition;

    //built-in things to pass down the pipeline
    gl_Position = u_ViewProj * modelposition;

}
