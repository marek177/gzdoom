#include "n3d_preview_core.h"

#include <algorithm>
#include <cmath>

namespace n3dpreview
{
namespace
{
constexpr double Pi = 3.14159265358979323846;

double Lerp(double a, double b, double t) { return a + (b - a) * t; }

double WrapAngle(double a)
{
    while (a > Pi) a -= 2.0 * Pi;
    while (a < -Pi) a += 2.0 * Pi;
    return a;
}

double LerpAngle(double a, double b, double t)
{
    return a + WrapAngle(b - a) * t;
}

struct Waypoint { double x, y, angle, time; };

constexpr Waypoint Route[] = {
    { 6.0,  6.0,  0.00,   0.0},
    {12.5,  6.0,  0.00,  14.0},
    {20.5,  6.0,  0.00,  28.0},
    {24.5,  7.0,  0.70,  40.0},
    {25.0, 13.5,  1.57,  54.0},
    {25.0, 22.0,  1.57,  68.0},
    {17.0, 23.0,  3.14,  82.0},
    { 7.0, 23.0,  3.14,  96.0},
    { 7.0, 14.0, -1.57, 108.0},
    { 6.0,  6.0, -1.70, 120.0}
};

inline int Quantize(int v)
{
    v = std::clamp(v, 0, 255);
    return (v / 8) * 8;
}
}

PreviewRenderer::PreviewRenderer()
{
    BuildMap();
}

void PreviewRenderer::BuildMap()
{
    for (int y = 0; y < MapH; ++y)
        for (int x = 0; x < MapW; ++x)
            map_[y][x] = 4;

    auto carve = [&](int x0, int y0, int x1, int y1)
    {
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                map_[y][x] = 0;
    };

    carve(3, 3, 10, 9);
    carve(10, 5, 18, 7);
    carve(17, 3, 27, 9);
    carve(24, 8, 26, 19);
    carve(18, 18, 28, 27);
    carve(9, 22, 18, 24);
    carve(3, 18, 10, 27);
    carve(6, 9, 8, 19);

    auto paintBoundary = [&](int x0, int y0, int x1, int y1, int mat)
    {
        for (int y = y0 - 1; y <= y1 + 1; ++y)
        {
            for (int x = x0 - 1; x <= x1 + 1; ++x)
            {
                if (x < 0 || y < 0 || x >= MapW || y >= MapH) continue;
                if (x >= x0 && x <= x1 && y >= y0 && y <= y1) continue;
                if (map_[y][x] != 0) map_[y][x] = mat;
            }
        }
    };

    paintBoundary(3, 3, 10, 9, 1);
    paintBoundary(17, 3, 27, 9, 2);
    paintBoundary(18, 18, 28, 27, 1);
    paintBoundary(3, 18, 10, 27, 5);

    map_[4][10] = 3;
    map_[8][17] = 3;
    map_[17][25] = 3;
    map_[21][18] = 5;
    map_[17][7] = 3;
}

Camera PreviewRenderer::CameraAt(double t) const
{
    t = std::clamp(t, 0.0, 120.0);
    constexpr int Count = int(sizeof(Route) / sizeof(Route[0]));
    int i = 0;
    while (i + 1 < Count && t > Route[i + 1].time) ++i;
    if (i + 1 >= Count) return {Route[Count - 1].x, Route[Count - 1].y, Route[Count - 1].angle};

    const auto& a = Route[i];
    const auto& b = Route[i + 1];
    double u = (t - a.time) / (b.time - a.time);
    u = u * u * (3.0 - 2.0 * u);

    Camera c;
    c.x = Lerp(a.x, b.x, u);
    c.y = Lerp(a.y, b.y, u);
    c.angle = LerpAngle(a.angle, b.angle, u) + std::sin(t * 1.6) * 0.012;
    return c;
}

void PreviewRenderer::PutPixel(std::vector<std::uint8_t>& rgb, int x, int y, int r, int g, int b) const
{
    if (x < 0 || y < 0 || x >= Width || y >= Height) return;
    const std::size_t o = (std::size_t(y) * Width + x) * 3;
    rgb[o + 0] = std::uint8_t(Quantize(r));
    rgb[o + 1] = std::uint8_t(Quantize(g));
    rgb[o + 2] = std::uint8_t(Quantize(b));
}

void PreviewRenderer::DrawRect(std::vector<std::uint8_t>& rgb, int x0, int y0, int x1, int y1, int r, int g, int b) const
{
    x0 = std::max(x0, 0); y0 = std::max(y0, 0);
    x1 = std::min(x1, Width - 1); y1 = std::min(y1, Height - 1);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            PutPixel(rgb, x, y, r, g, b);
}

void PreviewRenderer::TextureSample(int material, int tx, int ty, int& r, int& g, int& b) const
{
    tx &= 63;
    ty &= 63;

    switch (material)
    {
    case 1:
    {
        int stripe = ((tx / 4) & 1) ? 28 : -12;
        int trim = (ty < 6 || ty > 57) ? 35 : 0;
        r = 160 + stripe + trim;
        g = 152 + stripe + trim;
        b = 112 + stripe / 2 + trim / 2;
        break;
    }
    case 2:
    {
        int ring = ((tx / 8 + ty / 8) & 1) ? 30 : -15;
        int line = ((tx % 16 == 2 || ty % 16 == 2) ? 35 : 0);
        r = 130 + ring + line;
        g = 38 + line / 3;
        b = 30 + line / 4;
        break;
    }
    case 3:
    {
        int plank = ((tx / 8) & 1) ? 22 : -8;
        int band = (ty % 20 < 3) ? 35 : 0;
        r = 126 + plank + band;
        g = 70 + plank / 2 + band / 2;
        b = 28 + band / 4;
        break;
    }
    case 5:
    {
        int fold = int(24.0 * std::sin(tx * 0.42));
        r = 118 + fold;
        g = 25 + fold / 5;
        b = 32 + fold / 4;
        break;
    }
    default:
    {
        int block = ((tx / 8 + ty / 8) & 1) ? 18 : -8;
        int mortar = ((tx % 8 == 0) || (ty % 8 == 0)) ? -35 : 0;
        r = 95 + block + mortar;
        g = 92 + block + mortar;
        b = 82 + block + mortar;
        break;
    }
    }
}

void PreviewRenderer::DrawWorld(const Camera& c, std::vector<std::uint8_t>& rgb) const
{
    const int viewH = ViewBottom - ViewTop + 1;
    DrawRect(rgb, 0, ViewTop, Width - 1, ViewTop + viewH / 2, 52, 52, 52);
    DrawRect(rgb, 0, ViewTop + viewH / 2 + 1, Width - 1, ViewBottom, 8, 8, 8);

    const double dirX = std::cos(c.angle);
    const double dirY = std::sin(c.angle);
    const double planeScale = std::tan(60.0 * Pi / 360.0);
    const double planeX = -dirY * planeScale;
    const double planeY =  dirX * planeScale;

    std::vector<double> zbuf(Width, 1e30);

    for (int x = 0; x < Width; ++x)
    {
        const double cameraX = 2.0 * x / double(Width) - 1.0;
        const double rayDirX = dirX + planeX * cameraX;
        const double rayDirY = dirY + planeY * cameraX;

        int mapX = int(c.x);
        int mapY = int(c.y);
        const double deltaDistX = std::abs(rayDirX) < 1e-9 ? 1e30 : std::abs(1.0 / rayDirX);
        const double deltaDistY = std::abs(rayDirY) < 1e-9 ? 1e30 : std::abs(1.0 / rayDirY);
        double sideDistX, sideDistY;
        int stepX, stepY;

        if (rayDirX < 0) { stepX = -1; sideDistX = (c.x - mapX) * deltaDistX; }
        else             { stepX =  1; sideDistX = (mapX + 1.0 - c.x) * deltaDistX; }
        if (rayDirY < 0) { stepY = -1; sideDistY = (c.y - mapY) * deltaDistY; }
        else             { stepY =  1; sideDistY = (mapY + 1.0 - c.y) * deltaDistY; }

        int side = 0;
        int material = 4;
        for (int guard = 0; guard < 128; ++guard)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (mapX < 0 || mapY < 0 || mapX >= MapW || mapY >= MapH) break;
            if (map_[mapY][mapX] != 0)
            {
                material = map_[mapY][mapX];
                break;
            }
        }

        double perpDist;
        if (side == 0) perpDist = (mapX - c.x + (1 - stepX) * 0.5) / rayDirX;
        else           perpDist = (mapY - c.y + (1 - stepY) * 0.5) / rayDirY;

        perpDist = std::max(perpDist, 0.05);
        zbuf[x] = perpDist;

        int lineHeight = int((viewH * 2.15) / perpDist);
        int drawStart = ViewTop + viewH / 2 - lineHeight / 2;
        int drawEnd   = ViewTop + viewH / 2 + lineHeight / 2;
        drawStart = std::max(drawStart, ViewTop);
        drawEnd = std::min(drawEnd, ViewBottom);

        double wallX = (side == 0) ? (c.y + perpDist * rayDirY) : (c.x + perpDist * rayDirX);
        wallX -= std::floor(wallX);

        int texX = int(wallX * 64.0);
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
            texX = 63 - texX;

        const double texStep = 64.0 / std::max(lineHeight, 1);
        double texPos = (drawStart - (ViewTop + viewH / 2) + lineHeight / 2.0) * texStep;
        double shade = 1.0 / (1.0 + perpDist * 0.10);
        shade = std::clamp(shade + 0.28, 0.30, 1.0);
        if (side) shade *= 0.82;

        for (int y = drawStart; y <= drawEnd; ++y)
        {
            int texY = int(texPos) & 63;
            texPos += texStep;

            int r, g, b;
            TextureSample(material, texX, texY, r, g, b);
            PutPixel(rgb, x, y, int(r * shade), int(g * shade), int(b * shade));
        }
    }

