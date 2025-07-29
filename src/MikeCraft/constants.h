#pragma once

constexpr int SECTOR_BYTES = 4096;

// 32x32 chunks per region
constexpr int REGION_SIZE = 32;

// 16x16 blocks per chunk
constexpr int CHUNK_SIZE = 16;

constexpr int REGION_BLOCKS     = REGION_SIZE * CHUNK_SIZE;
constexpr int NOISE_SAMPLE_STEP = 4;

// Add 1 to ensure smooth interpolation between regions
constexpr int NOISE_GRID_SIZE = REGION_BLOCKS / NOISE_SAMPLE_STEP + 1;

// TODO: change names for these constants