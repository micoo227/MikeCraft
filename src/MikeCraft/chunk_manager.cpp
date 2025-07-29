#include "chunk_manager.h"
#include "constants.h"
#include "file_utils.h"
#include "world_generator.h"
#include "zlib/zlib.h"

#include <stdexcept>
#include <vector>

ChunkManager::ChunkManager(WorldGenerator& generator) : worldGenerator(generator)
{
    workerThread = std::thread(&ChunkManager::workerFunc, this);
}

ChunkManager::~ChunkManager()
{
    stopWorker();
}

std::vector<Chunk*> ChunkManager::getLoadedChunks() const
{
    std::vector<Chunk*> result;
    for (const auto& pair : loadedChunks)
        result.push_back(pair.second.get());
    return result;
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

void ChunkManager::updateChunksAroundPlayer(float playerX, float playerZ, int renderRadius)
{
    int playerChunkX = static_cast<int>(std::floor(playerX / Chunk::WIDTH));
    int playerChunkZ = static_cast<int>(std::floor(playerZ / Chunk::DEPTH));

    for (int dx = -renderRadius; dx <= renderRadius; ++dx)
    {
        for (int dz = -renderRadius; dz <= renderRadius; ++dz)
        {
            int  chunkX = playerChunkX + dx;
            int  chunkZ = playerChunkZ + dz;
            auto key    = std::make_pair(chunkX, chunkZ);
            if (loadedChunks.find(key) == loadedChunks.end())
                enqueueChunkLoad(chunkX, chunkZ);
        }
    }

    std::vector<std::pair<int, int>> toUnload;
    for (const auto& pair : loadedChunks)
    {
        int chunkX = pair.first.first;
        int chunkZ = pair.first.second;
        int dx     = chunkX - playerChunkX;
        int dz     = chunkZ - playerChunkZ;
        if (std::abs(dx) > renderRadius || std::abs(dz) > renderRadius)
        {
            toUnload.emplace_back(chunkX, chunkZ);
        }
    }
    for (const auto& key : toUnload)
    {
        unloadChunk(key.first, key.second);
    }
}

void ChunkManager::processChunkUploads()
{
    std::lock_guard<std::mutex> lock(readyMutex);
    int                         uploadsThisFrame   = 0;
    const int                   maxUploadsPerFrame = 2;

    while (!readyChunks.empty() && uploadsThisFrame < maxUploadsPerFrame)
    {
        auto& pending = readyChunks.front();
        pending.chunk->uploadMeshToGPU();
        loadedChunks[{pending.chunkX, pending.chunkZ}] = std::move(pending.chunk);
        readyChunks.pop();
        ++uploadsThisFrame;
    }
}

void ChunkManager::stopWorker()
{
    stopWorkerThread = true;
    queueCV.notify_all();
    if (workerThread.joinable())
        workerThread.join();
}

// Helper to get or create a RegionFile for a given chunk
RegionFile* ChunkManager::getRegionFile(int chunkX, int chunkZ)
{
    std::lock_guard<std::mutex> lock(regionFilesMutex);

    int  regionX = static_cast<int>(std::floor(static_cast<double>(chunkX) / REGION_SIZE));
    int  regionZ = static_cast<int>(std::floor(static_cast<double>(chunkZ) / REGION_SIZE));
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

void ChunkManager::workerFunc()
{
    while (!stopWorkerThread)
    {
        std::pair<int, int> request;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [&] { return !chunkLoadQueue.empty() || stopWorkerThread; });
            if (stopWorkerThread)
                break;
            request = chunkLoadQueue.front();
            chunkLoadQueue.pop();
        }

        int chunkX = request.first;
        int chunkZ = request.second;

        RegionFile* region = getRegionFile(chunkX, chunkZ);
        auto        data   = region->loadChunk(chunkX, chunkZ);
        auto        chunk  = std::make_unique<Chunk>(chunkX, chunkZ);
        if (!data.empty())
            deserializeChunk(*chunk, data);
        else
        {
            int regionX = static_cast<int>(std::floor(static_cast<double>(chunkX) / REGION_SIZE));
            int regionZ = static_cast<int>(std::floor(static_cast<double>(chunkZ) / REGION_SIZE));
            region->generateNoiseGrids(worldGenerator, regionX, regionZ, 0.01f,
                                       0);  // Temp frequency and seed

            for (int x = 0; x < Chunk::WIDTH; ++x)
            {
                for (int z = 0; z < Chunk::DEPTH; ++z)
                {
                    // TODO: make below more clear (move into WorldGenerator)
                    int regionBlockX = x + (chunkX & (REGION_SIZE - 1)) * Chunk::WIDTH;
                    int regionBlockZ = z + (chunkZ & (REGION_SIZE - 1)) * Chunk::DEPTH;

                    // clang-format off
                    float continentNoise = worldGenerator.getInterpolatedNoise(
                        region->continentGrid, regionBlockX, regionBlockZ);
                    float erosionNoise = worldGenerator.getInterpolatedNoise(
                        region->erosionGrid, regionBlockX, regionBlockZ);
                    float pvNoise = worldGenerator.getInterpolatedNoise(
                        region->pvGrid, regionBlockX, regionBlockZ);
                    // clang-format on

                    float continentVal = worldGenerator.continentSpline.evaluate(continentNoise);
                    float erosionVal   = worldGenerator.erosionSpline.evaluate(erosionNoise);
                    float pvVal        = worldGenerator.pvSpline.evaluate(pvNoise);

                    // float targetHeight = continentVal * erosionVal + pvVal * 10.0f;
                    float targetHeight = continentVal;
                    int   blockY       = static_cast<int>(targetHeight);

                    for (int y = 0; y < blockY && y < Chunk::HEIGHT; ++y)
                    {
                        Block::Id id = Block::Id::DIRT;
                        if (y == blockY - 1)
                            id = Block::Id::GRASS;
                        if (y < blockY - 5)
                            id = Block::Id::STONE;
                        chunk->setBlock(x, y, z, Block(id));
                    }
                }
            }
        }
        chunk->generateMesh();

        {
            std::lock_guard<std::mutex> lock(readyMutex);
            readyChunks.push({chunkX, chunkZ, std::move(chunk)});
        }
    }
}

void ChunkManager::enqueueChunkLoad(int chunkX, int chunkZ)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    chunkLoadQueue.emplace(chunkX, chunkZ);
    queueCV.notify_one();
}
