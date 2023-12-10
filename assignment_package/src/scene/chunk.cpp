#include "chunk.h"
#include <iostream>

Chunk::Chunk(int x, int z, OpenGLContext* context)
    : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}, vboData(this)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

int Chunk::is_boundary(int x, int y, int z) const
{
    // check whether the block at (x, z, y) is a boundary block that need to be rendered
    // return a six-bit binary number, each bit indicates whether a certain face is boundary
    // [xneg, xpos, yneg, ypos, zneg, zpos], 1 means is_boundary

    int res = 0b000000;

    // empty cannot be boundaries
    if (getBlockAt(x, y, z) == EMPTY)
        return res;

    // if water
    if (getBlockAt(x, y, z) == WATER)
    {  // only render the YPOS of the first layer of water
        if (y == 255 || (y != 255 && getBlockAt(x, y + 1, z) == EMPTY))
            return 0b000100;
        else
            return 0;
    }

    // x neg face direction
    if ((x == 0 && m_neighbors.at(XNEG) == nullptr) ||
        (x == 0 && m_neighbors.at(XNEG)->getBlockAt(15, y, z) == EMPTY) ||
        (x != 0 && getBlockAt(x - 1, y, z) == EMPTY) ||
        (x == 0 && m_neighbors.at(XNEG)->getBlockAt(15, y, z) == WATER) ||
        (x != 0 && getBlockAt(x - 1, y, z) == WATER))
        res = res | 0b100000;

    // x pos face direction
    if ((x == 15 && m_neighbors.at(XPOS) == nullptr) ||
        (x == 15 && m_neighbors.at(XPOS)->getBlockAt(0, y, z) == EMPTY) ||
        (x != 15 && getBlockAt(x + 1, y, z) == EMPTY) ||
        (x == 15 && m_neighbors.at(XPOS)->getBlockAt(0, y, z) == WATER) ||
        (x != 15 && getBlockAt(x + 1, y, z) == WATER))
        res = res | 0b010000;

    // y neg face direction
    if (y == 0 ||
        (y != 0 && getBlockAt(x, y - 1, z) == EMPTY) ||
        (y != 0 && getBlockAt(x, y - 1, z) == WATER))
        res = res | 0b001000;

    // y pos face direction
    if (y == 255 ||
        (y != 255 && getBlockAt(x, y + 1, z) == EMPTY) ||
        (y != 255 && getBlockAt(x, y + 1, z) == WATER))
        res = res | 0b000100;

    // z neg face direction
    if ((z == 0 && m_neighbors.at(ZNEG) == nullptr) ||
        (z == 0 && m_neighbors.at(ZNEG)->getBlockAt(x, y, 15) == EMPTY) ||
        (z != 0 && getBlockAt(x, y, z - 1) == EMPTY) ||
        (z == 0 && m_neighbors.at(ZNEG)->getBlockAt(x, y, 15) == WATER) ||
        (z != 0 && getBlockAt(x, y, z - 1) == WATER))
        res = res | 0b000010;

    // z pos face direction
    if ((z == 15 && m_neighbors.at(ZPOS) == nullptr) ||
        (z == 15 && m_neighbors.at(ZPOS)->getBlockAt(x, y, 0) == EMPTY) ||
        (z != 15 && getBlockAt(x, y, z + 1) == EMPTY) ||
        (z == 15 && m_neighbors.at(ZPOS)->getBlockAt(x, y, 0) == WATER) ||
        (z != 15 && getBlockAt(x, y, z + 1) == WATER))
        res = res | 0b000001;

    return res;
}


