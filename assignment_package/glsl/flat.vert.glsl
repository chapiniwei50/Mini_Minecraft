#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform mat4 u_ModelInvTr;
uniform mat4 u_LightSpaceMatrix;

in vec4 vs_Pos;
in vec4 vs_Nor;
in vec4 vs_UV;

out vec3 fs_UV;
out vec4 fs_Nor;
out vec4 fs_LightVec;
out vec3 fs_Pos;
out vec4 fs_PosLightSpace;

const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));

void main()
{
    fs_UV = vs_UV.xyz;


    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);

    vec4 modelposition = u_Model * vs_Pos;

    fs_LightVec = (lightDir);

    vec4 worldPos = u_Model * vs_Pos;
    fs_Pos = worldPos.xyz;

    fs_PosLightSpace = u_LightSpaceMatrix * modelposition;

    //built-in things to pass down the pipeline
    gl_Position = u_ViewProj * modelposition;

}
