#include "constants.h"
#include "world_generator.h"

WorldGenerator::WorldGenerator(int seed)
{
    continentPerlin  = FastNoise::New<FastNoise::Perlin>();
    continentFractal = FastNoise::New<FastNoise::FractalFBm>();

    continentFractal->SetSource(continentPerlin);
    continentFractal->SetOctaveCount(5);

    erosionPerlin  = FastNoise::New<FastNoise::Perlin>();
    erosionFractal = FastNoise::New<FastNoise::FractalFBm>();

    erosionFractal->SetSource(erosionPerlin);
    erosionFractal->SetOctaveCount(4);

    pvPerlin  = FastNoise::New<FastNoise::Perlin>();
    pvFractal = FastNoise::New<FastNoise::FractalPingPong>();

    pvFractal->SetSource(pvPerlin);
    pvFractal->SetOctaveCount(3);

    continentSpline.addPoint(-1.0f, 40.0f);
    continentSpline.addPoint(-0.2f, 60.0f);
    continentSpline.addPoint(0.0f, 70.0f);
    continentSpline.addPoint(0.5f, 100.0f);
    continentSpline.addPoint(1.0f, 140.0f);

    erosionSpline.addPoint(-1.0f, 0.0f);
    erosionSpline.addPoint(0.0f, 1.0f);
    erosionSpline.addPoint(1.0f, 0.0f);

    pvSpline.addPoint(-1.0f, 0.0f);
    pvSpline.addPoint(0.0f, 1.0f);
    pvSpline.addPoint(1.0f, 0.0f);
}

void WorldGenerator::generateRegionNoiseGrids(std::vector<float>& continentGrid,
                                              std::vector<float>& erosionGrid,
                                              std::vector<float>& pvGrid, int regionX, int regionZ,
                                              float frequency, int seed) const
{
    continentGrid.resize(NOISE_GRID_SIZE * NOISE_GRID_SIZE);
    erosionGrid.resize(NOISE_GRID_SIZE * NOISE_GRID_SIZE);
    pvGrid.resize(NOISE_GRID_SIZE * NOISE_GRID_SIZE);

    int startX = regionX * REGION_BLOCKS / NOISE_SAMPLE_STEP;
    int startZ = regionZ * REGION_BLOCKS / NOISE_SAMPLE_STEP;

    continentFractal->GenUniformGrid2D(continentGrid.data(), startX, startZ, NOISE_GRID_SIZE,
                                       NOISE_GRID_SIZE, NOISE_SAMPLE_STEP * frequency, seed);
    erosionFractal->GenUniformGrid2D(erosionGrid.data(), startX, startZ, NOISE_GRID_SIZE,
                                     NOISE_GRID_SIZE, NOISE_SAMPLE_STEP * frequency, seed);
    pvFractal->GenUniformGrid2D(pvGrid.data(), startX, startZ, NOISE_GRID_SIZE, NOISE_GRID_SIZE,
                                NOISE_SAMPLE_STEP * frequency, seed);
}

float WorldGenerator::getInterpolatedNoise(const std::vector<float>& grid, int blockX,
                                           int blockZ) const
{
    float gx = static_cast<float>(blockX) / NOISE_SAMPLE_STEP;
    float gz = static_cast<float>(blockZ) / NOISE_SAMPLE_STEP;

    int ix = static_cast<int>(gx);
    int iz = static_cast<int>(gz);

    float tx = gx - ix;
    float tz = gz - iz;

    // Bilinear interpolation
    float q11 = grid[iz * NOISE_GRID_SIZE + ix];
    float q21 = grid[iz * NOISE_GRID_SIZE + ix + 1];
    float q12 = grid[(iz + 1) * NOISE_GRID_SIZE + ix];
    float q22 = grid[(iz + 1) * NOISE_GRID_SIZE + ix + 1];

    float interpX1 = q11 * (1 - tx) + q21 * tx;
    float interpX2 = q12 * (1 - tx) + q22 * tx;
    return interpX1 * (1 - tz) + interpX2 * tz;
}