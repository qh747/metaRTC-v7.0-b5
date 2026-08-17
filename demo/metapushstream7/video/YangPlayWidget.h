//
// Copyright (c) 2019-2022 yanggaofeng
//
#ifndef YangPlayWidget_H
#define YangPlayWidget_H

#if !defined(__APPLE__)
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

class YangPlayWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    YangPlayWidget(QWidget* parent);
    ~YangPlayWidget();

public:
    void playVideo(unsigned char* buf, int width, int height);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;

private:
    // y纹理数据位置
    GLuint textureUniformY;
    // u纹理数据位置
    GLuint textureUniformU; 
    // v纹理数据位置
    GLuint textureUniformV; 
    
    // y纹理对象ID
    GLuint id_y;
    // u纹理对象ID
    GLuint id_u; 
    // v纹理对象ID
    GLuint id_v; 
    
    // y纹理对象
    QOpenGLTexture* m_pTextureY;
    // u纹理对象
    QOpenGLTexture* m_pTextureU;
    // v纹理对象
    QOpenGLTexture* m_pTextureV;
   
    // 顶点着色器程序对象
    QOpenGLShader* m_pVSHader;
    // 片段着色器对象
    QOpenGLShader* m_pFSHader;  
    
    // 着色器程序容器
    QOpenGLShaderProgram* m_pShaderProgram; 
    
    // 视频分辨率宽
    int m_width; 
    // 视频分辨率高
    int m_height; 
    
    unsigned char* m_yuv420pBuf;
};

#endif // !defined(__APPLE__)
#endif // CPLAYWIDGET_H
