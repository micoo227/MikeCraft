#pragma once

#include "spline.h"

#include <FastNoise/FastNoise.h>

class WorldGenerator
{
   public:
    WorldGenerator(int seed = 0);

    void  generateRegionNoiseGrids(std::vector<float>& continentGrid,
                                   std::vector<float>& erosionGrid, std::vector<float>& pvGrid,
                                   int regionX, int regionZ, float frequency, int seed) const;
    float getInterpolatedNoise(const std::vector<float>& grid, int blockX, int blockZ) const;

    Spline continentSpline;
    Spline erosionSpline;
    Spline pvSpline;

   private:
    FastNoise::SmartNode<FastNoise::Perlin>          continentPerlin;
    FastNoise::SmartNode<FastNoise::FractalFBm>      continentFractal;
    FastNoise::SmartNode<FastNoise::Perlin>          erosionPerlin;
    FastNoise::SmartNode<FastNoise::FractalFBm>      erosionFractal;
    FastNoise::SmartNode<FastNoise::Perlin>          pvPerlin;
    FastNoise::SmartNode<FastNoise::FractalPingPong> pvFractal;
};