    struct Obj { double x, y; int type; };
    constexpr Obj objects[] = {
        {8.6, 6.8, 0},
        {21.7, 6.5, 1},
        {25.4, 20.8, 2},
        {7.6, 21.3, 0}
    };

    const double invDet = 1.0 / (planeX * dirY - dirX * planeY);

    for (const auto& o : objects)
    {
        const double sx = o.x - c.x;
        const double sy = o.y - c.y;
        const double transformX = invDet * (dirY * sx - dirX * sy);
        const double transformY = invDet * (-planeY * sx + planeX * sy);
        if (transformY <= 0.15) continue;

        int screenX = int((Width / 2.0) * (1.0 + transformX / transformY));
        int spriteH = std::abs(int(viewH / transformY * 1.05));
        int spriteW = std::max(3, spriteH / 2);
        int y0 = ViewTop + viewH / 2 - spriteH / 2;
        int y1 = y0 + spriteH;
        int x0 = screenX - spriteW / 2;
        int x1 = screenX + spriteW / 2;

        for (int x = x0; x <= x1; ++x)
        {
            if (x < 0 || x >= Width || transformY >= zbuf[x]) continue;

            for (int y = std::max(y0, ViewTop); y <= std::min(y1, ViewBottom); ++y)
            {
                double nx = (x - screenX) / double(std::max(spriteW, 1));
                double ny = (y - (y0 + spriteH / 2.0)) / double(std::max(spriteH, 1));
                if (nx * nx * 1.7 + ny * ny > 0.9) continue;

                int r = 0, g = 0, b = 0;
                if (o.type == 0) { r = 45; g = 70; b = 185; }
                else if (o.type == 1) { r = 175; g = 155; b = 75; }
                else { r = 80; g = 155; b = 65; }

                double edge = std::max(0.35, 1.0 - std::abs(nx) * 0.4 - std::abs(ny) * 0.25);
                PutPixel(rgb, x, y, int(r * edge), int(g * edge), int(b * edge));
            }
        }
    }
}

