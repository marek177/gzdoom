#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace n3dpreview
{
struct IndexedFrame
{
    std::uint32_t fileOffset = 0;
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    std::array<std::uint8_t, 8> metadata{};
    std::vector<std::uint8_t> pixels; // original N3D x-major / column-major layout

    bool Valid() const noexcept { return width != 0 && height != 0 && !pixels.empty(); }
    std::uint8_t Pixel(std::size_t x, std::size_t y) const;
};

class ImgArchive
{
public:
    static ImgArchive Load(const std::filesystem::path& path);

    const IndexedFrame* WallFrame(std::uint8_t wallId) const;
    const IndexedFrame* ObjectFrame(std::uint8_t objectId) const;

private:
    std::array<std::uint32_t, 256> wallOffsets_{};
    std::array<std::uint32_t, 256> objectOffsets_{};
    std::vector<IndexedFrame> frames_;
    std::vector<std::pair<std::uint32_t, std::size_t>> offsetIndex_;

    const IndexedFrame* FrameAt(std::uint32_t offset) const;
};

class GamePalette
{
public:
    static GamePalette Load(const std::filesystem::path& path);

    const std::array<std::uint8_t, 3>& RGB(std::uint8_t index) const
    {
        return colors_[index];
    }

private:
    std::array<std::array<std::uint8_t, 3>, 256> colors_{};
};

struct ReferenceProjection
{
    static constexpr int FramebufferWidth = 320;
    static constexpr int FramebufferHeight = 200;

    static constexpr int ViewportLeft = 8;
    static constexpr int ViewportRight = 311;
    static constexpr int ViewportTop = 4;
    static constexpr int ViewportBottom = 155;
    static constexpr int ViewportWidth = 304;
    static constexpr int ViewportHeight = 152;

    static constexpr double HorizontalProjection = 178.0;
    static constexpr double VerticalWallScale = 155.859375;

    static constexpr std::uint8_t CeilingPaletteIndex = 0x11;
    static constexpr std::uint8_t FloorPaletteIndex = 0x0c;
};
}
