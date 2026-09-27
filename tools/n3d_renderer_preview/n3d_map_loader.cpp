#include "n3d_map_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace n3dpreview
{
namespace
{
std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open " + path.string());

    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    if (size < 0) throw std::runtime_error("Cannot determine size of " + path.string());

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (!data.empty())
        input.read(reinterpret_cast<char*>(data.data()), size);
    if (!input && !data.empty())
        throw std::runtime_error("Cannot read " + path.string());
    return data;
}

std::uint16_t ReadU16LE(const std::vector<std::uint8_t>& data, std::size_t offset)
{
    if (offset + 2 > data.size()) throw std::runtime_error("Truncated 16-bit value");
    return static_cast<std::uint16_t>(data[offset]) |
           static_cast<std::uint16_t>(data[offset + 1] << 8u);
}
}

const MapCell& MapLevel::At(std::size_t x, std::size_t y) const
{
    if (x >= Width || y >= Height)
        throw std::out_of_range("Nitemare 3D MAP coordinate outside 64x64 grid");
    return cells[y * Width + x];
}

MapArchive MapArchive::Load(const std::filesystem::path& path)
{
    const auto bytes = ReadBytes(path);
    if (bytes.size() < HeaderSize)
        throw std::runtime_error("MAP file is smaller than the 514-byte header");

    const auto payload = bytes.size() - HeaderSize;
    if (payload % LevelBytes != 0)
        throw std::runtime_error("MAP payload is not an integer number of 8192-byte levels");

    MapArchive archive;
    archive.declaredLevelCount_ = ReadU16LE(bytes, 0);
    const auto physicalLevels = payload / LevelBytes;
    if (archive.declaredLevelCount_ != physicalLevels)
        throw std::runtime_error("MAP level count does not match file size");

    archive.levels_.resize(physicalLevels);
    for (std::size_t level = 0; level < physicalLevels; ++level)
    {
        const auto base = HeaderSize + level * LevelBytes;
        for (std::size_t i = 0; i < MapLevel::Width * MapLevel::Height; ++i)
        {
            archive.levels_[level].cells[i].wall = bytes[base + i * 2];
            archive.levels_[level].cells[i].object = bytes[base + i * 2 + 1];
        }
    }
    return archive;
}

WallClassTable WallClassTable::Load(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open " + path.string());

    WallClassTable table;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream stream(line);
        std::string id, imageRef, resource, wallClass;
        if (!(stream >> id >> imageRef >> resource >> wallClass)) continue;

        try
        {
            const auto value = std::stoul(id, nullptr, 16);
            if (value < table.classes_.size())
                table.classes_[value] = wallClass;
        }
        catch (...) {}
    }
    return table;
}

const std::string& WallClassTable::ClassFor(std::uint8_t wallId) const
{
    return classes_[wallId];
}

bool WallClassTable::IsTraversalCell(std::uint8_t wallId) const
{
    const auto& value = classes_[wallId];
    return value == "FLOOR" || value == "TURN" ||
           value == "DOORVC" || value == "DOORHC" || value == "DOORVI" ||
           value == "WARP_1" || value == "WARP_5" || value == "WARP_L1" ||
           value == "LEVEL_UP";
}
}
