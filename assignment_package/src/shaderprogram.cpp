#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QDebug>
#include <stdexcept>
#include <iostream>



ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
    attrPos(-1), attrNor(-1), attrCol(-1), attrUV(-1), attrUVFrameBuffer(-1),
    unifModel(-1), unifModelInvTr(-1), unifViewProj(-1), unifColor(-1),
    unifLightSpaceMatrix(-1), unifLightDirection(-1), unifEffectType(-1),
    unifSampler2D(-1), unifSamplerFrameBuffer(-1),
    unifTime(-1), unifCameraPos(-1),
      context(context)
{}

void ShaderProgram::create(const char *vertfile, const char *fragfile)
{
    // Allocate space on our GPU for a vertex shader and a fragment shader and a shader program to manage the two
    vertShader = context->glCreateShader(GL_VERTEX_SHADER);
    fragShader = context->glCreateShader(GL_FRAGMENT_SHADER);
    prog = context->glCreateProgram();
    // Get the body of text stored in our two .glsl files
    QString qVertSource = qTextFileRead(vertfile);
    QString qFragSource = qTextFileRead(fragfile);

    char* vertSource = new char[qVertSource.size()+1];
    strcpy(vertSource, qVertSource.toStdString().c_str());
    char* fragSource = new char[qFragSource.size()+1];
    strcpy(fragSource, qFragSource.toStdString().c_str());


    // Send the shader text to OpenGL and store it in the shaders specified by the handles vertShader and fragShader
    context->glShaderSource(vertShader, 1, (const char**)&vertSource, 0);
    context->glShaderSource(fragShader, 1, (const char**)&fragSource, 0);
    // Tell OpenGL to compile the shader text stored above
    context->glCompileShader(vertShader);
    context->glCompileShader(fragShader);
    // Check if everything compiled OK
    GLint compiled;
    context->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(vertShader);
    }
    context->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(fragShader);
    }

    // Tell prog that it manages these particular vertex and fragment shaders
    context->glAttachShader(prog, vertShader);
    context->glAttachShader(prog, fragShader);
    context->glLinkProgram(prog);

    // Check for linking success
    GLint linked;
    context->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(prog);
    }

    // Get the handles to the variables stored in our shaders
    // See shaderprogram.h for more information about these variables

    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    attrNor = context->glGetAttribLocation(prog, "vs_Nor");
    attrCol = context->glGetAttribLocation(prog, "vs_Col");
    if(attrCol == -1) attrCol = context->glGetAttribLocation(prog, "vs_ColInstanced");
    attrPosOffset = context->glGetAttribLocation(prog, "vs_OffsetInstanced");
    attrUV = context->glGetAttribLocation(prog, "vs_UV");
    attrUVFrameBuffer = context->glGetAttribLocation(prog, "vs_UVFrameBuffer");

    unifModel      = context->glGetUniformLocation(prog, "u_Model");
    unifModelInvTr = context->glGetUniformLocation(prog, "u_ModelInvTr");
    unifViewProj   = context->glGetUniformLocation(prog, "u_ViewProj");
    unifColor      = context->glGetUniformLocation(prog, "u_Color");
    unifEffectType = context->glGetUniformLocation(prog, "u_EffectType");
    unifLightSpaceMatrix = context->glGetUniformLocation(prog, "u_LightSpaceMatrix");
    unifLightDirection = context->glGetUniformLocation(prog, "u_LightDirection");

    unifSampler2D  = context->glGetUniformLocation(prog, "u_Texture");
    unifSamplerFrameBuffer = context->glGetUniformLocation(prog, "u_RenderedTexture");
    unifTime = context->glGetUniformLocation(prog, "u_Time");
    unifCameraPos = context->glGetUniformLocation(prog, "u_CameraPos");

    unifDimensions = context->glGetUniformLocation(prog, "u_Dimensions");
    unifEye = context->glGetUniformLocation(prog, "u_Eye");

    context->printGLErrorLog();
}

void ShaderProgram::useMe()
{
    context->glUseProgram(prog);
}

void ShaderProgram::setCameraPosition(const glm::vec3 &camPos)
{
    useMe();

    if (unifCameraPos != -1) {
        context->glUniform3fv(unifCameraPos, 1, &camPos[0]);
    }
}

void ShaderProgram::setLightDirection(const glm::vec3 &lightDirection)
{
    useMe();

    if (unifLightDirection != -1) {
        context->glUniform3fv(unifLightDirection, 1, &lightDirection[0]);
    }
}


void ShaderProgram::setTime(int t)
{
    useMe();

    if(unifTime != -1)
    {
        context->glUniform1i(unifTime, t);
    }
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model)
{
    useMe();

    if (unifModel != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModel,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &model[0][0]);
    }

    if (unifModelInvTr != -1) {
        glm::mat4 modelinvtr = glm::inverse(glm::transpose(model));
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModelInvTr,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &modelinvtr[0][0]);
    }
}

void ShaderProgram::setViewProjMatrix(const glm::mat4 &vp)
{
    // Tell OpenGL to use this shader program for subsequent function calls
    useMe();

    if(unifViewProj != -1) {
    // Pass a 4x4 matrix into a uniform variable in our shader
                    // Handle to the matrix variable on the GPU
    context->glUniformMatrix4fv(unifViewProj,
                    // How many matrices to pass
                       1,
                    // Transpose the matrix? OpenGL uses column-major, so no.
                       GL_FALSE,
                    // Pointer to the first element of the matrix
                       &vp[0][0]);
    }
}

