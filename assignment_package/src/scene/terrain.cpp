#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context)
{}

Terrain::~Terrain() {
    for (auto &i : m_chunks)
        i.second->destroyVBOdata();
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(x, z, mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram)
{
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)){
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                shaderProgram->drawInterleaved(chunk.get());
            }
        }
    }
}

void Terrain::CreateTestScene()
{
    //Current boundary for testing (will be changed after milestone2)
    int m_minX = -128;
    int m_maxX = 128;
    int m_minZ = -128;
    int m_maxZ = 128;


    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = m_minX; x < m_maxX; x += 16) {
        for(int z = m_minZ; z < m_maxZ; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));

    //Noise terrain
    const float terrainScale = 0.1f; // Smaller values will make the terrain smoother
    const int heightMultiplier = 5;  // Adjust this for higher or lower terrain
    const int baseHeight = 128;       // Base height for the terrain

    for(int x = m_minX; x < m_maxX; ++x) {
        for(int z = m_minZ; z < m_maxZ; ++z) {
            float height = PerlinNoise(x * terrainScale, z * terrainScale, 1.0f, 4) * heightMultiplier;
            height += baseHeight;
            int intHeight = static_cast<int>(round(height));
            intHeight = intHeight > 255 ? 255 : intHeight;
            for(int y = 0; y <= intHeight; ++y) {
                BlockType blockType = (y == intHeight) ? GRASS : DIRT;
                setBlockAt(x, y, z, blockType);
            }
        }
    }

    // for each chunk, create the vbo data
    for (int x = m_minX; x < m_maxX; x += 16)
        for (int z = m_minZ; z < m_maxZ; z += 16)
            m_chunks[toKey(x, z)]->createVBOdata();

}

glm::vec2 random2(glm::vec2 p) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                                         glm::dot(p, glm::vec2(269.5,183.3))))
                      * 43758.5453f);
}

float surflet(glm::vec2 P, glm::vec2 gridPoint) {
    float distX = glm::abs(P.x - gridPoint.x);
    float distY = glm::abs(P.y - gridPoint.y);
    float tX = 1.f - 6.f * glm::pow(distX, 5.f) + 15.f * glm::pow(distX, 4.f) - 10.f * glm::pow(distX, 3.f);
    float tY = 1.f - 6.f * glm::pow(distY, 5.f) + 15.f * glm::pow(distY, 4.f) - 10.f * glm::pow(distY, 3.f);
    glm::vec2 gradient = 2.f * random2(gridPoint) - glm::vec2(1.f);
    glm::vec2 diff = P - gridPoint;
    float height = glm::dot(diff, gradient);
    return height * tX * tY;
}

float Terrain::perlinNoiseSingle(glm::vec2 uv) {
    float surfletSum = 0.f;
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::vec2((int)uv.x + dx, (int)uv.y + dy));
        }
    }
    return surfletSum;
}

float Terrain::PerlinNoise(float x, float z, float frequency, int octaves) {
    float amplitude = 1.0f;
    float maxAmplitude = 0.0f;
    float noise = 0.0f;
    glm::vec2 uv(x, z);

    for(int i = 0; i < octaves; i++) {
        noise += perlinNoiseSingle(uv * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    noise /= maxAmplitude;

    return noise;
}

void Terrain::check_edge(float x_f, float z_f)
{
    // check whether the player is at the edge of the terrain
    // if true, add a new terrain and add to VBO
    int xFloor = static_cast<int>(glm::floor(x_f / 16.f)) * 16;
    int zFloor = static_cast<int>(glm::floor(z_f / 16.f)) * 16;

    Chunk* new_chunk = nullptr;

    // check for four directions
    int x_bias, z_bias;
    x_bias = 0;
    for (z_bias = -16; z_bias <= 16; z_bias += 32)
        if (! hasChunkAt(xFloor + x_bias, zFloor + z_bias))
        {
            new_chunk = instantiateChunkAt(xFloor + x_bias, zFloor + z_bias);

            //Noise terrain
            const float terrainScale = 0.1f; // Smaller values will make the terrain smoother
            const int heightMultiplier = 5;  // Adjust this for higher or lower terrain
            const int baseHeight = 128;       // Base height for the terrain

            for(int x = new_chunk->get_minX(); x < new_chunk->get_minX() + 16; ++x) {
                for(int z = new_chunk->get_minZ(); z < new_chunk->get_minZ() + 16; ++z) {
                    float height = PerlinNoise(x * terrainScale, z * terrainScale, 1.0f, 4) * heightMultiplier;
                    height += baseHeight;
                    int intHeight = static_cast<int>(round(height));
                    intHeight = intHeight > 255 ? 255 : intHeight;
                    for(int y = 0; y <= intHeight; ++y) {
                        BlockType blockType = (y == intHeight) ? GRASS : DIRT;
                        setBlockAt(x, y, z, blockType);
                    }
                }
            }
            new_chunk->createVBOdata();
        }
    z_bias = 0;
    for (x_bias = -16; x_bias <= 16; x_bias += 32)
        if (! hasChunkAt(xFloor + x_bias, zFloor + z_bias))
        {
            new_chunk = instantiateChunkAt(xFloor + x_bias, zFloor + z_bias);

            //Noise terrain
            const float terrainScale = 0.1f; // Smaller values will make the terrain smoother
            const int heightMultiplier = 5;  // Adjust this for higher or lower terrain
            const int baseHeight = 128;       // Base height for the terrain

            for(int x = new_chunk->get_minX(); x < new_chunk->get_minX() + 16; ++x) {
                for(int z = new_chunk->get_minZ(); z < new_chunk->get_minZ() + 16; ++z) {
                    float height = PerlinNoise(x * terrainScale, z * terrainScale, 1.0f, 4) * heightMultiplier;
                    height += baseHeight;
                    int intHeight = static_cast<int>(round(height));
                    intHeight = intHeight > 255 ? 255 : intHeight;
                    for(int y = 0; y <= intHeight; ++y) {
                        BlockType blockType = (y == intHeight) ? GRASS : DIRT;
                        setBlockAt(x, y, z, blockType);
                    }
                }
            }
            new_chunk->createVBOdata();
        }

    if (new_chunk != nullptr)
    {
        // TODO: if there is a chunk added
        // update the vbo of current chunk
        // so that there will be no wall between chunks
    }
}


