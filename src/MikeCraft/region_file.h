#pragma once

#include "chunk.h"
#include "world_generator.h"

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <fstream>
#include <mutex>

class RegionFile
{
   public:
    struct FreeRegion
    {
        uint32_t offset;  // Offset in 4 KiB sectors
        uint32_t size;    // Size in 4 KiB sectors
    };

    explicit RegionFile(const std::string& filePath);

    std::vector<float> continentGrid;
    std::vector<float> erosionGrid;
    std::vector<float> pvGrid;

    void generateNoiseGrids(const WorldGenerator& generator, int regionX, int regionZ,
                            float frequency, int seed);

    std::vector<uint8_t>      loadChunk(int chunkX, int chunkZ);
    void                      saveChunk(const Chunk& chunk);
    void                      rebuildFreeList();
    std::optional<FreeRegion> findFreeRegion(uint32_t sectorCount);
    void                      addFreeRegion(uint32_t offset, uint32_t size);

   private:
    std::string             filePath;
    std::fstream            file;
    std::vector<FreeRegion> freeList;
    std::mutex              fileMutex;

    void initializeRegionFile();
};
