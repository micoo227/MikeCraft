#pragma once

#include "chunk.h"
#include "region_file.h"

#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class ChunkManager
{
   public:
    ChunkManager();
    ~ChunkManager();

    // Custom hash for std::pair<int, int>
    struct pair_hash
    {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept
        {
            return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
        }
    };

    std::vector<Chunk*> getLoadedChunks() const;
    void                unloadChunk(int chunkX, int chunkZ);
    void                unloadAllChunks();
    void                updateChunksAroundPlayer(float playerX, float playerZ, int renderRadius);
    void                processChunkUploads();
    void                stopWorker();

   private:
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, pair_hash>      loadedChunks;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<RegionFile>, pair_hash> regionFiles;
    std::mutex regionFilesMutex;

    RegionFile* getRegionFile(int chunkX, int chunkZ);
    void        deserializeChunk(Chunk& chunk, const std::vector<uint8_t>& data);

    std::queue<std::pair<int, int>> chunkLoadQueue;
    std::mutex                      queueMutex;
    std::condition_variable         queueCV;
    std::thread                     workerThread;
    std::atomic<bool>               stopWorkerThread{false};

    struct PendingChunk
    {
        int                    chunkX, chunkZ;
        std::unique_ptr<Chunk> chunk;
    };
    std::queue<PendingChunk> readyChunks;
    std::mutex               readyMutex;

    void workerFunc();
    void enqueueChunkLoad(int chunkX, int chunkZ);
};
