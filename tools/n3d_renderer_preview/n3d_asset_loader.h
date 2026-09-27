#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace n3dpreview
{
struct MapLevel
{
    std::array<std::uint8_t, 4096> walls{};
    std::array<std::uint8_t, 4096> objects{};
    int startX = -1;
    int startY = -1;
    std::uint8_t startObject = 0;
};

struct MapEpisode
{
    std::vector<MapLevel> levels;
    static MapEpisode Load(const std::string& path);
};

struct ImgEntry
{
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    std::array<std::uint8_t, 8> metadata{};
    std::vector<std::uint8_t> pixels; // file order: x-major, then y

    std::uint8_t At(int x, int y) const
    {
        return pixels[std::size_t(x) * height + y];
    }
};

struct ImgArchive
{
    std::vector<ImgEntry> entries;
    static ImgArchive Load(const std::string& path);
};

struct Palette
{
    std::array<std::uint8_t, 768> rgb{};
    static Palette LoadFromUif(const std::string& path);
};
}
