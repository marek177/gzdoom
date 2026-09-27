#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace n3d
{
struct WallVector
{
    std::uint8_t wallId = 0;
    std::uint8_t orientation = 0; // 0/1 vertical faces, 2/3 horizontal faces
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
};

struct ProjectedWallVector
{
    int vectorIndex = -1;
    double screenLeft = 0.0;
    double screenRight = 0.0;
    double halfHeightLeft = 0.0;
    double halfHeightRight = 0.0;
};

struct VisibleWallSpan
{
    int vectorIndex = -1;
    int firstX = 0;
    int lastX = 0;

    // Signed 16.16 step corresponding to the original visible-span design.
    std::int32_t halfHeightStep16_16 = 0;
};

struct ProjectionParams
{
    int viewportLeft = 8;
    int viewportRight = 311;
    double centerX = 159.5;
    double horizonY = 79.5;
    double horizontalScale = 178.0;
    double verticalWallScale = 155.859375;
    double nearPlane = 0.08;
    double cameraPlaneScale = 304.0 / (2.0 * 178.0);
};

class VectorSpanCore
{
public:
    // Build axis-aligned wall faces from a 64x64 wall-id grid. A face is emitted
    // when a non-traversable cell borders a traversable cell. Compatible
    // collinear faces are merged, mirroring Nitemare 3D's vector-generation
    // stage before projection.
    static std::vector<WallVector> BuildBoundaryVectors(
        const std::array<std::uint8_t, 64 * 64>& wallIds,
        const std::array<bool, 256>& traversableWallIds);

    static bool ProjectVector(
        const WallVector& vector,
        double cameraX,
        double cameraY,
        double cameraAngle,
        const ProjectionParams& params,
        ProjectedWallVector& projected);

    // Resolve one owning wall vector per viewport column. This is deliberately
    // a vector/column-owner pass rather than tile-by-tile DDA traversal.
    static void ResolveColumnOwners(
        const std::vector<WallVector>& vectors,
        const std::vector<ProjectedWallVector>& projected,
        double cameraX,
        double cameraY,
        double cameraAngle,
        const ProjectionParams& params,
        std::vector<int>& owners,
        std::vector<double>& depths);

    // Compress maximal equal-owner column runs into visible spans and calculate
    // the signed 16.16 vertical interpolation step used by the wall rasterizer.
    static std::vector<VisibleWallSpan> BuildVisibleSpans(
        const std::vector<int>& owners,
        const std::vector<ProjectedWallVector>& projected,
        const ProjectionParams& params);

    static bool IntersectColumnRay(
        const WallVector& vector,
        double cameraX,
        double cameraY,
        double rayX,
        double rayY,
        double nearPlane,
        double& depth,
        double& textureU);
};
}
