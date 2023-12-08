#include "terrain.h"
#include <stdexcept>
#include <iostream>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context), mp_texture(nullptr)
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

bool Terrain::hasChunkAt(int x, int z) const{
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));

    //QMutexLocker locker(&m_chunksMutex);
    bool ret = m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
    return ret;
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    //QMutexLocker locker(&m_chunksMutex);
    uPtr<Chunk>& ret = m_chunks[toKey(16 * xFloor, 16 * zFloor)];
    return ret;
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    //QMutexLocker locker(&m_chunksMutex);
    const uPtr<Chunk>& ret = m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
    return ret;
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
    chunk->m_countOpq = 0;
    chunk->m_countTra = 0;

    //QMutexLocker locker(&m_chunksMutex);
    m_chunks[toKey(x, z)] = std::move(chunk);
    //locker.unlock();
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        //locker.relock();
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        //locker.unlock();
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        //locker.relock();
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        //locker.unlock();
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        //locker.relock();
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        //locker.unlock();
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        //locker.relock();
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        //locker.unlock();
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool opaque)
{
    // bind the texture
    mp_texture->bind(0);

    // need optimize!
    // only draw chunk that has vbo data and within visible range!
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)){
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if (opaque && chunk->m_countOpq <= 0)
                    continue;
                if (!opaque && chunk->m_countTra <= 0)
                    continue;

                shaderProgram->drawInterleaved(chunk.get(), opaque, 0);
            }
        }
    }
}

std::unordered_set<int64_t> Terrain::borderingZone(glm::ivec2 zone, int radius) const {
    int radiusInZoneScale = static_cast<int>(radius) * 64;
    std::unordered_set<int64_t> result;
    for (int i = -radiusInZoneScale; i <= radiusInZoneScale; i += 64) {
        for (int j = -radiusInZoneScale; j <= radiusInZoneScale; j += 64) {
            result.insert(toKey(zone.x + i, zone.y + j));
        }
    }
    return result;
}

