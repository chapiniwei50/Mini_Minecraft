#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <qdatetime.h>



MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
    m_progLambert(this), m_progFlat(this), m_depth(this),m_progSky(this),
    m_terrain(this), m_player(glm::vec3(32.f, 255.f, 32.f), m_terrain), m_lastTime(QDateTime::currentMSecsSinceEpoch()),m_WLoverlay(this), m_geomQuad(this),
    m_frameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
    m_time(0)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    m_frameBuffer.destroy();
    m_geomQuad.destroyVBOdata();
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::setDepthFBO(){
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_frameBuffer.m_depthTexture, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGL::set_shadow_mapping(){
    // create the buffer
    glGenFramebuffers(1, &shadow_mapping_fbo);

    // create the texture
    glGenTextures(1, &shadow_mapping_texture);
    glBindTexture(GL_TEXTURE_2D, shadow_mapping_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 shad_width, shad_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_mapping_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_mapping_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();
    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    // enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    m_frameBuffer.create();
    setDepthFBO();
    set_shadow_mapping();
    m_geomQuad.createVBOdata();

    m_WLoverlay.create(":/glsl/WLoverlay.vert.glsl", ":/glsl/WLoverlay.frag.glsl");
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_depth.create(":/glsl/depth.vert.glsl", ":/glsl/depth.frag.glsl");
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");

    //m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    m_terrain.create_load_texture(":/textures/minecraft_textures_all.png");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    lastMousePosition = QPoint(0, 0);

    m_terrain.initialTerrainGeneration(m_player.mcr_position);
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);

    m_progSky.setViewProjMatrix(glm::inverse(viewproj));
    m_progSky.useMe();
    glUniform2i(m_progSky.unifDimensions, width(), height());
    glUniform3f(m_progSky.unifEye, m_player.mcr_position.x, m_player.mcr_position.y, m_player.mcr_position.z);

    m_frameBuffer.resize(this->width(), this->height(), this->devicePixelRatio());
    m_frameBuffer.destroy();
    m_frameBuffer.create();

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    float deltaT = (currentTime - m_lastTime) * 0.001;
    m_lastTime = currentTime;
    m_player.tick(deltaT, m_inputs);
    m_terrain.multithreadedTerrainUpdate(m_player.mcr_position, m_player.mcr_lastFramePosition);
    m_time++;
    update_light_vector();
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    //renderDepthView();

    renderShadowMappingDepth();
    //renderSkybox();
    renderTerrain();
    renderOverlay();
}

void MyGL::renderSkybox(){
    m_progSky.useMe();
    m_progSky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    m_progSky.setCameraPosition(m_player.mcr_camera.mcr_position);
    m_progSky.setTime(m_time);
    m_progSky.drawSkybox(m_geomQuad);
}

void MyGL::renderShadowMappingDepth() {
    glViewport(0, 0, shad_width, shad_height);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_mapping_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    // render scene
    m_depth.setModelMatrix(glm::mat4(1.0f));
    m_depth.setViewProjMatrix(LightSpaceMatrix);

    int x = static_cast<int>(floor(m_player.mcr_position.x / 16.f) * 16);
    int z = static_cast<int>(floor(m_player.mcr_position.z / 16.f) * 16);
    int drawBlockSize = m_terrain.zoneRadius * 32;
    m_terrain.draw(x - drawBlockSize, x + drawBlockSize, z - drawBlockSize, z + drawBlockSize, &m_depth, true);

    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)f
void MyGL::renderTerrain() {

    m_frameBuffer.bindFrameBuffer();
    glViewport(0, 0, this->width(), this->height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    m_progFlat.useMe();

    m_progFlat.setLightSpaceMatrix(LightSpaceMatrix);
    // activate shadow mapping depth texture
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, shadow_mapping_texture);
    GLuint shadMapDepthUnif = glGetUniformLocation(m_progFlat.prog, "u_ShadowMappingDepth");
    glUniform1i(shadMapDepthUnif, 2);

    int x = static_cast<int>(floor(m_player.mcr_position.x / 16.f) * 16);
    int z = static_cast<int>(floor(m_player.mcr_position.z / 16.f) * 16);

    int drawBlockSize = m_terrain.zoneRadius * 32;
    // draw opaque
    m_terrain.draw(x - drawBlockSize, x + drawBlockSize, z - drawBlockSize, z + drawBlockSize, &m_progFlat, true);
    // draw transparent
    m_terrain.draw(x - drawBlockSize, x + drawBlockSize, z - drawBlockSize, z + drawBlockSize, &m_progFlat, false);

    // update time for shader
    m_progFlat.setTime(m_time);
    m_progFlat.setCameraPosition(m_player.mcr_camera.mcr_position);

    //set model and view matrix
    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    glEnable(GL_DEPTH_TEST);
}

void MyGL::renderOverlay(){
    // If in water
    if (m_terrain.m_chunks.size()>0 && m_player.isInWater(m_terrain, m_inputs)) {
        m_WLoverlay.seteffectType(1);
        // If in lava
    } else if (m_terrain.m_chunks.size()>0 &&m_player.isInLava(m_terrain, m_inputs)) {
        m_WLoverlay.seteffectType(2);
        // If in air
    } else {
        m_WLoverlay.seteffectType(0);

    }
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width() *  this->devicePixelRatio(), this->height() *  this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_WLoverlay.useMe();
    m_WLoverlay.setTime(m_time);
    m_frameBuffer.bindToTextureSlot(0);
    m_WLoverlay.drawEffect(m_geomQuad);
}


void MyGL::visualize(){
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width(), this->height());
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_mapping_texture);
    m_WLoverlay.useMe();
    GLuint shadMapDepthUnif = glGetUniformLocation(m_WLoverlay.prog, "u_RenderedTexture");
    glUniform1i(shadMapDepthUnif, 0);

    m_WLoverlay.drawEffect(m_geomQuad);
}

