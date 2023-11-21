

#pragma once
#include "drawable.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Quad : public Drawable
{
public:
    Quad(OpenGLContext* context) : Drawable(context){}
    virtual ~Quad() override;
    void createVBOdata() override;
    GLenum drawMode() override;

};



