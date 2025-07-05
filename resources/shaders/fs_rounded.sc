$input v_color0, v_texcoord0, v_position

#include <bgfx_shader.sh>

// 圆角矩形参数
uniform vec4 u_roundedParams; // x=width, y=height, z=cornerRadius, w=unused

// 计算圆角矩形的距离场
float roundedBox(vec2 pos, vec2 size, float radius)
{
    vec2 d = abs(pos) - size + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

void main()
{
    // 简单测试：创建一个圆形来验证着色器是否工作
    vec2 center = vec2(0.0, 0.0);
    vec2 pos = v_position; // 纹理坐标 (-1到1)

    // 计算到中心的距离
    float dist = length(pos - center);

    // 创建一个圆形，半径为0.8
    float alpha = 1.0 - smoothstep(0.7, 0.8, dist);

    // 如果在圆形内，显示原色；否则透明
    if (alpha > 0.5) {
        gl_FragColor = v_color0;
    } else {
        gl_FragColor = vec4(v_color0.rgb, 0.0); // 透明
    }
}
