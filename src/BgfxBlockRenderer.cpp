#include "BgfxBlockRenderer.hpp"
#include <QDebug>
#include <QTimer>

// 包含bgfx工具函数用于加载着色器
#include <QFile>
#include <bgfx/bgfx.h>
#include <bx/file.h>

// 包含生成的统一着色器头文件
#include "shaders.h"

// 从内嵌数据加载着色器的函数
static bgfx::ShaderHandle loadShaderFromMemory(const uint8_t* data, uint32_t size)
{
    if (!data || size == 0) {
        return BGFX_INVALID_HANDLE;
    }
    const bgfx::Memory* mem = bgfx::copy(data, size);
    return bgfx::createShader(mem);
}

// 根据当前渲染器类型获取平台名称
static std::string getRendererPlatformName(bgfx::RendererType::Enum rendererType)
{
    switch (rendererType) {
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            return "dx11"; // 使用DX11着色器作为DirectX的通用选择
        case bgfx::RendererType::OpenGL:
            return "glsl";
        case bgfx::RendererType::OpenGLES:
            return "essl";
        case bgfx::RendererType::Vulkan:
            return "spirv";
        case bgfx::RendererType::Metal:
            return "metal";
        default:
            return "dx11"; // 默认后备选择
    }
}

// 通用着色器加载函数 - 支持任何着色器名称
static bgfx::ShaderHandle loadShaderForCurrentRenderer(const char* shaderName)
{
    bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    std::string platformName = getRendererPlatformName(rendererType);

    // 使用动态查找获取着色器数据
    ShaderData shaderData = getShaderData(shaderName, platformName);

    if (shaderData.data == nullptr) {
        qWarning() << "Shader not found:" << shaderName << "for platform:" << platformName.c_str();

        // 尝试后备平台
        if (platformName != "dx11") {
            qDebug() << "Trying fallback platform: dx11";
            shaderData = getShaderData(shaderName, "dx11");
        }

        if (shaderData.data == nullptr) {
            qWarning() << "Shader not found even with fallback platform";
            return BGFX_INVALID_HANDLE;
        }
    }

    qDebug() << "Loading shader:" << shaderName << "for platform:" << platformName.c_str()
             << "size:" << shaderData.size << "bytes";

    return loadShaderFromMemory(shaderData.data, shaderData.size);
}

static bgfx::ProgramHandle loadProgram(const char* vsName, const char* fsName)
{
    bgfx::ShaderHandle vsh = loadShaderForCurrentRenderer(vsName);
    bgfx::ShaderHandle fsh = loadShaderForCurrentRenderer(fsName);

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        qWarning() << "Failed to load shaders:" << vsName << fsName;
        return BGFX_INVALID_HANDLE;
    }

    qDebug() << "Successfully loaded shaders for renderer:" << bgfx::getRendererName(bgfx::getRendererType());
    return bgfx::createProgram(vsh, fsh, true);
}

// 顶点结构
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;

    static void init() {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

// 创建矩形几何体
static void createRectangleGeometry(bgfx::VertexBufferHandle& vbh, bgfx::IndexBufferHandle& ibh, bgfx::VertexLayout& layout)
{
    // 初始化顶点布局
    PosColorVertex::init();
    layout = PosColorVertex::ms_layout;

    // 创建矩形顶点数据 (中心在原点，大小200x100)
    static PosColorVertex vertices[] = {
        {-100.0f, -50.0f, 0.0f, 0xff0000ff}, // 左下 - 红色
        { 100.0f, -50.0f, 0.0f, 0xff00ff00}, // 右下 - 绿色
        { 100.0f,  50.0f, 0.0f, 0xffff0000}, // 右上 - 蓝色
        {-100.0f,  50.0f, 0.0f, 0xffffff00}, // 左上 - 黄色
    };

    // 创建索引数据 (两个三角形组成矩形)
    static uint16_t indices[] = {
        0, 1, 2, // 第一个三角形
        2, 3, 0  // 第二个三角形
    };

    // 创建顶点缓冲区
    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices, sizeof(vertices)),
        layout
    );

    // 创建索引缓冲区
    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices, sizeof(indices))
    );
}