void Terrain::initialTerrainGeneration(glm::vec3 currentPlayerPos){
    glm::ivec2 currentZone(64.f * glm::floor(currentPlayerPos.x / 64.f), 64.f * glm::floor(currentPlayerPos.z / 64.f));
    std::unordered_set<int64_t> currentNearZones = borderingZone(currentZone, zoneRadius);

    for (auto id : currentNearZones) {
        //This zone id will alaways be ungenerated, but this is a check for safty's sake
        //If get called multiple times, will not be generating blocks over and over.
        if (m_generatedTerrain.count(id) == 0) {
            spawnBlockTypeWorker(id);
        }
        //There is no previously generated block, obviously
    }
    QThreadPool::globalInstance()->waitForDone();

    //No need to destroy VBO data.

    //Generate VBO for newly generated chunks
    m_chunksThatHaveBlockDataLock.lock();
    spawnVBOWorkers(m_chunksThatHaveBlockData.size());
    //m_chunksThatHaveBlockData.clear();
    m_chunksThatHaveBlockDataLock.unlock();
    QThreadPool::globalInstance()->waitForDone();

    // Binding VBO data
    m_chunksThatHaveVBOsLock.lock();
    for (ChunkOpaqueTransparentVBOData* cd : m_chunksThatHaveVBOs) {
        cd->mp_chunk->bindVBOdata();
    }
    if (m_chunkCreated < 25 * 4 * 4) {
        m_chunkCreated += m_chunksThatHaveVBOs.size();
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();

}

void Terrain::multithreadedTerrainUpdate(glm::vec3 currentPlayerPos, glm::vec3 previousPlayerPos)
{

    glm::ivec2 currentZone(64.f * glm::floor(currentPlayerPos.x / 64.f), 64.f * glm::floor(currentPlayerPos.z / 64.f));
    glm::ivec2 previousZone(64.f * glm::floor(previousPlayerPos.x / 64.f), 64.f * glm::floor(previousPlayerPos.z / 64.f));

    if (currentZone != previousZone){  // start generate new terrains

        std::unordered_set<int64_t> currentNearZones = borderingZone(currentZone, zoneRadius);
        std::unordered_set<int64_t> previousNearZones = borderingZone(previousZone, zoneRadius);

        for (auto id : currentNearZones) {
            //This zone id is ungenerated
            if (m_generatedTerrain.count(id) == 0) {
                //spawnBlockTypeWorker(id);
                block_to_generate_id.push_back(id);
            }
        }

        for (auto id : previousNearZones) {
            if (currentNearZones.count(id) == 0) {
                glm::ivec2 coord = toCoords(id);
                for (int x = coord.x; x < coord.x + 64; x += 16) {
                    for (int z = coord.y; z < coord.y + 64; z += 16) {
                        auto& chunk = getChunkAt(x, z);
                        if(chunk) chunk->destroyVBOdata();
                    }
                }
            }
        }
    }

    int block_to_generate_size, block_that_have_type_size, block_that_have_vbo_size;

    // Generate n = 1 Block Data each tick
    block_to_generate_size = block_to_generate_id.size();
    spawnBlockTypeWorkers(2);
    //QThreadPool::globalInstance()->waitForDone();

    //Generate VBO for newly generated terrain
    m_chunksThatHaveBlockDataLock.lock();
    block_that_have_type_size = m_chunksThatHaveBlockData.size();
    spawnVBOWorkers(8);
    m_chunksThatHaveBlockDataLock.unlock();
    //QThreadPool::globalInstance()->waitForDone();

    // Binding VBO data
    m_chunksThatHaveVBOsLock.lock();
    block_that_have_vbo_size = m_chunksThatHaveVBOs.size();
    bind_terrain_vbo_data(8);
//    for (ChunkOpaqueTransparentVBOData* cd : m_chunksThatHaveVBOs) {
//        cd->mp_chunk->bindVBOdata();
//    }
//    if (m_chunkCreated < 25 * 4 * 4) {
//       m_chunkCreated += m_chunksThatHaveVBOs.size();
//    }
//    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();

    if ((block_to_generate_size + block_that_have_type_size + block_that_have_vbo_size) != 0)
        fprintf(stderr, "%d\t%d\t%d\n", block_to_generate_size, block_that_have_type_size, block_that_have_vbo_size);

}

void Terrain::spawnVBOWorkers(int n) {
    // each call, we only spwan n workers to process n chunks
    while (n-- && m_chunksThatHaveBlockData.size() > 0){
       // pop the first element
       Chunk* c = *m_chunksThatHaveBlockData.begin();
       m_chunksThatHaveBlockData.erase(m_chunksThatHaveBlockData.begin());
       if (c->m_blocks[0] != STONE){
            printf("here");
            continue;
       }
       spawnVBOWorker(c);
    }
}

void Terrain::spawnVBOWorker(Chunk* chunkNeedingVBOData) {
    VBOWorker* worker = new VBOWorker(
        chunkNeedingVBOData, &m_chunksThatHaveVBOs, &m_chunksThatHaveVBOsLock, this
    );
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnBlockTypeWorkers(int n){
    // call n block type worker each time
    while (n-- && block_to_generate_id.size() > 0){
       int64_t id = *block_to_generate_id.begin();
       block_to_generate_id.erase(block_to_generate_id.begin());
       spawnBlockTypeWorker(id);
    }
}

void Terrain::bind_terrain_vbo_data(int n){
    while (n-- && m_chunksThatHaveVBOs.size() > 0){
       ChunkOpaqueTransparentVBOData* cd = *m_chunksThatHaveVBOs.begin();
       if (cd->m_vboDataOpaque.size() + cd->m_vboDataTransparent.size() == 0)
            printf("here");

       cd->mp_chunk->bindVBOdata();

       if (m_chunkCreated < 25 * 4 * 4) {
            m_chunkCreated += 1;
       }
       m_chunksThatHaveVBOs.erase(m_chunksThatHaveVBOs.begin());
    }
}

void Terrain::spawnBlockTypeWorker(int64_t zone) {
    glm::ivec2 coord = toCoords(zone);
    std::vector<Chunk*> chunksToFill;
    for(int x = coord.x; x < coord.x + 64; x += 16) {
        for(int z = coord.y; z < coord.y + 64; z += 16) {
            Chunk* c = instantiateChunkAt(x, z);
            chunksToFill.push_back(c);
        }
    }

    BlockGenerateWorker* worker = new BlockGenerateWorker(
        coord.x, coord.y, chunksToFill,
        &m_chunksThatHaveBlockData, &m_chunksThatHaveBlockDataLock, this
    );
    QThreadPool::globalInstance()->start(worker);
    /*
    if (QThreadPool::globalInstance()->waitForDone() == false)
    {
        throw std::out_of_range("Waiting threads finish failed!");
    }
    */
    m_generatedTerrain.insert(zone);
}

void Terrain::createChunkBlockData(Chunk* c){
    for(int x = c->get_minX(); x < c->get_minX() + 16; ++x) {
        for(int z = c->get_minZ(); z < c->get_minZ() + 16; ++z) {
            BiomeType biome;
            int height;
            getHeight(x,z,height,biome);
            fillTerrainBlocks(x, z, biome, height);
        }
    }
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
    for (z_bias = -16; z_bias <= 16; z_bias += 32){
        if (! hasChunkAt(xFloor + x_bias, zFloor + z_bias))
        {
            new_chunk = instantiateChunkAt(xFloor + x_bias, zFloor + z_bias);
            createChunkBlockData(new_chunk);
            new_chunk->createVBOdata();
        }
    }

    z_bias = 0;
    for (x_bias = -16; x_bias <= 16; x_bias += 32){
        if (! hasChunkAt(xFloor + x_bias, zFloor + z_bias))
        {
            new_chunk = instantiateChunkAt(xFloor + x_bias, zFloor + z_bias);
            createChunkBlockData(new_chunk);
            new_chunk->createVBOdata();
        }
    }

    if (new_chunk != nullptr)
    {
        // TODO: if there is a chunk added
        // update the vbo of current chunk
        // so that there will be no wall between chunks

    }
}

void Terrain::fillTerrainBlocks(int x, int z, BiomeType biome, int height) {
    // Fill base with STONE
    for (int y = 0; y <= 128; ++y) {
        setBlockAt(x, y, z, STONE);
    }

    // Based on biome, fill above y = 128
    for (int y = 129; y <= height; ++y) {
        if (y == height) {
            // Top block determination
            if (biome == BiomeType::PLAIN) {
                setBlockAt(x, y, z, STONE);
            } else if (biome == BiomeType::MOUNTAIN) {
                setBlockAt(x, y, z, STONE);
            } else if (biome == BiomeType::DESSERT){
                setBlockAt(x, y, z, DIRT);
            }
        } else {
            // Filling other blocks
            if (biome == BiomeType::PLAIN) {
                setBlockAt(x, y, z, DIRT);
            } else if (biome == BiomeType::MOUNTAIN) {
                setBlockAt(x, y, z, STONE);
            } else if (biome == BiomeType::DESSERT){
                setBlockAt(x, y, z, DIRT);
            }
        }
    }

    // Fill WATER if there's empty space between 128 and 138
    for (int y = 129; y < 138; ++y) {
        if (getBlockAt(x, y, z) == EMPTY) {
            setBlockAt(x, y, z, WATER);
        }
        else if(getBlockAt(x, y, z) == GRASS) {
            setBlockAt(x, y, z, DIRT);
        }
    }

    // Generate Cave (Optional, depending on your implementation)

    for (int y = 1; y < 32; ++y) {
        float noiseValue = PerlinNoise3D(glm::vec3(x, y, z) * 0.05f);
        if (noiseValue < 0 && getBlockAt(x, y, z) == STONE)  {
            setBlockAt(x, y, z, EMPTY);
        }
        if (y < 25) {
            // Change for future LAVA
            setBlockAt(x, y, z, EMPTY);
        }
    }

}


void Terrain::getHeight(int x, int z, int& y, BiomeType& b) {
    // Noise settings for biome determination and height variation.
    const float biomeScale = 0.05f; // Larger scale for biome determination.
    const float terrainScale = 0.01f; // Terrain variation scale.
    const int baseHeight = 135;      // Base height for the terrain.

    float biomeNoiseValue = PerlinNoise2D(x * biomeScale, z * biomeScale, 1.0f, 2) * 0.005 + 0.5;

    float height = baseHeight;

    // Determine the biome based on the biomeNoiseValue
    if (biomeNoiseValue <= 0.35) { // Plains
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 30 + 10;
        b = BiomeType::PLAIN;
    } else if (biomeNoiseValue >= 0.4 && biomeNoiseValue <= 0.75) { // Desert
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 40 + 5;
        b = BiomeType::DESSERT;
    } else if (biomeNoiseValue > 0.75) { // Mountains
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 30 + 15;
        b = BiomeType::MOUNTAIN;
    } else { // Transition between Plains and Desert
        float plainsHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 30 + 10;
        float desertHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 40 + 5;
        float smoothStepInput = (biomeNoiseValue - 0.4f) / 0.3f;
        float smoothStepResult = glm::smoothstep(0.25f, 0.75f, smoothStepInput);
        height += plainsHeight * (1.0f - smoothStepResult) + desertHeight * smoothStepResult;
        b = smoothStepResult < 0.5f ? BiomeType::PLAIN : BiomeType::DESSERT;
    }
    y = static_cast<int>(round(height));
    y = std::min(255, std::max(0, y));
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

float Terrain::PerlinNoise2D(float x, float z, float frequency, int octaves) {
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

glm::vec2 fract(glm::vec2 v) {
    return glm::vec2(v.x - std::floor(v.x), v.y - std::floor(v.y));
}

glm::vec2 floor(glm::vec2 v) {
    return glm::vec2(std::floor(v.x), std::floor(v.y));
}

float length(glm::vec2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

float min(float a, float b) {
    return (a < b) ? a : b;
}

float Terrain::WorleyNoise(float x, float y) {
    glm::vec2 uv(x * 10.0f, y * 10.0f);
    glm::vec2 uvInt = floor(uv);
    glm::vec2 uvFract = fract(uv);
    float minDist = 1.0f;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor(x, y);
            glm::vec2 point = random2(uvInt + neighbor);
            glm::vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }

    return minDist;
}

glm::vec3 random3(glm::vec3 p) {
    // This should return a random glm::vec3 where each component is in the range [-1, 1]
    // Adjust the numbers for the dot product to suit your seed needs
    return glm::fract(glm::sin(glm::vec3(glm::dot(p, glm::vec3(127.1, 311.7, 74.7)),
                                         glm::dot(p, glm::vec3(269.5, 183.3, 246.1)),
                                         glm::dot(p, glm::vec3(113.5, 271.9, 124.6))))
                      * 43758.5453f) * 2.0f - 1.0f;
}

float surflet(glm::vec3 p, glm::vec3 gridPoint) {
    glm::vec3 t = glm::abs(p - gridPoint);
    t = 1.f - 6.f * glm::pow(t, glm::vec3(5.0)) + 15.f * glm::pow(t, glm::vec3(4.0f)) - 10.f * glm::pow(t, glm::vec3(3.0f));
    glm::vec3 gradient = random3(gridPoint);
    glm::vec3 diff = p - gridPoint;
    float height = glm::dot(diff, gradient);
    return height * t.x * t.y * t.z;
}

float Terrain::PerlinNoise3D(glm::vec3 p) {
    float surfletSum = 0.f;
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet(p, glm::floor(p) + glm::vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}

void Terrain::CreateTestScene()
{
    //ABANDONED FUNCTION

    //Current boundary for testing (will be changed after milestone2)
    int m_minX = 0;
    int m_maxX = 0;
    int m_minZ = 0;
    int m_maxZ = 0;

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

    // for each chunk, create the vbo data
    for (int x = m_minX; x < m_maxX; x += 16)
        for (int z = m_minZ; z < m_maxZ; z += 16)
            m_chunks[toKey(x, z)]->createVBOdata();

}


void Terrain::create_load_texture(const char* textureFile)
{
    mp_texture = mkU<Texture>(mp_context);
    mp_texture->create(textureFile);
    mp_texture->load(0);
}


