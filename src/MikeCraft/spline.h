#pragma once

#include <vector>

struct SplinePoint
{
    float input;
    float output;
};

class Spline
{
   public:
    void addPoint(float input, float output);

    float evaluate(float input) const;

    const std::vector<SplinePoint>& getPoints() const
    {
        return points;
    }

   private:
    std::vector<SplinePoint> points;
};