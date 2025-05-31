#include "chunk_manager.h"
#include "constants.h"
#include "file_utils.h"
#include "zlib/zlib.h"

#include <stdexcept>
#include <vector>

std::vector<Chunk*> ChunkManager::getLoadedChunks() const
{
    std::vector<Chunk*> result;
    for (const auto& pair : loadedChunks)
        result.push_back(pair.second.get());
    return result;
}

// Get a chunk (load it if not already loaded)
Chunk& ChunkManager::getChunk(int chunkX, int chunkZ)
{
    auto key = std::make_pair(chunkX, chunkZ);

    auto it = loadedChunks.find(key);
    if (it != loadedChunks.end())
        return *(it->second);

    RegionFile* region = getRegionFile(chunkX, chunkZ);
    auto        data   = region->loadChunk(chunkX, chunkZ);
    auto        chunk  = std::make_unique<Chunk>(chunkX, chunkZ);
    if (!data.empty())
    {
        deserializeChunk(*chunk, data);
    }
    else
    {
        // Generate a small pyramid (temporary implementation before worldgen)
        int baseSize = 7;
        int height   = 4;
        int centerX  = Chunk::WIDTH / 2;
        int centerZ  = Chunk::DEPTH / 2;

        for (int y = 0; y < height; ++y)
        {
            int layerSize = baseSize - 2 * y;
            int startX    = centerX - layerSize / 2;
            int startZ    = centerZ - layerSize / 2;
            for (int x = 0; x < layerSize; ++x)
            {
                for (int z = 0; z < layerSize; ++z)
                {
                    chunk->setBlock(startX + x, y, startZ + z, Block(Block::Id::DIRT));
                }
            }
        }
    }

    chunk->generateMesh();
    chunk->uploadMeshToGPU();
    loadedChunks[key] = std::move(chunk);
    return *(loadedChunks[key]);
}

// Unload a chunk (save it to disk and remove it from memory)
void ChunkManager::unloadChunk(int chunkX, int chunkZ)
{
    auto key = std::make_pair(chunkX, chunkZ);

    auto it = loadedChunks.find(key);
    if (it != loadedChunks.end())
    {
        it->second->deleteMesh();

        RegionFile* region = getRegionFile(chunkX, chunkZ);
        region->saveChunk(*(it->second));
        loadedChunks.erase(it);
    }
}

void ChunkManager::unloadAllChunks()
{
    for (auto& pair : loadedChunks)
    {
        pair.second->deleteMesh();

        int         chunkX = pair.first.first;
        int         chunkZ = pair.first.second;
        RegionFile* region = getRegionFile(chunkX, chunkZ);
        region->saveChunk(*(pair.second));
    }
    loadedChunks.clear();
}

// Helper to get or create a RegionFile for a given chunk
RegionFile* ChunkManager::getRegionFile(int chunkX, int chunkZ)
{
    int  regionX = chunkX / REGION_SIZE;
    int  regionZ = chunkZ / REGION_SIZE;
    auto key     = std::make_pair(regionX, regionZ);

    auto it = regionFiles.find(key);
    if (it != regionFiles.end())
        return it->second.get();

    std::string filename = "r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";
    std::string path     = getRegionFilePath(filename);
    auto        regionFile = std::make_unique<RegionFile>(path);
    RegionFile* ptr        = regionFile.get();
    regionFiles[key]       = std::move(regionFile);
    return ptr;
}

void ChunkManager::deserializeChunk(Chunk& chunk, const std::vector<uint8_t>& data)
{
    if (data.size() < 4)
    {
        throw std::invalid_argument("Invalid chunk data: too small to contain length field");
    }

    // Length of chunk stored in big-endian format
    uint32_t compressedLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    if (data.size() - 4 < compressedLength)
    {
        throw std::invalid_argument("Invalid chunk data: not enough data for compressed length");
    }

    std::vector<uint8_t> decompressedData(Chunk::WIDTH * Chunk::HEIGHT * Chunk::DEPTH);
    uLongf               decompressedSize = decompressedData.size();

    int result = uncompress(decompressedData.data(), &decompressedSize, data.data() + 4,
                            compressedLength);  // Skip the 4-byte length field

    if (result != Z_OK)
    {
        throw std::runtime_error("Failed to decompress chunk data");
    }

    if (decompressedSize != decompressedData.size())
    {
        throw std::runtime_error("Decompressed data size does not match expected size");
    }

    size_t i = 0;
    for (int y = 0; y < Chunk::HEIGHT; ++y)
    {
        for (int z = 0; z < Chunk::DEPTH; ++z)
        {
            for (int x = 0; x < Chunk::WIDTH; ++x)
            {
                Block::Id blockId = static_cast<Block::Id>(decompressedData[i++]);
                chunk.setBlock(x, y, z, Block(blockId));
            }
        }
    }
}
