#include "n3d_asset_loader.h"

#include <fstream>
#include <stdexcept>

namespace n3dpreview
{
static std::vector<std::uint8_t> ReadAll(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("cannot open: " + path);

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0);

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

static std::uint16_t ReadU16(const std::vector<std::uint8_t>& bytes, std::size_t pos)
{
    if (pos + 2 > bytes.size())
        throw std::runtime_error("truncated u16");

    return std::uint16_t(bytes[pos] | (std::uint16_t(bytes[pos + 1]) << 8));
}

static std::uint32_t ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t pos)
{
    if (pos + 4 > bytes.size())
        throw std::runtime_error("truncated u32");

    return std::uint32_t(bytes[pos])
        | (std::uint32_t(bytes[pos + 1]) << 8)
        | (std::uint32_t(bytes[pos + 2]) << 16)
        | (std::uint32_t(bytes[pos + 3]) << 24);
}

MapEpisode MapEpisode::Load(const std::string& path)
{
    auto bytes = ReadAll(path);

    constexpr std::size_t HeaderSize = 514;
    constexpr std::size_t LevelStride = 8192;

    if (bytes.size() < HeaderSize || ((bytes.size() - HeaderSize) % LevelStride) != 0)
        throw std::runtime_error("unexpected MAP size");

    MapEpisode episode;
    const std::size_t levelCount = (bytes.size() - HeaderSize) / LevelStride;
    episode.levels.resize(levelCount);

    for (std::size_t level = 0; level < levelCount; ++level)
    {
        auto& out = episode.levels[level];
        const std::size_t base = HeaderSize + level * LevelStride;

        for (std::size_t cell = 0; cell < 4096; ++cell)
        {
            out.walls[cell] = bytes[base + cell * 2];
            out.objects[cell] = bytes[base + cell * 2 + 1];

            if (out.objects[cell] >= 1 && out.objects[cell] <= 4)
            {
                out.startX = int(cell % 64);
                out.startY = int(cell / 64);
                out.startObject = out.objects[cell];
            }
        }
    }

    return episode;
}

ImgArchive ImgArchive::Load(const std::string& path)
{
    auto bytes = ReadAll(path);

    if (bytes.size() < 8)
        throw std::runtime_error("IMG too small");

    // IMG.1 begins its offset table at byte 4. The first non-header image
    // begins at the first stored offset. Images themselves can then be read
    // sequentially because each has width/height followed by eight metadata
    // bytes and width*height indexed pixels.
    std::size_t pos = ReadU32(bytes, 4);
    if (pos >= bytes.size())
        throw std::runtime_error("bad IMG first offset");

    ImgArchive archive;

    while (pos < bytes.size())
    {
        if (pos + 10 > bytes.size())
            throw std::runtime_error("truncated IMG header");

        ImgEntry entry;
        entry.width = bytes[pos++];
        entry.height = bytes[pos++];

        if (entry.width == 0 || entry.height == 0)
            throw std::runtime_error("zero IMG dimensions");

        for (auto& value : entry.metadata)
            value = bytes[pos++];

        const std::size_t pixelCount = std::size_t(entry.width) * entry.height;
        if (pos + pixelCount > bytes.size())
            throw std::runtime_error("truncated IMG pixels");

        entry.pixels.assign(bytes.begin() + pos, bytes.begin() + pos + pixelCount);
        pos += pixelCount;

        archive.entries.push_back(std::move(entry));
    }

    return archive;
}

Palette Palette::LoadFromUif(const std::string& path)
{
    auto bytes = ReadAll(path);

    std::size_t pos = 0;
    Palette result;
    bool found = false;

    // UIF.DAT records use a 16-bit length followed by a 32-bit offset.
    // The 320x200 PCX entries contain a standard 0x0c palette marker and
    // a 768-byte RGB palette trailer. This gives the indexed colours needed
    // for previewing IMG.* assets even when GAME.PAL is unavailable.
    while (pos + 6 <= bytes.size())
    {
        const auto length = ReadU16(bytes, pos);
        const auto offset = ReadU32(bytes, pos + 2);
        pos += 6;

        if (std::size_t(offset) + length > bytes.size())
            throw std::runtime_error("bad UIF record");

        if (length >= 769
            && bytes[offset] == 0x0a
            && bytes[std::size_t(offset) + length - 769] == 0x0c)
        {
            const std::size_t paletteStart = std::size_t(offset) + length - 768;

            for (std::size_t i = 0; i < result.rgb.size(); ++i)
                result.rgb[i] = bytes[paletteStart + i];

            found = true;
            break;
        }

        if (std::size_t(offset) + length == bytes.size())
            break;
    }

    if (!found)
        throw std::runtime_error("no PCX palette in UIF.DAT");

    return result;
}
