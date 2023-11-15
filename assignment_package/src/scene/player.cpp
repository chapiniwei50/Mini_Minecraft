#include "player.h"
#include <QString>
#include <iostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
    m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
    mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain, input);
}

void Player::processInputs(InputBundle &inputs) {

    float acc = 500.f;
    m_acceleration = glm::vec3(0);

    if(inputs.flight_mode){
        if (inputs.wPressed) {
            m_acceleration = acc * this->m_forward;
        } else if (inputs.sPressed) {
            m_acceleration = -acc * this->m_forward;
        } else if (inputs.dPressed) {
            m_acceleration = acc * this->m_right;
        } else if (inputs.aPressed) {
            m_acceleration = -acc * this->m_right;
        } else if (inputs.ePressed) {
            m_acceleration = acc * this->m_up;
        } else if (inputs.qPressed) {
            m_acceleration = -acc * this->m_up;
        }
    }
    else{
        if (inputs.wPressed) {
            m_forward.y = 0;
            m_acceleration = acc * glm::normalize(glm::vec3(m_forward.x, 0, m_forward.z));
        } else if (inputs.sPressed) {
            m_forward.y = 0;
            m_acceleration = -acc * glm::normalize(glm::vec3(m_forward.x, 0, m_forward.z));
        } else if (inputs.dPressed) {
            m_right.y = 0;
            m_acceleration = acc * glm::normalize(glm::vec3(m_right.x, 0, m_right.z));
        } else if (inputs.aPressed) {
            m_right.y = 0;
            m_acceleration = -acc * glm::normalize(glm::vec3(m_right.x, 0, m_right.z));
        } else if (inputs.spacePressed) {
            m_velocity.y = 40.f;
        }

    }
}
bool Player::isBlockAt( glm::vec3& position, const Terrain& terrain) {
    return terrain.getBlockAt(position) != EMPTY;
}

bool Player::isOnGround( const Terrain &terrain, InputBundle &input) {
    //std::cout << "entering isOnGround" << std::endl;

    glm::vec3 corner = this->m_position - glm::vec3(0.5f, 0, 0.5f);
    for (int x = 0; x <= 1; ++x) {
        for (int z = 0; z <= 1; ++z) {
            glm::vec3 checkPos = glm::vec3(floor(corner.x) + x,
                                           floor(corner.y) - 0.01f, // slightly below the player to ensure the block is indeed beneath
                                           floor(corner.z) + z);
            return isBlockAt(checkPos, terrain);
        }
    }
    return false; // return false if no ground is found after checking all corners
}

void Player::computePhysics(float dT, const Terrain &terrain, InputBundle &input) {

    // TODO: Update the Player's position based on its
    // and velocity, and also perform collision detection.

    const float friction = 0.9f;
    const glm::vec3 gravity = glm::vec3(0.f, -9.8f, 0.f);
    if(!input.flight_mode){
        m_acceleration += gravity;
    }
    m_velocity *= friction;
    glm::vec3 rayDir = m_velocity * dT;
    terrain_collision_check(&rayDir, terrain);

    this->moveAlongVector(rayDir);
    m_velocity += m_acceleration * dT;

    /*
    if(!input.flight_mode){
        if(isOnGround(terrain, input)){
            m_velocity.y = 0;
        }
        else{
            const glm::vec3 gravity = glm::vec3(0.f, -9.8f, 0.f);
            m_velocity += gravity * dT;
            m_acceleration += gravity;
        }
    }*/

}

void Player::terrain_collision_check(glm::vec3 *rayDir, const Terrain &terrain) {
    glm::vec3 playerMin = this->m_position - glm::vec3(0.5f, 0.f, 0.5f);
    glm::vec3 playerMax = this->m_position + glm::vec3(0.5f, 2.f, 0.5f);

    if(!(terrain.hasChunkAt(playerMax.x,playerMax.z))){
        return;
    }

    glm::ivec3 collisionPoint = glm::ivec3();
    float collisionDist = 0.f;

    for (int x = 0; x <= 1; x++) {
        for (int z = 0; z >= -1; z--) {
            for (int y = 0; y <= 2; y++) {
                glm::vec3 corner = playerMin + glm::vec3(x, y, z);
                try {
                    if (gridMarch(terrain, &collisionDist, &collisionPoint, corner, *rayDir, glm::length(m_velocity))) {

                        float currentDistanceToCollision = glm::abs(glm::length(this->m_position - glm::vec3(collisionPoint)));
                        float safeCollisionResponseDistance = collisionDist - 0.01f;
                        float distance = glm::min(safeCollisionResponseDistance, currentDistanceToCollision);

                        *rayDir = distance * glm::normalize(*rayDir);



                    }
                } catch(std::exception &e) {
                    std::cout << "Exception encountered:" << e.what() << std::endl;
                }
            }
        }
    }
}