void MyGL::renderDepthView(){
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, this->width(), this->height());
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    m_progFlat.useMe();
    m_frameBuffer.bindToTextureSlot(1);
    GLint depthTexUniform = glGetUniformLocation(m_progFlat.prog, "u_DepthTexture");
    glUniform1i(depthTexUniform, 1);

    m_depth.useMe();
    int x = static_cast<int>(floor(m_player.mcr_position.x / 16.f) * 16);
    int z = static_cast<int>(floor(m_player.mcr_position.z / 16.f) * 16);
    // draw opaque
    int drawBlockSize = m_terrain.zoneRadius * 32;
    m_terrain.draw(x - drawBlockSize, x + drawBlockSize, z - drawBlockSize, z + drawBlockSize, &m_depth, true);

    glDisable(GL_DEPTH_TEST);
    m_depth.setModelMatrix(glm::mat4());
    m_depth.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
}


void MyGL::update_light_vector() {
    if (m_time % 2 == 0) {
        lightInvDir = glm::vec3(rotMat * glm::vec4(lightInvDir, 0));
//        if (glm::dot(lightInvDir, glm::vec3(0.f, 1.f, 0.f)) < 0)  // let the light always be above the horizon.
//            lightInvDir *= -1;
    }

    // set the projection matrix according to player height
    // so that it can have higher resolution when the player is near ground
    float curr_height = m_player.getHeight(m_terrain);
    curr_height = curr_height > max_height ? max_height : curr_height;
    float half_width = glm::mix(
        min_half_width, max_half_width,
        (curr_height - min_height) / (max_height - min_height)
    );
    depthProjMatrix = glm::ortho<float>(-half_width, half_width, -half_width, half_width, 0.1f, 1000.f);

    // send the height value to gpu
    m_progFlat.useMe();
    int unifHeight = glGetUniformLocation(m_progFlat.prog, "u_height");
    glUniform1f(unifHeight, curr_height);


    depthViewMatrix = glm::lookAt(lightInvDir + m_player.mcr_position, m_player.mcr_position, glm::vec3(0.f, 1.f, 0.f));
    LightSpaceMatrix = depthProjMatrix * depthViewMatrix;
    m_progFlat.setLightDirection(lightInvDir);
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_F) {
        if( m_inputs.flight_mode){
            m_inputs.flight_mode = false;
            //std::cout <<"flight mode off"<< std::endl;
        }
        else{
            m_inputs.flight_mode = true;
            //std::cout <<"flight mode on"<< std::endl;

        }

    }
    //flight mode
    if (m_inputs.flight_mode) {
        if (e->key() == Qt::Key_Q) {
            m_inputs.qPressed = true;
        } else if (e->key() == Qt::Key_E) {
            m_inputs.ePressed = true;
        }
    }else{
        if (e->key() == Qt::Key_Space) {
            m_inputs.spacePressed = true;
        } else if (e->key() == Qt::Key_Shift){

        }
    }

}
void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (!e->isAutoRepeat()) {
        if (e->key() == Qt::Key_W) {
            m_inputs.wPressed = false;
        } else if (e->key() == Qt::Key_S) {
            m_inputs.sPressed = false;
        } else if (e->key() == Qt::Key_D) {
            m_inputs.dPressed = false;
        } else if (e->key() == Qt::Key_A) {
            m_inputs.aPressed = false;
        } else if (e->key() == Qt::Key_Q) {
            m_inputs.qPressed = false;
        } else if (e->key() == Qt::Key_E) {
            m_inputs.ePressed = false;
        } else if (e->key() == Qt::Key_Space) {
            m_inputs.spacePressed = false;
        }
    }
}
void MyGL::mouseMoveEvent(QMouseEvent *e) {

#ifdef Q_OS_WIN

    QPoint mousePos = e->pos();
    QPoint centerPos = QPoint(width() / 2, height() / 2);
    if (this->hasFocus()){
        m_inputs.mouseX = centerPos.x() - mousePos.x();
        m_inputs.mouseY = centerPos.y() - mousePos.y();
        moveMouseToCenter();
    } else {
        m_inputs.mouseX = 0;
        m_inputs.mouseY = 0;
    }
    m_player.processCameraRotation(m_inputs.mouseX,m_inputs.mouseY);

#endif

#ifdef Q_OS_MAC
    float dx = e->pos().x() - lastMousePosition.x();
    float dy = e->pos().y() - lastMousePosition.y();
    lastMousePosition = e->pos();
    m_player.rotateOnUpGlobal(dx);
    m_player.rotateOnRightLocal(dy);
#endif

}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_player.removeBlock();
    }
    if (e->button() == Qt::RightButton) {
        m_player.addBlock(&m_terrain);
    }
}
