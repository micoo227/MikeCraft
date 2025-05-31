#include "constants.h"
#include "region_file.h"

RegionFile::RegionFile(const std::string& filePath) : filePath(filePath)
{
    file.open(filePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file)
    {
        initializeRegionFile();
        file.open(filePath, std::ios::binary | std::ios::in | std::ios::out);
        if (!file)
        {
            throw std::runtime_error("Failed to open region file: " + filePath);
        }
    }
    rebuildFreeList();
}

// Helper to load a chunk from the region file
std::vector<uint8_t> RegionFile::loadChunk(int chunkX, int chunkZ)
{
    // Use & instead of % to prevent negative values
    int index = 4 * ((chunkX & (REGION_SIZE - 1)) + (chunkZ & (REGION_SIZE - 1)) * REGION_SIZE);

    file.seekg(index, std::ios::beg);
    uint32_t locationEntry;
    file.read(reinterpret_cast<char*>(&locationEntry), sizeof(locationEntry));

    // First 3 bytes are the offset, last byte is the size
    // Stored in 4KiB sectors
    uint32_t offsetBytes = (locationEntry >> 8) * SECTOR_BYTES;
    uint8_t  sectorCount = locationEntry & 0xFF;

    if (offsetBytes == 0 || sectorCount == 0)
    {
        // Chunk not found
        return {};
    }

    file.seekg(offsetBytes, std::ios::beg);
    std::vector<uint8_t> data(sectorCount * SECTOR_BYTES);
    file.read(reinterpret_cast<char*>(data.data()), data.size());

    return data;
}

void RegionFile::saveChunk(const Chunk& chunk)
{
    // Use & instead of % to prevent negative values
    int index =
        4 * ((chunk.getX() & (REGION_SIZE - 1)) + (chunk.getZ() & (REGION_SIZE - 1)) * REGION_SIZE);

    file.seekg(index, std::ios::beg);
    uint32_t locationEntry;
    file.read(reinterpret_cast<char*>(&locationEntry), sizeof(locationEntry));

    uint32_t currentSectorOffset = (locationEntry >> 8);
    uint32_t currentOffsetBytes  = (locationEntry >> 8) * SECTOR_BYTES;
    uint8_t  currentSectorCount  = locationEntry & 0xFF;

    auto data = chunk.serialize();
    // Pad chunk data to be a multiple of 4 KiB
    uint32_t newSectorCount = (data.size() + SECTOR_BYTES - 1) / SECTOR_BYTES;
    data.resize(newSectorCount * SECTOR_BYTES, 0);

    uint32_t newSectorOffset;

    if (currentSectorOffset != 0 && newSectorCount <= currentSectorCount)
    {
        newSectorOffset = currentSectorOffset;
        file.seekp(currentOffsetBytes, std::ios::beg);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    else
    {
        if (currentSectorOffset != 0)
        {
            addFreeRegion(currentSectorOffset, currentSectorCount);
        }

        auto freeRegion = findFreeRegion(newSectorCount);
        if (freeRegion)
        {
            newSectorOffset = freeRegion->offset;
        }
        else
        {
            file.seekp(0, std::ios::end);
            newSectorOffset = file.tellp() / SECTOR_BYTES;
        }

        file.seekp(newSectorOffset * SECTOR_BYTES, std::ios::beg);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    locationEntry = (newSectorOffset << 8) | newSectorCount;
    file.seekp(index, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&locationEntry), sizeof(locationEntry));

    uint32_t timestamp = static_cast<uint32_t>(time(nullptr));
    file.seekp(SECTOR_BYTES + index, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
}

void RegionFile::rebuildFreeList()
{
    const int LOCATION_TABLE_SIZE = REGION_SIZE * REGION_SIZE;

    file.seekg(0, std::ios::end);
    std::streamsize fileSize     = file.tellg();
    uint32_t        totalSectors = static_cast<uint32_t>(fileSize / SECTOR_BYTES);

    std::vector<bool> sectorUsed(totalSectors, false);
    sectorUsed[0] = true;
    sectorUsed[1] = true;

    file.seekg(0, std::ios::beg);
    for (int i = 0; i < LOCATION_TABLE_SIZE; ++i)
    {
        uint8_t entry[4];
        file.read(reinterpret_cast<char*>(entry), 4);
        uint32_t offset      = (entry[0] << 16) | (entry[1] << 8) | entry[2];
        uint8_t  sectorCount = entry[3];
        if (offset != 0 && sectorCount != 0)
        {
            for (uint8_t s = 0; s < sectorCount; ++s)
            {
                uint32_t sector = offset + s;
                if (sector < totalSectors)
                    sectorUsed[sector] = true;
            }
        }
    }

    freeList.clear();
    uint32_t start = 0;
    while (start < totalSectors)
    {
        while (start < totalSectors && sectorUsed[start])
            ++start;
        if (start >= totalSectors)
            break;
        uint32_t end = start;
        while (end < totalSectors && !sectorUsed[end])
            ++end;
        freeList.push_back({start, end - start});
        start = end;
    }
}

std::optional<RegionFile::FreeRegion> RegionFile::findFreeRegion(uint32_t sectorCount)
{
    for (auto it = freeList.begin(); it != freeList.end(); ++it)
    {
        if (sectorCount <= it->size)
        {
            FreeRegion region = *it;

            // If the free region is larger than needed, split it
            if (it->size > sectorCount)
            {
                it->offset += sectorCount;
                it->size -= sectorCount;
            }
            else
            {
                // Remove the region from the free list if it's fully used
                freeList.erase(it);
            }

            return region;
        }
    }

    return std::nullopt;
}

void RegionFile::addFreeRegion(uint32_t offset, uint32_t size)
{
    for (auto it = freeList.begin(); it != freeList.end(); ++it)
    {
        // Merge with an adjacent free region
        if (it->offset + it->size == offset)
        {
            it->size += size;
            return;
        }
        else if (offset + size == it->offset)
        {
            it->offset = offset;
            it->size += size;
            return;
        }
    }

    // No adjacent region found, add a new entry
    freeList.push_back({offset, size});
}

void RegionFile::initializeRegionFile()
{
    std::ofstream out(filePath, std::ios::binary);
    if (!out)
    {
        throw std::runtime_error("Failed to create region file: " + filePath);
    }

    // 1024 * 4 bytes for the location table
    // 1024 * 4 bytes for the timestamp table
    const size_t HEADER_SIZE = (REGION_SIZE * REGION_SIZE * 4) * 2;

    std::vector<uint8_t> header(HEADER_SIZE, 0);
    out.write(reinterpret_cast<const char*>(header.data()), header.size());
}