bool Player::gridMarch(const Terrain &terrain,
                       float *collisionDist, glm::ivec3 *collisionPoint,
                       glm::vec3 rayOrigin,  glm::vec3 rayDir, float maxMarchLength) {

    rayDir = glm::length(rayDir) > 0 ? glm::normalize(rayDir) : rayDir; // world dist

    glm::vec3 rayStepSize = glm::sign(rayDir) * glm::vec3(1.0f);
    glm::vec3 rayLength1D;
    glm::ivec3 mapCheck = glm::ivec3(rayOrigin);
    glm::ivec3 stepDir;

    for (int i = 0; i < 3; ++i) {
        if (rayDir[i] == 0.0f) {
            rayLength1D[i] = INFINITY;
        } else {
            rayLength1D[i] = abs(1 / rayDir[i]);
            stepDir[i] = (rayDir[i] > 0) ? 1 : -1;
        }
    }
    glm::vec3 nextBoundary = rayOrigin;
    for (int i = 0; i < 3; ++i) {
        if (rayDir[i] > 0) {
            nextBoundary[i] = ceil(rayOrigin[i]) - rayOrigin[i];
        } else if (rayDir[i] < 0) {
            nextBoundary[i] = rayOrigin[i] - floor(rayOrigin[i]);
        }
    }

    float currRayLength = 0; // how far along the ray we've traveled
    while(currRayLength < maxMarchLength){

        if(terrain.getBlockAt(mapCheck.x, mapCheck.y, mapCheck.z) != EMPTY)
        {
            // if we hit a block
            *collisionDist = currRayLength;
            *collisionPoint = glm::vec3(mapCheck);
            return true; // collision detected
        }

        // find which plane we hit
        glm::vec3 deltaDist; // distance to the next boundary crossing for each axis
        for (int i = 0; i < 3; ++i) {
            deltaDist[i] = (nextBoundary[i] * rayLength1D[i]);
        }
        // select the smallest distance to move along the ray
        if (deltaDist.x < deltaDist.y && deltaDist.x < deltaDist.z) {
            mapCheck.x += stepDir.x;
            currRayLength += glm::abs(deltaDist.x);
            nextBoundary.x += rayStepSize.x;
        } else if (deltaDist.y < deltaDist.z) {
            mapCheck.y += stepDir.y;
            currRayLength += glm::abs(deltaDist.y);
            nextBoundary.y += rayStepSize.y;
        } else {
            mapCheck.z += stepDir.z;
            currRayLength += glm::abs(deltaDist.z);
            nextBoundary.z += rayStepSize.z;
        }

}

void Player::addBlock(Terrain *terrain) {
    glm::vec3 corner = m_camera.mcr_position;
    glm::vec3 rayDir =  3.f *glm::normalize(this->m_forward);
    float maxDistance = 3.f;
    float collDist;
    glm::ivec3 collHit;
    //std::cout<<"add block"<<std::endl;
    if (gridMarch(*terrain, &collDist, &collHit, corner, rayDir * maxDistance, 3.0f)) {
        if(collDist < maxDistance){
            // determine the face of the block that was hit
            glm::vec3 collisionPoint = corner + rayDir * collDist;
            glm::ivec3 faceNormal = computeFaceNormal(collHit, collisionPoint);

            // new position for new block
            glm::ivec3 newBlockPos = collHit + faceNormal;

            // check if the position where we want to add the new block is empty
            if(terrain->getBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z) == EMPTY) {
                BlockType blockType = DIRT;
                terrain->setBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z, blockType);

                // update chunk
                uPtr<Chunk>& chunk = terrain->getChunkAt(newBlockPos.x, newBlockPos.z);
                chunk->destroyVBOdata();
                chunk->createVBOdata();
            }
        }
    }


}

glm::ivec3 Player::computeFaceNormal(const glm::ivec3 &blockPos, const glm::vec3 &collisionPoint) {
    glm::ivec3 faceNormal(0, 0, 0);

    // relative position of the collision point to the block position
    glm::vec3 relativeCollisionPoint = collisionPoint - glm::vec3(blockPos);

    // how far the collision point is from the center of each face
    glm::vec3 distances = glm::abs(relativeCollisionPoint - 0.5f);

    // find max distance component to determine the face that was hit
    // 0 for x, 1 for y, 2 for z
    float maxDist = distances.x;
    int maxIndex = 0;

    if(distances.y > maxDist) {
        maxDist = distances.y;
        maxIndex = 1;
    }
    if(distances.z > maxDist) {
        maxIndex = 2;
    }
    if (maxIndex == 0) {
        faceNormal.x = (relativeCollisionPoint.x > 0.5f) ? 1 : -1;
    } else if (maxIndex == 1) {
        faceNormal.y = (relativeCollisionPoint.y > 0.5f) ? 1 : -1;
    } else {
        faceNormal.z = (relativeCollisionPoint.z > 0.5f) ? 1 : -1;
    }

    return faceNormal;

}
void Player::removeBlock(Terrain *terrain) {

    // Implement ray casting and grid marching to find and remove the block

    glm::vec3 corner = m_camera.mcr_position;
    glm::vec3 rayDir =  3.f *glm::normalize(this->m_forward);
    float maxDistance = 3.f;

    float collDist;
    glm::ivec3 collHit;

    if (gridMarch(*terrain, &collDist, &collHit, corner, rayDir, 3.0f)) {
        if(collDist < maxDistance){

            BlockType removedBlockType = terrain->getBlockAt(collHit.x, collHit.y, collHit.z);
            // remove the block by setting its type to EMPTY
            terrain->setBlockAt(collHit.x, collHit.y, collHit.z, EMPTY);
            // update the chunk containing this block
            uPtr<Chunk>& chunk = terrain->getChunkAt(collHit.x, collHit.z);
            chunk->destroyVBOdata();
            chunk->createVBOdata();
        }
    }
}



void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
