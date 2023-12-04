#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <QRunnable>
#include <QMutex>
#include <QThreadPool>
#include "shaderprogram.h"
#include "chunkworkers.h"
#include "chunk.h"
#include "texture.h"




namespace std {
template<>
struct hash<glm::ivec2> {
    size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};
}

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.

    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;

    OpenGLContext* mp_context;

    std::unordered_set<Chunk*> m_chunksThatHaveBlockData;
    QMutex m_chunksThatHaveBlockDataLock;
    std::vector<ChunkOpaqueTransparentVBOData*> m_chunksThatHaveVBOs;
    QMutex m_chunksThatHaveVBOsLock;
    std::vector<int64_t> block_to_generate_id;
    int m_chunkCreated;
    //mutable QMutex m_chunksMutex;

    // the texture that applies to all chunks
    uPtr<Texture> mp_texture;



public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the created Chunk.
    Chunk* instantiateChunkAt(int x, int z);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setBlockAt(int x, int y, int z, BlockType t);

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool opaque);

    // Initializes the Chunks that store the 64 x 256 x 64 block scene you
    // see when the base code is run.
    void CreateTestScene();

    // check whether to add a new chunk when player is at x, z
    void check_edge(float x, float z);

    void createChunkBlockData(Chunk* c);

    // Multithreading for terrain update
    void multithreadedTerrainUpdate(glm::vec3 currentPlayerPos, glm::vec3 previousPlayerPos);
    std::unordered_set<int64_t> borderingZone(glm::ivec2 zone, int radius) const;
    void spawnVBOWorker(Chunk* c);
    void spawnVBOWorkers(std::unordered_set<Chunk *> &chunksNeedingVBOs, int n);
    void spawnBlockTypeWorker(int64_t zone);
    void spawnBlockTypeWorkers(int n);
    void bind_terrain_vbo_data(int n);
    void initialTerrainGeneration(glm::vec3 currentPlayerPos);

    float PerlinNoise2D(float x, float z, float frequency, int octaves);
    float PerlinNoise3D(glm::vec3 p);
    float perlinNoiseSingle(glm::vec2 uv);
    float WorleyNoise(float x, float y);
    void getHeight(int x, int z, int& y, BiomeType& b);
    void fillTerrainBlocks(int x, int z, BiomeType biome, int height);

    // init texture file
    void create_load_texture(const char *textureFile);

    // Visible distance
    const int zoneRadius = 4;

};
