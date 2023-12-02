#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments
uniform sampler2D u_Texture;
uniform int u_Time;
uniform vec3 u_CameraPos;

in vec3 fs_UV;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec3 fs_Pos;

out vec4 out_Col;

void main()
{
    vec4 diffuseColor;

    if (abs(fs_UV.z - 1.0) < 0.001)  // WATER
    {
        vec3 viewDir = normalize(fs_Pos - u_CameraPos);
        vec3 reflectedDir = reflect(viewDir, normalize(fs_Nor.xyz));


        vec3 currentPos = fs_Pos;
        float totalDistance = 0.0;
        const float maxStepDistance = 15.0;
        const float stepSize = 0.05;
        const float thickness = 0.0006;

        bool hitSomething = false;
        vec4 reflectedColor = vec4(0.0);

        while (totalDistance < maxStepDistance)
        {
                currentPos += stepSize * reflectedDir;  // Step forward
                totalDistance += stepSize;

                // 在这里添加代码以检查 currentPos 在深度纹理中的深度
                // 比如: float depthAtCurrentPos = getDepthAt(currentPos);

                // 如果深度匹配（表示反射光线击中了某物），则采样反射颜色
                // if (depthAtCurrentPos - totalDistance < thickness) {
                //     reflectedColor = texture(u_Texture, currentPos.xy);  // 假设你有一种方法将当前位置转换为纹理坐标
                //     hitSomething = true;
                //     break;
                // }
        }

        // 如果击中了物体，则使用反射颜色，否则使用原始颜色
        //diffuseColor = hitSomething ? reflectedColor : texture(u_Texture, fs_UV.xy);

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