void PreviewRenderer::DrawHud(double timeSeconds, std::vector<std::uint8_t>& rgb) const
{
    DrawRect(rgb, 0, 0, Width - 1, 5, 32, 32, 32);
    DrawRect(rgb, 0, ViewBottom + 1, Width - 1, Height - 1, 5, 5, 5);
    DrawRect(rgb, 0, ViewBottom + 1, Width - 1, ViewBottom + 2, 150, 150, 150);

    for (int y = 8; y < ViewBottom; ++y)
    {
        int w = 4 + int(2.0 * std::sin(y * 0.18));
        DrawRect(rgb, 0, y, w, y, 92, 92, 88);
        DrawRect(rgb, Width - 1 - w, y, Width - 1, y, 92, 92, 88);
    }

    DrawRect(rgb, 4, 163, 43, 196, 16, 16, 16);
    DrawRect(rgb, 48, 163, 111, 196, 12, 12, 12);
    DrawRect(rgb, 116, 163, 159, 196, 12, 12, 12);
    DrawRect(rgb, 164, 163, 205, 196, 12, 12, 12);
    DrawRect(rgb, 210, 163, 252, 196, 12, 12, 12);
    DrawRect(rgb, 257, 163, 315, 196, 10, 10, 10);

    for (int x : {4, 43, 48, 111, 116, 159, 164, 205, 210, 252, 257, 315})
        DrawRect(rgb, x, 163, x, 196, 170, 170, 170);

    DrawRect(rgb, 4, 163, 315, 164, 170, 170, 170);
    DrawRect(rgb, 4, 195, 315, 196, 170, 170, 170);

    int health = 88 + int(8 * std::sin(timeSeconds * 0.09));
    DrawRect(rgb, 53, 168, 105, 173, 40, 40, 40);
    DrawRect(rgb, 53, 168, 53 + health / 2, 173, 165, 55, 35);
    DrawRect(rgb, 53, 178, 105, 183, 40, 40, 40);
    DrawRect(rgb, 53, 178, 91, 183, 55, 90, 180);

    DrawRect(rgb, 264, 170, 308, 171, 25, 80, 25);
    DrawRect(rgb, 264, 170, 265, 189, 25, 80, 25);
    DrawRect(rgb, 265, 188, 297, 189, 25, 80, 25);
    DrawRect(rgb, 296, 177, 297, 189, 25, 80, 25);
    DrawRect(rgb, 283, 177, 297, 178, 25, 80, 25);

    DrawRect(rgb, 180, 171, 190, 190, 45, 55, 110);
    DrawRect(rgb, 177, 177, 193, 185, 60, 80, 160);
}

void PreviewRenderer::Render(double timeSeconds, std::vector<std::uint8_t>& rgb) const
{
    rgb.assign(std::size_t(Width) * Height * 3, 0);
    DrawWorld(CameraAt(timeSeconds), rgb);
    DrawHud(timeSeconds, rgb);
}
}
