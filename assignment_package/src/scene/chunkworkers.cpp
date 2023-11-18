#include "chunkworkers.h"
#include <iostream>

BlockGenerateWorker::BlockGenerateWorker(int x, int z, std::vector<Chunk*> chunksToFill,
                     std::unordered_set<Chunk*>* chunksCompleted, QMutex* ChunksCompletedLock, Terrain *m) :
    m_xCorner(x), m_zCorner(z), m_chunksToFill(chunksToFill), m_terrain(m),
    mp_chunksCompleted(chunksCompleted), mp_chunksCompletedLock(ChunksCompletedLock)
{}

void BlockGenerateWorker::run() {
    try{
        for (auto &chunk : m_chunksToFill) {
            mp_chunksCompletedLock->lock();
            mp_chunksCompletedLock->unlock();
            chunk->createChunkBlockData();
            mp_chunksCompletedLock->lock();
            mp_chunksCompleted->insert(chunk);
            mp_chunksCompletedLock->unlock();
        }
    }
    catch(const std::exception& e){
        std::cout << "Exception in block generation:" << e.what() << std::endl;
    }
}

VBOWorker::VBOWorker(Chunk* c, std::vector<ChunkOpaqueTransparentVBOData>* dat, QMutex * datLock, Terrain* m) :
    mp_chunk(c), mp_chunkVBOsCompleted(dat), mp_chunkVBOsCompletedLock(datLock), m_terrain(m)
{}

void VBOWorker::run() {
    try{
        mp_chunk->createVBOdata();
        mp_chunkVBOsCompletedLock->lock();
        mp_chunkVBOsCompleted->push_back(mp_chunk->vboData);
        mp_chunkVBOsCompletedLock->unlock();
    }
    catch(const std::exception& e){
        std::cout << "Exception in VBOWorker:" << e.what() << std::endl;
    }
}

