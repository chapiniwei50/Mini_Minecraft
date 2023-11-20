#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : m_countOpq(-1), m_countTra(-1), m_bufIdxOpq(), m_bufIdxTra(), m_bufDataOpq(), m_bufDataTra(), m_bufUV(),m_bufPos(),
    m_idxOpqGenerated(false), m_idxTraGenerated(false), m_bufDataOpqGenerated(false), m_bufDataTraGenerated(false), m_UVGenerated(false), m_PosGenerated(false),
      mp_context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroyVBOdata()
{
    mp_context->glDeleteBuffers(1, &m_bufIdxOpq);
    mp_context->glDeleteBuffers(1, &m_bufIdxTra);
    mp_context->glDeleteBuffers(1, &m_bufDataOpq);
    mp_context->glDeleteBuffers(1, &m_bufDataTra);
    mp_context->glDeleteBuffers(1, &m_bufUV);
    mp_context->glDeleteBuffers(1, &m_bufPos);

    m_idxOpqGenerated = m_idxTraGenerated = m_bufDataOpqGenerated = m_bufDataTraGenerated = m_bufUV = m_bufPos = false;
    m_countOpq = m_countTra = -1;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemOpqCount()
{
    return m_countOpq;
}

int Drawable::elemTraCount()
{
    return m_countTra;
}

void Drawable::generateIdxOpq()
{
    m_idxOpqGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxOpq);
}

void Drawable::generateIdxTra()
{
    m_idxTraGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxTra);
}

void Drawable::generateDataOpq()
{
    m_bufDataOpqGenerated = true;
    mp_context->glGenBuffers(1, &m_bufDataOpq);
}

void Drawable::generateDataTra()
{
    m_bufDataTraGenerated = true;
    mp_context->glGenBuffers(1, &m_bufDataTra);
}

void Drawable::generateUV()
{
    m_UVGenerated = true;
    mp_context->glGenBuffers(1, &m_bufUV);
}

void Drawable::generatePos()
{
    m_PosGenerated = true;
    mp_context->glGenBuffers(1, &m_bufPos);
}

bool Drawable::bindIdxOpq()
{
    if(m_idxOpqGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    }
    return m_idxOpqGenerated;
}

bool Drawable::bindIdxTra()
{
    if(m_idxTraGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
    }
    return m_idxTraGenerated;
}

bool Drawable::bindDataOpq()
{
    if(m_bufDataOpqGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufDataOpq);
    }
    return m_bufDataOpqGenerated;
}

bool Drawable::bindDataTra()
{
    if(m_bufDataTraGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufDataTra);
    }
    return m_bufDataTraGenerated;
}
bool Drawable::bindUV()
{
    if(m_UVGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    }
    return m_UVGenerated;
}


bool Drawable::bindPos()
{
    if(m_PosGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    }
    return m_PosGenerated;
}

//InstancedDrawable::InstancedDrawable(OpenGLContext *context)
//    : Drawable(context), m_numInstances(0), m_bufPosOffset(-1), m_offsetGenerated(false)
//{}

//InstancedDrawable::~InstancedDrawable(){}

//int InstancedDrawable::instanceCount() const {
//    return m_numInstances;
//}

//void InstancedDrawable::generateOffsetBuf() {
//    m_offsetGenerated = true;
//    mp_context->glGenBuffers(1, &m_bufPosOffset);
//}

//bool InstancedDrawable::bindOffsetBuf() {
//    if(m_offsetGenerated){
//        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOffset);
//    }
//    return m_offsetGenerated;
//}


//void InstancedDrawable::clearOffsetBuf() {
//    if(m_offsetGenerated) {
//        mp_context->glDeleteBuffers(1, &m_bufPosOffset);
//        m_offsetGenerated = false;
//    }
//}
//void InstancedDrawable::clearColorBuf() {
//    if(m_colGenerated) {
//        mp_context->glDeleteBuffers(1, &m_bufCol);
//        m_colGenerated = false;
//    }
//}