BgfxBlockRenderer::BgfxBlockRenderer(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(800, 600);
    
    // 禁用Qt双缓冲，这对bgfx很重要
    setAttribute(Qt::WA_PaintOnScreen, true);
    
    qDebug() << "BgfxBlockRenderer: Initialized";
}

BgfxBlockRenderer::~BgfxBlockRenderer()
{
    m_renderTimer.stop();
    shutdownBgfx();
    qDebug() << "BgfxBlockRenderer: Destroyed";
}

void BgfxBlockRenderer::shutdownBgfx()
{
    if (!m_bgfxInitialized) {
        return;
    }

    // 清理bgfx资源
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_vertexBuffer)) {
        bgfx::destroy(m_vertexBuffer);
        m_vertexBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_indexBuffer)) {
        bgfx::destroy(m_indexBuffer);
        m_indexBuffer = BGFX_INVALID_HANDLE;
    }

    bgfx::shutdown();
    m_bgfxInitialized = false;

    qDebug() << "bgfx shutdown complete";
}

void BgfxBlockRenderer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    
    if (m_bgfxInitialized) {
        return;
    }
    
    qDebug() << "BgfxBlockRenderer::showEvent - initializing bgfx";
    qDebug() << "Widget size:" << width() << "x" << height();
    qDebug() << "Real size:" << realWidth() << "x" << realHeight();
    
    // 初始化bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // 自动选择最佳渲染器
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = static_cast<uint32_t>(realWidth());
    init.resolution.height = static_cast<uint32_t>(realHeight());
    init.resolution.reset = BGFX_RESET_NONE;
    init.platformData.nwh = reinterpret_cast<void*>(winId());
    
    if (!bgfx::init(init)) {
        qCritical() << "Failed to initialize bgfx";
        return;
    }
    
    // 启用调试文本
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    // 设置视图0的清屏状态
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x336699ff, 1.0f, 0);

    // 创建着色器程序
    m_program = loadProgram("vs_simple", "fs_simple");

    if (bgfx::isValid(m_program)) {
        qDebug() << "Shader program created successfully";
    } else {
        qWarning() << "Failed to create shader program - will use default rendering";
    }

    // 创建几何体
    createRectangleGeometry(m_vertexBuffer, m_indexBuffer, m_vertexLayout);

    if (bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer)) {
        qDebug() << "Rectangle geometry created successfully";
    } else {
        qWarning() << "Failed to create rectangle geometry";
    }

    m_bgfxInitialized = true;

    qDebug() << "bgfx initialized successfully!";
    qDebug() << "Renderer:" << bgfx::getRendererName(bgfx::getRendererType());
    qDebug() << "Shader program created successfully";
    
    // 启动渲染定时器
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxBlockRenderer::onRenderTimer);
    m_renderTimer.start(16); // 60 FPS
}

void BgfxBlockRenderer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    if (!m_bgfxInitialized) {
        return;
    }
    
    // 设置视图矩形
    bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(realWidth()), static_cast<uint16_t>(realHeight()));
    
    // 这个虚拟绘制调用确保视图0被清除
    bgfx::touch(0);

    // 绘制调试信息
    renderDebugInfo();

    // 绘制测试几何体
    renderTestGeometry();

    // 提交帧
    bgfx::frame();
}

void BgfxBlockRenderer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    if (m_bgfxInitialized && bgfx::getInternalData()->context != nullptr) {
        bgfx::reset(static_cast<uint32_t>(realWidth()), static_cast<uint32_t>(realHeight()), BGFX_RESET_NONE);
        qDebug() << "bgfx reset to resolution:" << realWidth() << "x" << realHeight();
    }
    
    update();
}

void BgfxBlockRenderer::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    // 不在这里关闭bgfx，因为可能只是临时隐藏
}

void BgfxBlockRenderer::onRenderTimer()
{
    if (isVisible() && m_bgfxInitialized) {
        update(); // 触发paintEvent
    }
}

// 视图控制
void BgfxBlockRenderer::setZoom(float zoom)
{
    m_zoom = qBound(0.1f, zoom, 5.0f);
    update();
}

void BgfxBlockRenderer::setPan(const QPointF& pan)
{
    m_pan = pan;
    update();
}