void Chunk::createVBOdata()
{

    for (unsigned int x = 0; x < 16; x++)
        for (unsigned int y = 0; y < 256; y++)
            for (unsigned int z = 0; z < 16; z++)
            {
                int boundary_info = is_boundary(x, y, z);

                // empty block or no boundaries. No need to draw
                if (boundary_info == 0)
                    continue;

                BlockType t = getBlockAt(x, y, z);

                // choose the correct data buffer according to whther the block is opaque
                // should change to a set for futhre work
                std::vector<glm::vec4>& data_to_push = (t == WATER) ? vboData.m_vboDataTransparent : vboData.m_vboDataOpaque;

                glm::vec4 uv;
                // draw x neg face
                if ((boundary_info & 0b100000) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(XNEG), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }

                // draw x pos face
                if ((boundary_info & 0b010000) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(XPOS), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }

                // draw y neg face
                if ((boundary_info & 0b001000) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(YNEG), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }

                // draw y pos face
                if ((boundary_info & 0b000100) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(YPOS), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }

                // draw z neg face
                if ((boundary_info & 0b000010) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(ZNEG), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }

                // draw z pos face
                if ((boundary_info & 0b000001) != 0)
                {
                    uv = glm::vec4(blockFaceUVs.at(t).at(ZPOS), 0, 0);
                    if (t == WATER)  // WATER
                        uv[2] = 1.0;
                    if (t == LAVA)  // LAVA
                        uv[2] = 0.5;

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, GRID, 0, 0));

                    data_to_push.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(0, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, 0, 0, 0));

                    data_to_push.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    data_to_push.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    data_to_push.push_back(uv + glm::vec4(GRID, GRID, 0, 0));
                }
            }

    // generate index data according to the number of face to render
    int num_faces_opaque = vboData.m_vboDataOpaque.size() / 4 / 3;
    for (int i = 0; i < num_faces_opaque; i++)
    {
        vboData.m_idxDataOpaque.push_back(i * 4);
        vboData.m_idxDataOpaque.push_back(i * 4 + 1);
        vboData.m_idxDataOpaque.push_back(i * 4 + 2);
        vboData.m_idxDataOpaque.push_back(i * 4);
        vboData.m_idxDataOpaque.push_back(i * 4 + 2);
        vboData.m_idxDataOpaque.push_back(i * 4 + 3);
    }
    int num_faces_transparent = vboData.m_vboDataTransparent.size() / 4 / 3;
    for (int i = 0; i < num_faces_transparent; i++)
    {
        vboData.m_idxDataTransparent.push_back(i * 4);
        vboData.m_idxDataTransparent.push_back(i * 4 + 1);
        vboData.m_idxDataTransparent.push_back(i * 4 + 2);
        vboData.m_idxDataTransparent.push_back(i * 4);
        vboData.m_idxDataTransparent.push_back(i * 4 + 2);
        vboData.m_idxDataTransparent.push_back(i * 4 + 3);
    }

    m_countOpq = vboData.m_idxDataOpaque.size();
    m_countTra = vboData.m_idxDataTransparent.size();
}

void Chunk::bindVBOdata()
{
    // buff vertex data and indices into proper VBOs.
    if (m_countOpq > 0)
    {
        generateIdxOpq();
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, vboData.m_idxDataOpaque.size() * sizeof(GLuint), vboData.m_idxDataOpaque.data(), GL_STATIC_DRAW);

        generateDataOpq();
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufDataOpq);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.m_vboDataOpaque.size() * sizeof(glm::vec4), vboData.m_vboDataOpaque.data(), GL_STATIC_DRAW);
    }

    if (m_countTra > 0)
    {
        generateIdxTra();
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, vboData.m_idxDataTransparent.size() * sizeof(GLuint), vboData.m_idxDataTransparent.data(), GL_STATIC_DRAW);

        generateDataTra();
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufDataTra);
        mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.m_vboDataTransparent.size() * sizeof(glm::vec4), vboData.m_vboDataTransparent.data(), GL_STATIC_DRAW);
    }
}

