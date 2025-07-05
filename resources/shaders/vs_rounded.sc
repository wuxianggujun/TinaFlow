$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_position

#include <bgfx_shader.sh>

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;

    // 使用纹理坐标来传递相对位置 (-1到1的范围)
    // 这样片段着色器就能知道当前像素在矩形中的相对位置
    v_position = a_texcoord0;
}
