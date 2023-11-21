#pragma once

#include <glm/glm.hpp>
#include "entity.h"
#include "camera.h"
#include "terrain.h"

class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration, m_lastFramePosition;
    Camera m_camera;
    Terrain &mcr_terrain;

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain, InputBundle &input);
    void terrain_collision_check(glm::vec3 *rayDir, const Terrain &terrain);
    bool isBlockAt(glm::vec3& position, const Terrain& terrain);
    bool isOnGround(const Terrain &terrain, InputBundle &input);
    bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit, glm::ivec3 *prevCell = nullptr);
    glm::ivec3 computeFaceNormal(const glm::ivec3 &blockPos, const glm::vec3 &collisionPoint);


public:
    const glm::vec3& mcr_lastFramePosition;
    // Readonly public reference to our camera
    // for easy access from MyGL
    const Camera& mcr_camera;
    glm::vec2 m_cameraOrientation;

    Player(glm::vec3 pos, Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;
     bool isInWater(const Terrain &terrain, InputBundle &input);
    bool isInLava(const Terrain &terrain, InputBundle &input);


    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;

    void removeBlock();
    void addBlock(Terrain *terrain);
};