void ShaderProgram::setLightSpaceMatrix(const glm::mat4 &m)
{
    // Tell OpenGL to use this shader program for subsequent function calls
    useMe();

    if(unifLightSpaceMatrix != -1) {
    // Pass a 4x4 matrix into a uniform variable in our shader
    // Handle to the matrix variable on the GPU
    context->glUniformMatrix4fv(unifLightSpaceMatrix,
                                // How many matrices to pass
                                1,
                                // Transpose the matrix? OpenGL uses column-major, so no.
                                GL_FALSE,
                                // Pointer to the first element of the matrix
                                &m[0][0]);
    }
}

void ShaderProgram::setGeometryColor(glm::vec4 color)
{
    useMe();

    if(unifColor != -1)
    {
        context->glUniform4fv(unifColor, 1, &color[0]);
    }
}

void ShaderProgram::seteffectType(const int type)
{
    useMe();

    if(unifEffectType != -1)
    {
        context->glUniform1i(unifEffectType, type);
    }
}

void ShaderProgram::drawInterleaved(Drawable *d, bool opaque, int textureSlot)
{
    useMe();

    if(d->elemOpqCount() + d->elemTraCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d->elemOpqCount() + d->elemTraCount()) + "!");
    }

    if(unifSampler2D != -1)
    {
        context->glUniform1i(unifSampler2D, /*GL_TEXTURE*/textureSlot);
    }

    if (opaque)
    {
        if (d->bindDataOpq())
        {
            if (attrPos != -1)
            {
                context->glEnableVertexAttribArray(attrPos);
                context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(0));
            }

            if (attrNor != -1)
            {
                context->glEnableVertexAttribArray(attrNor);
                context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(sizeof(glm::vec4)));
            }

            if (attrUV != -1)
            {
                context->glEnableVertexAttribArray(attrUV);
                context->glVertexAttribPointer(attrUV, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(2 * sizeof(glm::vec4)));
            }
        }

        // Bind the index buffer and then draw shapes from it.
        // This invokes the shader program, which accesses the vertex buffers.
        d->bindIdxOpq();
        context->glDrawElements(d->drawMode(), d->elemOpqCount(), GL_UNSIGNED_INT, 0);
    }
    else
    {
        if (d->bindDataTra())
        {
            if (attrPos != -1)
            {
                context->glEnableVertexAttribArray(attrPos);
                context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(0));
            }

            if (attrNor != -1)
            {
                context->glEnableVertexAttribArray(attrNor);
                context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(sizeof(glm::vec4)));
            }

            if (attrUV != -1)
            {
                context->glEnableVertexAttribArray(attrUV);
                context->glVertexAttribPointer(attrUV, 4, GL_FLOAT, false, sizeof(glm::vec4) * 3, (void*)(2 * sizeof(glm::vec4)));
            }
        }

        // Bind the index buffer and then draw shapes from it.
        // This invokes the shader program, which accesses the vertex buffers.
        d->bindIdxTra();
        context->glDrawElements(d->drawMode(), d->elemTraCount(), GL_UNSIGNED_INT, 0);
    }

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);

    context->printGLErrorLog();
}


char* ShaderProgram::textFileRead(const char* fileName) {
    char* text;

    if (fileName != NULL) {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol, fixed by Cory
            }
            fclose(file);
        }
    }
    return text;
}

QString ShaderProgram::qTextFileRead(const char *fileName)
{
    QString text;
    QFile file(fileName);
    if(file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

void ShaderProgram::printShaderInfoLog(int shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        qDebug() << "ShaderInfoLog:" << "\n" << infoLog << "\n";
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}

void ShaderProgram::printLinkInfoLog(int prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        qDebug() << "LinkInfoLog:" << "\n" << infoLog << "\n";
        delete [] infoLog;
    }
}


void ShaderProgram::drawEffect(Drawable &d){

    useMe();

    // Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    // Remember, by calling bindPos(), we call
    // glBindBuffer on the Drawable's VBO for vertex position,
    // meaning that glVertexAttribPointer associates vs_Pos
    // (referred to by attrPos) with that VBO
    if(d.elemOpqCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d.elemOpqCount()) + "!");
    }

    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
    }
    if (attrUVFrameBuffer != -1 && d.bindUV()) {
        context->glEnableVertexAttribArray(attrUVFrameBuffer);
        context->glVertexAttribPointer(attrUVFrameBuffer, 2, GL_FLOAT, false, 0, NULL);
    }


    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdxOpq();
    context->glDrawElements(d.drawMode(), d.elemOpqCount(), GL_UNSIGNED_INT, 0);
    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrUVFrameBuffer != -1) context->glDisableVertexAttribArray(attrUVFrameBuffer);
    context->printGLErrorLog();
}

void ShaderProgram::drawSkybox(Drawable &d){
    useMe();

    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
    }

    d.bindIdxOpq();
    context->glDrawElements(d.drawMode(), d.elemOpqCount(), GL_UNSIGNED_INT, 0);
    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);

    context->printGLErrorLog();
}
