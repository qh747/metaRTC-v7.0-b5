//
// Copyright (c) 2019-2022 yanggaofeng
//

#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QMouseEvent>
#include "YangPlayWidget.h"

#if !defined(__APPLE__)

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

YangPlayWidget::YangPlayWidget(QWidget* parent) : QOpenGLWidget(parent) {
    textureUniformY = 0;
    textureUniformU = 0;
    textureUniformV = 0;

    id_y = 0;
    id_u = 0;
    id_v = 0;
    
    m_yuv420pBuf = nullptr;
    
    m_pVSHader = nullptr;
    m_pFSHader = nullptr;
    
    m_pShaderProgram = nullptr;
    
    m_pTextureY = nullptr;
    m_pTextureU = nullptr;
    m_pTextureV = nullptr;

    m_height = 0;
    m_width = 0;
}

YangPlayWidget::~YangPlayWidget() {
    this->makeCurrent();
    
    m_pTextureY->destroy();
    m_pTextureU->destroy();
    m_pTextureV->destroy();
    
    doneCurrent();
    
    m_yuv420pBuf = nullptr;
    
    m_pVSHader = nullptr;
    m_pFSHader = nullptr;
    
    m_pTextureY = NULL;
    m_pTextureU = NULL;
    m_pTextureV = NULL;
}

void YangPlayWidget::playVideo(unsigned char* buf, int32_t width, int32_t height) {
    if(m_width != width) {
        m_width = width;
        m_height = height;
    }

    m_yuv420pBuf = buf;

    // 刷新界面,触发paintGL接口
    this->update();
}

void YangPlayWidget::initializeGL() {
    // 初始化 OpenGL 函数入口
    this->initializeOpenGLFunctions();
    
    // 开启深度缓冲测试
    glEnable(GL_DEPTH_TEST);

    // 初始化顶点着色器对象
    m_pVSHader = new QOpenGLShader(QOpenGLShader::Vertex, this);

    // 编译顶点着色器程序
    m_pVSHader->compileSourceCode("\
        attribute vec4 vertexIn;    \
        attribute vec2 textureIn;   \
        varying vec2 textureOut;    \
                                    \
        void main(void) {           \
            gl_Position = vertexIn; \
            textureOut = textureIn; \
        }                           \
    ");
 
    // 初始化片段着色器 功能gpu中yuv转换成rgb
    m_pFSHader = new QOpenGLShader(QOpenGLShader::Fragment, this);

    // 将glsl源码送入编译器编译着色器程序
    m_pFSHader->compileSourceCode("\
        varying vec2 textureOut;                            \
        uniform sampler2D tex_y;                            \
        uniform sampler2D tex_u;                            \
        uniform sampler2D tex_v;                            \
                                                            \
        void main(void) {                                   \
            vec3 yuv;                                       \
            vec3 rgb;                                       \
                                                            \
            yuv.x = texture2D(tex_y, textureOut).r;         \
            yuv.y = texture2D(tex_u, textureOut).r - 0.5;   \
            yuv.z = texture2D(tex_v, textureOut).r - 0.5;   \
                                                            \
            rgb = mat3(                                     \
                1,       1,         1,                      \
                0,       -0.34414,  1.772,                  \
                1.402,   -0.71414,  0                       \
            ) * yuv;                                        \
                                                            \
            gl_FragColor = vec4(rgb, 1);                    \
        }\
    ");

    // 创建着色器程序容器
    m_pShaderProgram = new QOpenGLShaderProgram;

    // 将片段着色器添加到程序容器
    m_pShaderProgram->addShader(m_pFSHader);
    // 将顶点着色器添加到程序容器
    m_pShaderProgram->addShader(m_pVSHader);

    // 绑定属性vertexIn到指定位置ATTRIB_VERTEX, 该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    // 绑定属性textureIn到指定位置ATTRIB_TEXTURE, 该属性在顶点着色源码其中有声明
    m_pShaderProgram->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

    // 链接所有所有添入到的着色器程序
    m_pShaderProgram->link();
    // 激活所有链接
    m_pShaderProgram->bind();

    // 读取着色器中的数据变量tex_y, tex_u, tex_v的位置,这些变量的声明可以在片段着色器源码中可以看到
    textureUniformY = m_pShaderProgram->uniformLocation("tex_y");
    textureUniformU = m_pShaderProgram->uniformLocation("tex_u");
    textureUniformV = m_pShaderProgram->uniformLocation("tex_v");

    // 顶点矩阵
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f,  -1.0f,
        -1.0f, 1.0f,
        1.0f,  1.0f,
    };

    // 纹理矩阵
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    // 设置属性ATTRIB_VERTEX的顶点矩阵值以及格式
    glVertexAttribPointer(
        ATTRIB_VERTEX, 
        2, 
        GL_FLOAT, 
        0, 
        0, 
        vertexVertices
    );
    
    // 设置属性ATTRIB_TEXTURE的纹理矩阵值以及格式
    glVertexAttribPointer(
        ATTRIB_TEXTURE, 
        2, 
        GL_FLOAT, 
        0, 
        0, 
        textureVertices
    );
    
    // 启用ATTRIB_VERTEX属性的数据,默认是关闭的
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    // 启用ATTRIB_TEXTURE属性的数据,默认是关闭的
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    
    //分别创建y,u,v纹理对象
    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);

    m_pTextureY->create();
    m_pTextureU->create();
    m_pTextureV->create();

    // 获取返回y分量的纹理索引值
    id_y = m_pTextureY->textureId();
    // 获取返回u分量的纹理索引值
    id_u = m_pTextureU->textureId();
    // 获取返回v分量的纹理索引值
    id_v = m_pTextureV->textureId();

    // 设置背景色
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void YangPlayWidget::resizeGL(int32_t w, int32_t h) {
    // 防止被零除
    if(h == 0) {
        h = 1;
    }

    //设置视口
    glViewport(0, 0, w, h);
}