void Chunk::createChunkBlockData(){
    std::vector<std::vector<int>> heights(16, std::vector<int>(16));
    std::vector<std::vector<BiomeType>> biomes(16, std::vector<BiomeType>(16));

    for(int x = minX; x < minX + 16; ++x) {
        for(int z = minZ; z < minZ + 16; ++z) {
            BiomeType biome;
            int height;
            getHeight(x,z,height,biome);
            heights[x-minX][z-minZ] = height;
            biomes[x-minX][z-minZ] = biome;
            fillTerrainBlocks(x, z, biome, height);
        }
    }

    placeTree(heights, biomes);

}

void Chunk::placeTree(std::vector<std::vector<int>>& heights, std::vector<std::vector<BiomeType>>& biomes){
    std::srand(std::time(nullptr) + minX + minZ);
    int numTrees = std::rand() % 3;
    std::vector<glm::vec2> treesPos;
    auto isValid = [&treesPos](const glm::vec2& newPoint) {
        for (const auto& point : treesPos) {
            if (std::abs(point.x - newPoint.x) <= 4 && std::abs(point.y - newPoint.y) <= 4) {
                return false;
            }
        }
        return true;
    };

    int maxTry = 10;
    int tryTimes = 0;
    while (treesPos.size() < numTrees && tryTimes < maxTry) {
        tryTimes++;
        glm::vec2 newPoint = {std::rand() % 11 + 3, std::rand() % 11 + 3};
        if (isValid(newPoint))
            treesPos.push_back(newPoint);
    }

    for (const auto& treePos : treesPos) {
        int x = static_cast<int>(treePos.x);
        int z = static_cast<int>(treePos.y);
        int floorHeight = heights[x][z];
        if(biomes[x][z] != BiomeType::PLAIN)
            continue;

        for(int dy = 1; dy <= 5 ; dy++)
            setBlockAt(x, floorHeight + dy, z, TRUNK);

        for(int dy = 3; dy <= 4 ; dy++){
            for(int dx = -2; dx <= 2 ; dx ++ ){
                for(int dz = -2; dz <= 2 ; dz ++ ){
                    if(dx == 0 && dz == 0)
                        continue;
                    setBlockAt(x + dx, floorHeight + dy, z + dz, LEAF);
                }
            }
        }

        int dy = 5;
        for(int dx = -2; dx <= 2 ; dx ++ ){
            for(int dz = -2; dz <= 2 ; dz ++ ){
                if((dx == 0 && dz == 0)   ||
                    (dx == -2 && dz == -2) ||
                    (dx == -2 && dz == 2)  ||
                    (dx == 2 && dz == 2)   ||
                    (dx == 2 && dz == -2)  )
                    continue;
                setBlockAt(x + dx, floorHeight + dy, z + dz, LEAF);
            }
        }

        dy = 6;
        for(int dx = -1; dx <= 1 ; dx ++ ){
            for(int dz = -1; dz <= 1 ; dz ++ ){
                setBlockAt(x + dx, floorHeight + dy, z + dz, LEAF);
            }
        }
    }
}

