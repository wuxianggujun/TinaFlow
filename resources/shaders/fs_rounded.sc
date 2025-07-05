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
    vec2 size = u_roundedParams.xy * 0.5; // 半宽度和半高度
    float radius = u_roundedParams.z;
    vec2 pos = v_position * size;

    float dist;

    // 检查是否在连接器附近的区域（顶部边缘）
    bool nearConnector = (v_position.y > 0.8 &&
                         ((abs(v_position.x) < 0.5 && abs(v_position.x) > 0.1) ||
                          v_position.y > 1.0));

    if (nearConnector) {
        // 在连接器区域或附近 - 使用简单矩形，不使用圆角
        if (v_position.y > 1.0) {
            // 连接器区域
            vec2 connectorPos = vec2(v_position.x * 6.0, (v_position.y - 1.25) * 2.0);
            vec2 connectorSize = vec2(6.0, 2.0);
            vec2 d = abs(connectorPos) - connectorSize;
            dist = max(d.x, d.y);
        } else {
            // 主体顶部区域 - 使用直边，不使用圆角
            vec2 d = abs(pos) - size;
            dist = max(d.x, d.y);
        }
    } else {
        // 主体其他区域 - 使用圆角
        dist = roundedBox(pos, size, radius);
    }

    // 使用距离场创建边缘
    float alpha = 1.0 - smoothstep(-0.1, 0.1, dist);

    // 确保完全透明的像素不会显示
    if (alpha < 0.01) {
        discard;
    }

    // 输出颜色
    gl_FragColor = vec4(v_color0.rgb, v_color0.a * alpha);
}