void YangPlayWidget::paintGL() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_yuv420pBuf == NULL || m_width == 0 || m_height == 0) {
        return;
    }

    // 加载y数据纹理
    {
        // 激活纹理单元GL_TEXTURE0
        glActiveTexture(GL_TEXTURE0);
    
        // 使用来自y数据生成纹理
        glBindTexture(GL_TEXTURE_2D, id_y);

        // 使用内存中m_yuv420pBuf数据创建真正的y数据纹理
        glTexImage2D(
            GL_TEXTURE_2D, 
            0, 
            GL_LUMINANCE, 
            m_width, 
            m_height, 
            0, 
            GL_LUMINANCE, 
            GL_UNSIGNED_BYTE, 
            m_yuv420pBuf
        );
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // 加载u数据纹理
    {
        // 激活纹理单元GL_TEXTURE1
        glActiveTexture(GL_TEXTURE1);

        // 使用来自u数据生成纹理 
        glBindTexture(GL_TEXTURE_2D, id_u);

        glTexImage2D(
            GL_TEXTURE_2D, 
            0, 
            GL_LUMINANCE, 
            m_width / 2, 
            m_height / 2, 
            0, 
            GL_LUMINANCE, 
            GL_UNSIGNED_BYTE, 
            (const char*)(m_yuv420pBuf + m_width * m_height)
        );
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // 加载v数据纹理
    {
        // 激活纹理单元GL_TEXTURE2
        glActiveTexture(GL_TEXTURE2);

        // 使用来自v数据生成纹理 
        glBindTexture(GL_TEXTURE_2D, id_v);

        glTexImage2D(
            GL_TEXTURE_2D, 
            0, 
            GL_LUMINANCE, 
            m_width / 2, m_height / 2, 
            0, 
            GL_LUMINANCE, 
            GL_UNSIGNED_BYTE, 
            (const char*)(m_yuv420pBuf + m_width * m_height * 5 / 4)
        );
        
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // 指定y纹理要使用新值
    glUniform1i(textureUniformY, 0);
    // 指定u纹理要使用新值
    glUniform1i(textureUniformU, 1);
    // 指定v纹理要使用新值
    glUniform1i(textureUniformV, 2);

    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#endif
