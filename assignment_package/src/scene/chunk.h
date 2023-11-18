#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <stdio.h>
#include <stdexcept>

class Terrain;
struct ChunkOpaqueTransparentVBOData;
//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

enum class BiomeType : unsigned char{
    NULLBIOME,
    DESSERT,
    PLAIN,
    MOUNTAIN
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

//Leave for opaque and transparent data
class Chunk;
struct ChunkOpaqueTransparentVBOData {
    Chunk* mp_chunk;
    std::vector<glm::vec4> m_vboDataOpaque, m_vboDataTransparent;
    std::vector<GLuint> m_idxDataOpaque, m_idxDataTransparent;

    ChunkOpaqueTransparentVBOData(Chunk* c) :
        mp_chunk(c), m_vboDataOpaque{}, m_vboDataTransparent{},
        m_idxDataOpaque{}, m_idxDataTransparent{}
    {}
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    int minX, minZ;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

public:
    Chunk();
    Chunk(int x, int z, OpenGLContext* context);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    int is_boundary(int x, int y, int z) const;

    void createVBOdata() override;

    ~Chunk() override {};

    void buff_data();

    int get_minX(){return minX;}
    int get_minZ(){return minZ;}

    ChunkOpaqueTransparentVBOData vboData;
    void createChunkBlockData();
    void fillTerrainBlocks(int x, int z, BiomeType biome, int height);
    void getHeight(int x, int z, int& y, BiomeType& b);
    float perlinNoiseSingle(glm::vec2 uv);
    float PerlinNoise2D(float x, float z, float frequency, int octaves);
    float WorleyNoise(float x, float y);
    float PerlinNoise3D(glm::vec3 p);
    glm::vec2 random2(glm::vec2 p);
    float surflet(glm::vec2 P, glm::vec2 gridPoint);
    glm::vec3 random3(glm::vec3 p);
    float surflet(glm::vec3 p, glm::vec3 gridPoint);

    glm::vec2 fract(glm::vec2 v);

    glm::vec2 floor(glm::vec2 v);

    float length(glm::vec2 v);

    float min(float a, float b);

    friend class Terrain;

};
