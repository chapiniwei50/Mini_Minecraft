#version 150
// ^ Change this to version 130 if you have compatibility issues


in vec4 vs_Col;             // The array of vertex colors passed to the shader.
in vec2 vs_UV;
in vec4 vs_Pos;


out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.
out vec2 fs_UV;


void main()
{
    fs_Col = vs_Col;                         // Pass the vertex colors to the fragment shader for interpolation

    fs_UV = vs_UV;
    gl_Position = vs_Pos;
}