void Chunk::fillTerrainBlocks(int x, int z, BiomeType biome, int height) {
    // Convert to local axis.
    x -= minX;
    z -= minZ;
    try {
        for (int y = 0; y <= 128; ++y) {
            setBlockAt(x, y, z, STONE);
        }
    }catch(std::exception &e) {
        std::cout << "Exception in fillTerrainBlocks STONE fill" << std::endl;
    }

    // Based on biome, fill above y = 128
    for (int y = 129; y <= height; ++y) {
        try {
            switch (biome) {
            case BiomeType::PLAIN:
                if (y == height) {
                    setBlockAt(x, y, z, GRASS);
                } else {
                    setBlockAt(x, y, z, DIRT);
                }
                break;

            case BiomeType::MOUNTAIN:
                if (y == height) {
                    setBlockAt(x, y, z, DIRT);
                } else {
                    setBlockAt(x, y, z, STONE);
                }
                break;

            case BiomeType::DESSERT:
                if (y == height) {
                    setBlockAt(x, y, z, STONE);
                } else {
                    setBlockAt(x, y, z, DIRT);
                }
                break;

            case BiomeType::RIVER:
                setBlockAt(x, y, z, DIRT);
                break;

            default:
                // Handle unknown biomes
                setBlockAt(x, y, z, WATER);
                break;
            }

        } catch(std::exception &e) {
            std::cout << "Exception in fillTerrainBlocks y = [129, ?] loop, height = " << height << ", xz = " << x << "," << z << std::endl;
        }
    }

    // Fill WATER if therew's empty space between 128 and 148
    for (int y = 129; y < 146; ++y) {
        try {
            if (getBlockAt(x, y, z) == EMPTY) {
                setBlockAt(x, y, z, WATER);
            }
            else if(getBlockAt(x, y, z) == GRASS) {
                setBlockAt(x, y, z, DIRT);
            }
        }catch(std::exception &e) {
            std::cout << "Exception in fillTerrainBlocks WATER table, y = " << y << ", xz = " << x << "," << z << std::endl;
        }
    }

    /*
    for (int y = 1; y < 64; ++y) {
        float noiseValue = PerlinNoise3D(glm::vec3(x, y, z) * 0.05f);
        if (noiseValue < 0 && getBlockAt(x, y, z) == STONE)  {
            setBlockAt(x, y, z, EMPTY);
        }
        if (y < 25) {
            // Change for future LAVA
            setBlockAt(x, y, z, LAVA);
        }
    }*/

}

void Chunk::refreshChunkVBOData(){
    destroyVBOdata();
    createVBOdata();
    bindVBOdata();
}

void Chunk::refreshAdjacentChunkVBOData(){

    m_neighbors[XNEG]->refreshChunkVBOData();
    m_neighbors[XPOS]->refreshChunkVBOData();
    m_neighbors[ZNEG]->refreshChunkVBOData();
    m_neighbors[ZPOS]->refreshChunkVBOData();
}

void Chunk::getHeight(int x, int z, int& y, BiomeType& b) {
    x += 10000;
    z += 10000;
    // Noise settings for biome determination and height variation.
    const float biomeScale = 0.0025f; // Larger scale for biome determination.
    const float terrainScale = 0.01f; // Terrain variation scale.
    const int baseHeight = 145;      // Base height for the terrain.
    const float plainStart = -1;
    const float plainEnd = 0.4;
    const float desertStart = 0.5;
    const float desertEnd = 0.8;
    const float mountainStart = 0.9;
    const float mountainEnd = 1.2;

    float biomeNoiseValue = PerlinNoise2D(x * biomeScale, z * biomeScale, 1.0f, 2) * 2 + 0.5;

    float height = baseHeight;

    // Determine the biome based on the biomeNoiseValue
    if (biomeNoiseValue >= plainStart && biomeNoiseValue <= plainEnd) { // Plains
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 30 + 10;
        b = BiomeType::PLAIN;
    }
    else if (biomeNoiseValue >= plainEnd && biomeNoiseValue <= desertStart) { // Transition between Plains and Desert
        float plainsHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 30 + 10;
        float desertHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 20 + 5;
        float smoothStepInput = (biomeNoiseValue - plainEnd) / (desertStart - plainEnd);
        float smoothStepResult = glm::smoothstep(0.0f, 1.0f, smoothStepInput);
        height += plainsHeight * (1.0f - smoothStepResult) + desertHeight * smoothStepResult;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> dis(0.5, 0.2);
        float u = dis(gen);

        b = smoothStepResult < u ? BiomeType::PLAIN : BiomeType::DESSERT;
    }
    else if (biomeNoiseValue >= desertStart && biomeNoiseValue <= desertEnd) { // Desert
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 20 + 5;
        b = BiomeType::DESSERT;
    }
    else if (biomeNoiseValue >= desertEnd && biomeNoiseValue <= mountainStart) { // Dessert and Mountains
        float desertHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 20 + 5;
        float mountainHeight = PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 80 + 10;
        float smoothStepInput = (biomeNoiseValue - desertEnd) / (mountainStart - desertEnd);
        float smoothStepResult = glm::smoothstep(0.0f, 1.0f, smoothStepInput);
        float riverBedFactor = 1 - pow(cos(2 * M_PI * smoothStepResult),7.0);
        float adjustedHeight = desertHeight * (1.0f - smoothStepResult) + mountainHeight * smoothStepResult;
        height += adjustedHeight - 10 * riverBedFactor;
        b = BiomeType::RIVER;
    }
    else if (biomeNoiseValue >= mountainStart && biomeNoiseValue <= mountainEnd) { // Mountains
        height += PerlinNoise2D(x * terrainScale, z * terrainScale, 1.0f, 4) * 80 + 10;
        b = BiomeType::MOUNTAIN;
    }
    else{
        height -= 50;
        b = BiomeType::LAVA;
    }
    y = static_cast<int>(round(height));
    y = std::min(255, std::max(0, y));
}

