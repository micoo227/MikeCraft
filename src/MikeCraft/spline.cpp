#include "spline.h"

#include <algorithm>

void Spline::addPoint(float input, float output)
{
    points.push_back({input, output});
    std::sort(points.begin(), points.end(),
              [](const SplinePoint& a, const SplinePoint& b) { return a.input < b.input; });
}

float Spline::evaluate(float input) const
{
    if (points.empty())
        return 0.0f;
    if (input <= points.front().input)
        return points.front().output;
    if (input >= points.back().input)
        return points.back().output;

    for (size_t i = 1; i < points.size(); ++i)
    {
        if (input < points[i].input)
        {
            const SplinePoint& p0 = points[i - 1];
            const SplinePoint& p1 = points[i];
            float              t  = (input - p0.input) / (p1.input - p0.input);
            return p0.output + t * (p1.output - p0.output);
        }
    }
    return points.back().output;  // Fallback, should not be reached
}