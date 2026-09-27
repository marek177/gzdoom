#pragma once

#include <cstdint>
#include <vector>

namespace n3dpreview
{
struct Camera
{
    double x = 6.0;
    double y = 6.0;
    double angle = 0.0;
};

class PreviewRenderer
{
public:
    static constexpr int Width = 320;
    static constexpr int Height = 200;
    static constexpr int ViewTop = 6;
    static constexpr int ViewBottom = 156;

    PreviewRenderer();
    void Render(double timeSeconds, std::vector<std::uint8_t>& rgb) const;

private:
    static constexpr int MapW = 32;
    static constexpr int MapH = 32;
    int map_[MapH][MapW]{};

    void BuildMap();
    Camera CameraAt(double t) const;
    void DrawWorld(const Camera& camera, std::vector<std::uint8_t>& rgb) const;
    void DrawHud(double timeSeconds, std::vector<std::uint8_t>& rgb) const;
    void PutPixel(std::vector<std::uint8_t>& rgb, int x, int y, int r, int g, int b) const;
    void DrawRect(std::vector<std::uint8_t>& rgb, int x0, int y0, int x1, int y1, int r, int g, int b) const;
    void TextureSample(int material, int tx, int ty, int& r, int& g, int& b) const;
};
}
