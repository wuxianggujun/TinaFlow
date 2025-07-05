$input v_color0, v_texcoord0, v_position

#include <bgfx_shader.sh>

// 圆角矩形参数
uniform vec4 u_roundedParams; // x=width, y=height, z=cornerRadius, w=unused
// 连接器配置参数
uniform vec4 u_connectorConfig; // x=topConnectors, y=bottomConnectors, z=leftConnectors, w=rightConnectors

// 计算圆角矩形的距离场
float roundedBox(vec2 pos, vec2 size, float radius)
{
    vec2 d = abs(pos) - size + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

// 检查是否在连接器区域
bool isInConnectorArea(vec2 pos, float threshold)
{
    // 顶部连接器区域
    if (pos.y > threshold && abs(pos.x) < 0.5) {
        return true;
    }

    // 底部连接器区域
    if (pos.y < -threshold && abs(pos.x) < 0.5) {
        return true;
    }

    // 左侧连接器区域
    if (pos.x < -threshold && abs(pos.y) < 0.5) {
        return true;
    }

    // 右侧连接器区域
    if (pos.x > threshold && abs(pos.y) < 0.5) {
        return true;
    }

    return false;
}

// 计算连接器的距离场
float getConnectorDistance(vec2 pos)
{
    float minDist = 1000.0;

    // 顶部连接器
    if (pos.y > 1.0 && abs(pos.x) < 0.5) {
        vec2 connectorPos = vec2(pos.x * 6.0, (pos.y - 1.25) * 2.0);
        vec2 connectorSize = vec2(6.0, 2.0);
        vec2 d = abs(connectorPos) - connectorSize;
        minDist = min(minDist, max(d.x, d.y));
    }

    // 底部连接器
    if (pos.y < -1.0 && abs(pos.x) < 0.5) {
        vec2 connectorPos = vec2(pos.x * 6.0, (pos.y + 1.25) * 2.0);
        vec2 connectorSize = vec2(6.0, 2.0);
        vec2 d = abs(connectorPos) - connectorSize;
        minDist = min(minDist, max(d.x, d.y));
    }

    // 左侧连接器
    if (pos.x < -1.0 && abs(pos.y) < 0.5) {
        vec2 connectorPos = vec2((pos.x + 1.25) * 2.0, pos.y * 6.0);
        vec2 connectorSize = vec2(2.0, 6.0);
        vec2 d = abs(connectorPos) - connectorSize;
        minDist = min(minDist, max(d.x, d.y));
    }

    // 右侧连接器
    if (pos.x > 1.0 && abs(pos.y) < 0.5) {
        vec2 connectorPos = vec2((pos.x - 1.25) * 2.0, pos.y * 6.0);
        vec2 connectorSize = vec2(2.0, 6.0);
        vec2 d = abs(connectorPos) - connectorSize;
        minDist = min(minDist, max(d.x, d.y));
    }

    return minDist;
}



void main()
{
    vec2 size = u_roundedParams.xy * 0.5; // 半宽度和半高度
    float radius = u_roundedParams.z;
    vec2 pos = v_position * size;

    float dist;

    // 检查是否在任何连接器附近的区域
    bool nearConnector = isInConnectorArea(v_position, 0.8);

    // 检查是否在连接器区域内
    bool inConnector = (abs(v_position.x) > 1.0 || abs(v_position.y) > 1.0);

    if (inConnector) {
        // 在连接器区域 - 计算连接器距离场
        dist = getConnectorDistance(v_position);
    } else if (nearConnector) {
        // 在连接器附近的主体区域 - 使用直边，不使用圆角
        vec2 d = abs(pos) - size;
        dist = max(d.x, d.y);
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
