#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "shaderprogram.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include "smartpointerhelp.h"
#include "scene/quad.h"
#include "framebuffer.h"



#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObjectFormat>
#include <smartpointerhelp.h>
#include <QDir>
#include <QString>
#include <ctime>

class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    ShaderProgram m_progLambert;// A shader program that uses lambertian reflection
    ShaderProgram m_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram m_WLoverlay; // A shader program for overlay of being underwater and underlava
    ShaderProgram m_depth; //

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
        // Don't worry too much about this. Just know it is necessary in order to render geometry.

    GLuint fbo;

    // settings about shadow mapping
    GLuint shadow_mapping_fbo;
    int shad_width = 1258 * 8; //this->width();
    int shad_height = 916 * 8; //this->height();
    GLuint shadow_mapping_texture;

    Terrain m_terrain; // All of the Chunks that currently comprise the world.
    Player m_player; // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs; // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.

    QTimer m_timer; // Timer linked to tick(). Fires approximately 60 times per second.


    QPoint lastMousePosition;
    qint64 m_lastTime;
    FrameBuffer m_frameBuffer;
    Quad m_geomQuad;

    void moveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
        // from within a mouse move event after reading the mouse movement so that
        // your mouse stays within the screen bounds and is always read.
    void sendPlayerDataToGUI() const;

    int m_time; // another timer for shader programs cuz I don't know how to use QTimer haha.

    void setDepthFBO();
    void set_shadow_mapping();

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrain();
    void renderDepthView();
    void renderOverlay();
    void renderShadowMappingDepth();

    void visualize();

    // about shadow mapping
    glm::vec3 lightInvDir = glm::vec3(10.f, 250.f, 10.f);
    glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir + m_player.mcr_position, m_player.mcr_position, glm::vec3(0.f, 1.f, 0.f));
    // at max_height, the half width of the projection matrix should be max_half_width, and vice versa
    float max_half_width = 200.f;
    float min_half_width = 64.f;
    float max_height = 50.f;
    float min_height = 0.f;
    glm::mat4 depthProjMatrix = glm::ortho<float>(-max_half_width, max_half_width, -max_half_width, max_half_width, 0.1f, 1000.f);
    glm::mat4 LightSpaceMatrix = depthProjMatrix * depthViewMatrix;
    glm::vec3 axis = glm::vec3(1.1f, 0.f, 0.9f);
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), glm::radians(0.1f), axis);
    void update_light_vector();

public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;



protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e);
    // Automatically invoked when the user
    // moves the mouse
    void mouseMoveEvent(QMouseEvent *e);
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;
};


#endif // MYGL_H