glm::vec2 Chunk::random2(glm::vec2 p) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                                         glm::dot(p, glm::vec2(269.5,183.3))))
                      * 43758.5453f);
}

float Chunk::surflet(glm::vec2 P, glm::vec2 gridPoint) {
    float distX = glm::abs(P.x - gridPoint.x);
    float distY = glm::abs(P.y - gridPoint.y);
    float tX = 1.f - 6.f * glm::pow(distX, 5.f) + 15.f * glm::pow(distX, 4.f) - 10.f * glm::pow(distX, 3.f);
    float tY = 1.f - 6.f * glm::pow(distY, 5.f) + 15.f * glm::pow(distY, 4.f) - 10.f * glm::pow(distY, 3.f);
    glm::vec2 gradient = 2.f * random2(gridPoint) - glm::vec2(1.f);
    glm::vec2 diff = P - gridPoint;
    float height = glm::dot(diff, gradient);
    return height * tX * tY;
}

float Chunk::perlinNoiseSingle(glm::vec2 uv) {
    float surfletSum = 0.f;
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::vec2((int)uv.x + dx, (int)uv.y + dy));
        }
    }
    return surfletSum;
}

float Chunk::PerlinNoise2D(float x, float z, float frequency, int octaves) {
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

glm::vec2 Chunk::fract(glm::vec2 v) {
    return glm::vec2(v.x - std::floor(v.x), v.y - std::floor(v.y));
}

glm::vec2 Chunk::floor(glm::vec2 v) {
    return glm::vec2(std::floor(v.x), std::floor(v.y));
}

float Chunk::length(glm::vec2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

float Chunk::min(float a, float b) {
    return (a < b) ? a : b;
}

float Chunk::WorleyNoise(float x, float y) {
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

glm::vec3 Chunk::random3(glm::vec3 p) {
    // This should return a random glm::vec3 where each component is in the range [-1, 1]
    // Adjust the numbers for the dot product to suit your seed needs
    return glm::fract(glm::sin(glm::vec3(glm::dot(p, glm::vec3(127.1, 311.7, 74.7)),
                                         glm::dot(p, glm::vec3(269.5, 183.3, 246.1)),
                                         glm::dot(p, glm::vec3(113.5, 271.9, 124.6))))
                      * 43758.5453f) * 2.0f - 1.0f;
}

float Chunk::surflet(glm::vec3 p, glm::vec3 gridPoint) {
    glm::vec3 t = glm::abs(p - gridPoint);
    t = 1.f - 6.f * glm::pow(t, glm::vec3(5.0)) + 15.f * glm::pow(t, glm::vec3(4.0f)) - 10.f * glm::pow(t, glm::vec3(3.0f));
    glm::vec3 gradient = random3(gridPoint);
    glm::vec3 diff = p - gridPoint;
    float height = glm::dot(diff, gradient);
    return height * t.x * t.y * t.z;
}

float Chunk::PerlinNoise3D(glm::vec3 p) {
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
