#version 150
// ^ Change this to version 130 if you have compatibility issues

in vec4 vs_Pos;
uniform mat4 u_Model;
uniform mat4 u_ViewProj;

void main() {
    gl_Position = u_ViewProj * u_Model * vs_Pos;
}