// 坐标变换
QPointF BgfxBlockRenderer::screenToWorld(const QPointF& screenPos) const
{
    QPointF worldPos = (screenPos - m_pan) / m_zoom;
    return worldPos;
}

QPointF BgfxBlockRenderer::worldToScreen(const QPointF& worldPos) const
{
    QPointF screenPos = worldPos * m_zoom + m_pan;
    return screenPos;
}

// 鼠标事件处理
void BgfxBlockRenderer::mousePressEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    m_lastMousePos = mousePos;

    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }

    QWidget::mousePressEvent(event);
}

void BgfxBlockRenderer::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    QPointF delta = mousePos - m_lastMousePos;

    if (m_isPanning) {
        m_pan += delta;
        update();
    }

    m_lastMousePos = mousePos;
    QWidget::mouseMoveEvent(event);
}

void BgfxBlockRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void BgfxBlockRenderer::wheelEvent(QWheelEvent* event)
{
    float scaleFactor = 1.0f + (event->angleDelta().y() / 1200.0f);
    setZoom(m_zoom * scaleFactor);

    QWidget::wheelEvent(event);
}

void BgfxBlockRenderer::renderDebugInfo()
{
    // 使用bgfx的调试文本系统
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x0f, "=== bgfx Renderer ===");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Widget Size: %dx%d", width(), height());
    bgfx::dbgTextPrintf(0, 3, 0x0f, "Real Size: %.0fx%.0f", realWidth(), realHeight());
    bgfx::dbgTextPrintf(0, 4, 0x0f, "DPI Ratio: %.2f", devicePixelRatio());
    bgfx::dbgTextPrintf(0, 5, 0x0f, "Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
    bgfx::dbgTextPrintf(0, 6, 0x0f, "Clear Color: 0x336699ff (Blue-Gray)");
    bgfx::dbgTextPrintf(0, 7, 0x0f, "Initialized: %s", m_bgfxInitialized ? "YES" : "NO");
    bgfx::dbgTextPrintf(0, 8, 0x0f, "Shader Program: %s", bgfx::isValid(m_program) ? "VALID" : "INVALID");
}

void BgfxBlockRenderer::renderTestGeometry()
{
    // 只有在几何体和着色器都有效时才渲染
    if (!bgfx::isValid(m_vertexBuffer) || !bgfx::isValid(m_indexBuffer)) {
        return;
    }

    // 创建正交投影矩阵 (适合2D渲染)
    float proj[16];
    bx::mtxOrtho(proj, 0.0f, static_cast<float>(realWidth()), static_cast<float>(realHeight()), 0.0f, -1.0f, 1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

    // 创建视图矩阵 (包含缩放和平移)
    float view[16];
    bx::mtxIdentity(view);

    // 应用平移和缩放
    float transform[16];
    bx::mtxIdentity(transform);

    // 平移到屏幕中心，然后应用用户的平移和缩放
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;

    // 组合变换：先缩放，再平移到中心，最后应用用户平移
    float scale[16], translate[16], userTranslate[16];
    bx::mtxScale(scale, m_zoom, m_zoom, 1.0f);
    bx::mtxTranslate(translate, centerX, centerY, 0.0f);
    bx::mtxTranslate(userTranslate, static_cast<float>(m_pan.x()), static_cast<float>(m_pan.y()), 0.0f);

    // 组合矩阵：userTranslate * translate * scale
    float temp[16];
    bx::mtxMul(temp, scale, translate);
    bx::mtxMul(transform, temp, userTranslate);

    // 设置变换矩阵
    bgfx::setTransform(transform);

    // 设置视图和投影矩阵
    bgfx::setViewTransform(0, view, proj);

    // 设置顶点和索引缓冲区
    bgfx::setVertexBuffer(0, m_vertexBuffer);
    bgfx::setIndexBuffer(m_indexBuffer);

    // 设置渲染状态 (启用深度测试和写入)
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

    // 提交绘制调用
    if (bgfx::isValid(m_program)) {
        bgfx::submit(0, m_program);
    } else {
        // 如果没有着色器程序，仍然提交以显示几何体（使用内置着色器）
        bgfx::submit(0, BGFX_INVALID_HANDLE);
    }
}
