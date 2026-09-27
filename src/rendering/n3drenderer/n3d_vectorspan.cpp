#include "n3d_vectorspan.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace n3d
{
namespace
{
constexpr int MapWidth = 64;
constexpr int MapHeight = 64;

bool InMap(int x, int y)
{
    return x >= 0 && y >= 0 && x < MapWidth && y < MapHeight;
}

bool SameLine(const WallVector& a, const WallVector& b)
{
    if (a.wallId != b.wallId || a.orientation != b.orientation) return false;
    if (a.orientation < 2) return std::abs(a.x0 - b.x0) < 1e-9;
    return std::abs(a.y0 - b.y0) < 1e-9;
}
}

std::vector<WallVector> VectorSpanCore::BuildBoundaryVectors(
    const std::array<std::uint8_t, 64 * 64>& wallIds,
    const std::array<bool, 256>& traversable)
{
    auto idAt = [&](int x, int y) -> std::uint8_t {
        return wallIds[static_cast<std::size_t>(y) * MapWidth + x];
    };
    auto canEnter = [&](int x, int y) {
        return InMap(x, y) && traversable[idAt(x, y)];
    };

    std::vector<WallVector> vectors;
    vectors.reserve(1000);

    for (int y = 0; y < MapHeight; ++y)
    {
        for (int x = 0; x < MapWidth; ++x)
        {
            const auto id = idAt(x, y);
            if (traversable[id]) continue;

            if (canEnter(x - 1, y)) vectors.push_back({id, 0, double(x),     double(y),     double(x),     double(y + 1)});
            if (canEnter(x + 1, y)) vectors.push_back({id, 1, double(x + 1), double(y + 1), double(x + 1), double(y)});
            if (canEnter(x, y - 1)) vectors.push_back({id, 2, double(x + 1), double(y),     double(x),     double(y)});
            if (canEnter(x, y + 1)) vectors.push_back({id, 3, double(x),     double(y + 1), double(x + 1), double(y + 1)});
        }
    }

    std::sort(vectors.begin(), vectors.end(), [](const WallVector& a, const WallVector& b) {
        if (a.orientation != b.orientation) return a.orientation < b.orientation;
        if (a.wallId != b.wallId) return a.wallId < b.wallId;
        if (a.orientation < 2)
        {
            if (a.x0 != b.x0) return a.x0 < b.x0;
            return std::min(a.y0, a.y1) < std::min(b.y0, b.y1);
        }
        if (a.y0 != b.y0) return a.y0 < b.y0;
        return std::min(a.x0, a.x1) < std::min(b.x0, b.x1);
    });

    std::vector<WallVector> merged;
    merged.reserve(vectors.size());

    for (const auto& v : vectors)
    {
        if (!merged.empty() && SameLine(merged.back(), v))
        {
            auto& m = merged.back();
            if (m.orientation < 2)
            {
                const double mMax = std::max(m.y0, m.y1);
                const double vMin = std::min(v.y0, v.y1);
                if (std::abs(mMax - vMin) < 1e-9)
                {
                    if (m.y0 < m.y1) m.y1 = std::max(v.y0, v.y1);
                    else             m.y0 = std::max(v.y0, v.y1);
                    continue;
                }
            }
            else
            {
                const double mMax = std::max(m.x0, m.x1);
                const double vMin = std::min(v.x0, v.x1);
                if (std::abs(mMax - vMin) < 1e-9)
                {
                    if (m.x0 < m.x1) m.x1 = std::max(v.x0, v.x1);
                    else             m.x0 = std::max(v.x0, v.x1);
                    continue;
                }
            }
        }
        merged.push_back(v);
    }

    return merged;
}

bool VectorSpanCore::ProjectVector(
    const WallVector& vector,
    double cameraX,
    double cameraY,
    double cameraAngle,
    const ProjectionParams& p,
    ProjectedWallVector& out)
{
    const double dirX = std::cos(cameraAngle);
    const double dirY = std::sin(cameraAngle);

    auto transform = [&](double x, double y, double& depth, double& lateral) {
        const double dx = x - cameraX;
        const double dy = y - cameraY;
        depth = dx * dirX + dy * dirY;
        lateral = -dx * dirY + dy * dirX;
    };

    double d0, l0, d1, l1;
    transform(vector.x0, vector.y0, d0, l0);
    transform(vector.x1, vector.y1, d1, l1);

    if (d0 <= p.nearPlane && d1 <= p.nearPlane) return false;

    if (d0 <= p.nearPlane)
    {
        const double t = (p.nearPlane - d0) / (d1 - d0);
        l0 += t * (l1 - l0);
        d0 = p.nearPlane;
    }
    if (d1 <= p.nearPlane)
    {
        const double t = (p.nearPlane - d1) / (d0 - d1);
        l1 += t * (l0 - l1);
        d1 = p.nearPlane;
    }

    double sx0 = p.centerX + p.horizontalScale * l0 / d0;
    double sx1 = p.centerX + p.horizontalScale * l1 / d1;
    double h0 = p.verticalWallScale / (2.0 * d0);
    double h1 = p.verticalWallScale / (2.0 * d1);

    if (sx1 < sx0)
    {
        std::swap(sx0, sx1);
        std::swap(h0, h1);
    }

    if (sx1 < p.viewportLeft || sx0 > p.viewportRight || std::abs(sx1 - sx0) < 1e-9)
        return false;

    out.screenLeft = sx0;
    out.screenRight = sx1;
    out.halfHeightLeft = h0;
    out.halfHeightRight = h1;
    return true;
}

bool VectorSpanCore::IntersectColumnRay(
    const WallVector& vector,
    double cameraX,
    double cameraY,
    double rayX,
    double rayY,
    double nearPlane,
    double& depth,
    double& textureU)
{
    constexpr double Epsilon = 1e-9;

    if (vector.orientation < 2)
    {
        if (std::abs(rayX) < Epsilon) return false;
        depth = (vector.x0 - cameraX) / rayX;
        if (depth <= nearPlane) return false;

        const double y = cameraY + depth * rayY;
        if (y < std::min(vector.y0, vector.y1) - Epsilon ||
            y > std::max(vector.y0, vector.y1) + Epsilon) return false;

        textureU = y - std::floor(y);
        return true;
    }

    if (std::abs(rayY) < Epsilon) return false;
    depth = (vector.y0 - cameraY) / rayY;
    if (depth <= nearPlane) return false;

    const double x = cameraX + depth * rayX;
    if (x < std::min(vector.x0, vector.x1) - Epsilon ||
        x > std::max(vector.x0, vector.x1) + Epsilon) return false;

    textureU = x - std::floor(x);
    return true;
}

void VectorSpanCore::ResolveColumnOwners(
    const std::vector<WallVector>& vectors,
    const std::vector<ProjectedWallVector>& projected,
    double cameraX,
    double cameraY,
    double cameraAngle,
    const ProjectionParams& p,
    std::vector<int>& owners,
    std::vector<double>& depths)
{
    const int width = p.viewportRight + 1;
    owners.assign(width, -1);
    depths.assign(width, std::numeric_limits<double>::infinity());

    const double dirX = std::cos(cameraAngle);
    const double dirY = std::sin(cameraAngle);
    const double planeX = -dirY * p.cameraPlaneScale;
    const double planeY =  dirX * p.cameraPlaneScale;
    const double halfWidth = (p.viewportRight - p.viewportLeft + 1) * 0.5;

    for (const auto& pr : projected)
    {
        if (pr.vectorIndex < 0 || pr.vectorIndex >= static_cast<int>(vectors.size())) continue;

        const int x0 = std::max(p.viewportLeft, static_cast<int>(std::ceil(pr.screenLeft)));
        const int x1 = std::min(p.viewportRight, static_cast<int>(std::floor(pr.screenRight)));

        for (int x = x0; x <= x1; ++x)
        {
            const double cameraXNorm = (x - p.centerX) / halfWidth;
            const double rayX = dirX + planeX * cameraXNorm;
            const double rayY = dirY + planeY * cameraXNorm;

            double depth, u;
            if (!IntersectColumnRay(vectors[pr.vectorIndex], cameraX, cameraY,
                                    rayX, rayY, p.nearPlane, depth, u))
                continue;

            if (depth < depths[x])
            {
                depths[x] = depth;
                owners[x] = pr.vectorIndex;
            }
        }
    }
}

std::vector<VisibleWallSpan> VectorSpanCore::BuildVisibleSpans(
    const std::vector<int>& owners,
    const std::vector<ProjectedWallVector>& projected,
    const ProjectionParams& p)
{
    std::vector<VisibleWallSpan> spans;

    int x = p.viewportLeft;
    while (x <= p.viewportRight)
    {
        const int owner = (x < static_cast<int>(owners.size())) ? owners[x] : -1;
        int end = x;
        while (end + 1 <= p.viewportRight &&
               end + 1 < static_cast<int>(owners.size()) &&
               owners[end + 1] == owner)
            ++end;

        if (owner >= 0 && owner < static_cast<int>(projected.size()))
        {
            const auto& pr = projected[owner];
            const double dx = pr.screenRight - pr.screenLeft;
            std::int32_t step = 0;
            if (std::abs(dx) > 1e-9)
            {
                const double delta = (pr.halfHeightRight - pr.halfHeightLeft) / dx;
                step = static_cast<std::int32_t>(std::llround(delta * 65536.0));
            }
            spans.push_back({owner, x, end, step});
        }

        x = end + 1;
    }

    return spans;
}
}
