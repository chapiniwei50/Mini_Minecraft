#version 150

in vec4 vs_Pos;

uniform ivec2 u_Dimensions;

void main()
{
    gl_Position = vs_Pos;
}
