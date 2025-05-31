#pragma once

#include "chunk.h"
#include "region_file.h"

#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>

class ChunkManager
{
   public:
    // Custom hash for std::pair<int, int>
    struct pair_hash
    {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept
        {
            return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
        }
    };

    std::vector<Chunk*> getLoadedChunks() const;
    Chunk&              getChunk(int chunkX, int chunkZ);
    void                unloadChunk(int chunkX, int chunkZ);
    void                unloadAllChunks();

   private:
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, pair_hash>      loadedChunks;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<RegionFile>, pair_hash> regionFiles;

    RegionFile* getRegionFile(int chunkX, int chunkZ);
    void        deserializeChunk(Chunk& chunk, const std::vector<uint8_t>& data);
};
