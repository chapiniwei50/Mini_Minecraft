#pragma once
#ifndef CHUNKWORKERS_H
#define CHUNKWORKERS_H
#include "chunk.h"
#include <unordered_set>
#include <QRunnable>
#include <QMutex>
#include "terrain.h"

// BlockTypeWorkers
class BlockGenerateWorker : public QRunnable {
private:
    // Coords of the terrain zone being generated
    int m_xCorner, m_zCorner;
    std::vector<Chunk*> m_chunksToFill;
    std::unordered_set<Chunk*>* mp_chunksCompleted;
    QMutex* mp_chunksCompletedLock;
    Terrain* m_terrain;
public:
    BlockGenerateWorker(int x, int z, std::vector<Chunk*> chunksToFill,
                        std::unordered_set<Chunk*>* chunksCompleted, QMutex* ChunksCompletedLock, Terrain* m);
    void run() override;

};

class VBOWorker : public QRunnable {
private:
    Chunk* mp_chunk;
    std::vector<ChunkOpaqueTransparentVBOData*>* mp_chunkVBOsCompleted;
    QMutex *mp_chunkVBOsCompletedLock;
    Terrain* m_terrain;
public:
    VBOWorker(Chunk* c, std::vector<ChunkOpaqueTransparentVBOData*>* dat, QMutex * datLock, Terrain* m) ;
    void run() override;
};

#endif // CHUNKWORKERS_H
