#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace n3dpreview
{
struct MapCell
{
    std::uint8_t wall = 0;
    std::uint8_t object = 0;
};

struct MapLevel
{
    static constexpr std::size_t Width = 64;
    static constexpr std::size_t Height = 64;
    std::array<MapCell, Width * Height> cells{};

    const MapCell& At(std::size_t x, std::size_t y) const;
};

class MapArchive
{
public:
    static constexpr std::size_t HeaderSize = 514;
    static constexpr std::size_t LevelBytes = 8192;

    static MapArchive Load(const std::filesystem::path& path);

    std::uint16_t DeclaredLevelCount() const { return declaredLevelCount_; }
    const std::vector<MapLevel>& Levels() const { return levels_; }

private:
    std::uint16_t declaredLevelCount_ = 0;
    std::vector<MapLevel> levels_;
};

class WallClassTable
{
public:
    static WallClassTable Load(const std::filesystem::path& path);

    const std::string& ClassFor(std::uint8_t wallId) const;
    bool IsTraversalCell(std::uint8_t wallId) const;

private:
    std::array<std::string, 256> classes_{};
};
}
