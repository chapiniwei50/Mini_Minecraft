#include "player.h"
#include <QString>
#include <iostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0), m_cameraOrientation(0,0), m_lastFramePosition(0,0,0),
    m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
    mcr_camera(m_camera), mcr_lastFramePosition(m_lastFramePosition)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain, input);
}

void Player::processInputs(InputBundle &inputs) {

    float acc = 300.f;
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

    if (glm::abs(inputs.mouseX) < 5 && glm::abs(inputs.mouseY) < 5) {
        return;
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

    m_lastFramePosition = m_position;
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
    if(!(terrain.hasChunkAt(m_position.x, m_position.z))){
        return;
    }

    std::array<glm::vec3, 12> rayOrigins = {
        m_position + glm::vec3(0.5, 0, 0.5),
        m_position + glm::vec3(0.5, 0, -0.5),
        m_position + glm::vec3(-0.5, 0, -0.5),
        m_position + glm::vec3(-0.5, 0, 0.5),
        m_position + glm::vec3(0.5, 1, 0.5),
        m_position + glm::vec3(0.5, 1, -0.5),
        m_position + glm::vec3(-0.5, 1, -0.5),
        m_position + glm::vec3(-0.5, 1, 0.5),
        m_position + glm::vec3(0.5, 2, 0.5),
        m_position + glm::vec3(0.5, 2, -0.5),
        m_position + glm::vec3(-0.5, 2, -0.5),
        m_position + glm::vec3(-0.5, 2, 0.5)
    };

    try{
        for (glm::vec3 rayOrigin : rayOrigins) {
            for (int axis = 0; axis < 3; axis++) {
                glm::vec3 rayDirection = glm::vec3(0);
                rayDirection[axis] = (*rayDir)[axis];
                float outdist;
                glm::ivec3 out_blockHit;
                bool isBlocked = gridMarch(rayOrigin, rayDirection, mcr_terrain, &outdist, &out_blockHit);
                if (isBlocked) {
                    if (outdist > 0.001f) {
                        (*rayDir)[axis] = glm::sign((*rayDir)[axis]) * (std::fmax(glm::min(glm::abs((*rayDir)[axis]), outdist) - 0.0001f, 0));
                    } else {
                        (*rayDir)[axis] = 0;
                    }
                    m_velocity[axis] = 0;
                    m_acceleration[axis] = 0;
                }
            }
        }
    }
    catch(std::exception &e) {
        std::cout << "Exception encountered:" << e.what() << std::endl;
    }
}


bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit, glm::ivec3 *out_prevCell) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.
    if(out_prevCell){
        *out_prevCell = currCell;
    }

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        if(out_prevCell){
            *out_prevCell = currCell;
        }
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

bool canPlaceBlock(const glm::ivec3& newBlockPos, const glm::vec3& playerPosition) {
    glm::vec3 playerSize(1.0f, 2.0f, 1.0f);
    glm::vec3 playerMin = playerPosition - playerSize / 2.0f;
    glm::vec3 playerMax = playerPosition + playerSize / 2.0f;

    glm::vec3 blockMin = glm::vec3(newBlockPos);
    glm::vec3 blockMax = glm::vec3(newBlockPos) + glm::vec3(1.0f, 1.0f, 1.0f);

    bool noOverlapX = blockMax.x <= playerMin.x || blockMin.x >= playerMax.x;
    bool noOverlapY = blockMax.y <= playerMin.y || blockMin.y >= playerMax.y;
    bool noOverlapZ = blockMax.z <= playerMin.z || blockMin.z >= playerMax.z;

    return noOverlapX || noOverlapY || noOverlapZ;
}


void Player::addBlock(Terrain *terrain){
    glm::vec3 corner = m_camera.mcr_position;
    glm::vec3 rayDir =  3.f *glm::normalize(this->m_forward);
    float maxDistance = 3.f;
    float collDist;
    glm::ivec3 collHit;
    glm::ivec3 newBlockPos;

    if (gridMarch(corner, rayDir, *terrain, &collDist, &collHit, &newBlockPos)) {
        if(collDist < maxDistance){
            if(terrain->getBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z) == EMPTY
                && canPlaceBlock(newBlockPos,m_position)) {
                BlockType blockType = GRASS;
                terrain->setBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z, blockType);

                // update chunk
                uPtr<Chunk>& chunk = terrain->getChunkAt(newBlockPos.x, newBlockPos.z);
                chunk->destroyVBOdata();
                chunk->createVBOdata();
                chunk->buff_data();
            }
        }
    }

}

void Player::removeBlock(Terrain *terrain) {

    // Implement ray casting and grid marching to find and remove the block

    glm::vec3 corner = m_camera.mcr_position;
    glm::vec3 rayDir =  3.f *glm::normalize(this->m_forward);
    float maxDistance = 3.f;

    float collDist;
    glm::ivec3 collHit;

    if (gridMarch(corner, rayDir, *terrain, &collDist, &collHit)) {
        if(collDist < maxDistance){

            BlockType removedBlockType = terrain->getBlockAt(collHit.x, collHit.y, collHit.z);
            // remove the block by setting its type to EMPTY
            terrain->setBlockAt(collHit.x, collHit.y, collHit.z, EMPTY);
            // update the chunk containing this block
            uPtr<Chunk>& chunk = terrain->getChunkAt(collHit.x, collHit.z);
            chunk->destroyVBOdata();
            chunk->createVBOdata();
            chunk->buff_data();
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
