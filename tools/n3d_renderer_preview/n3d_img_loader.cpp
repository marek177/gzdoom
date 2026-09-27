#include "n3d_img_loader.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace n3dpreview
{
namespace
{
std::vector<std::uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open " + path.string());

    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    input.seekg(0, std::ios::beg);
    if (length < 0) throw std::runtime_error("Cannot determine file size: " + path.string());

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    if (!bytes.empty())
        input.read(reinterpret_cast<char*>(bytes.data()), length);
    if (!input && !bytes.empty())
        throw std::runtime_error("Cannot read " + path.string());
    return bytes;
}

std::uint32_t U32LE(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    if (offset + 4 > bytes.size()) throw std::runtime_error("Truncated IMG directory");
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}
}

std::uint8_t IndexedFrame::Pixel(std::size_t x, std::size_t y) const
{
    if (x >= width || y >= height) throw std::out_of_range("IMG pixel outside frame");
    return pixels[x * static_cast<std::size_t>(height) + y];
}

ImgArchive ImgArchive::Load(const std::filesystem::path& path)
{
    const auto bytes = ReadFile(path);
    if (bytes.size() < 0x800)
        throw std::runtime_error("IMG file is too small for both 256-entry directories");

    ImgArchive archive;
    for (std::size_t i = 0; i < 256; ++i)
    {
        archive.wallOffsets_[i] = U32LE(bytes, i * 4);
        archive.objectOffsets_[i] = U32LE(bytes, 0x400 + i * 4);
    }

    std::uint32_t firstData = 0xffffffffu;
    for (const auto offset : archive.wallOffsets_)
        if (offset) firstData = std::min(firstData, offset);
    for (const auto offset : archive.objectOffsets_)
        if (offset) firstData = std::min(firstData, offset);

    if (firstData == 0xffffffffu || firstData >= bytes.size())
        throw std::runtime_error("IMG has no valid image payload");

    std::size_t pos = firstData;
    while (pos < bytes.size())
    {
        if (pos + 10 > bytes.size())
            throw std::runtime_error("Truncated IMG frame header");

        IndexedFrame frame;
        frame.fileOffset = static_cast<std::uint32_t>(pos);
        frame.width = bytes[pos];
        frame.height = bytes[pos + 1];
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 2), 8, frame.metadata.begin());

        const auto pixels = static_cast<std::size_t>(frame.width) * frame.height;
        if (pos + 10 + pixels > bytes.size())
            throw std::runtime_error("IMG frame extends beyond EOF");

        frame.pixels.assign(bytes.begin() + static_cast<std::ptrdiff_t>(pos + 10),
                            bytes.begin() + static_cast<std::ptrdiff_t>(pos + 10 + pixels));

        archive.offsetIndex_.push_back({frame.fileOffset, archive.frames_.size()});
        archive.frames_.push_back(std::move(frame));
        pos += 10 + pixels;
    }

    std::sort(archive.offsetIndex_.begin(), archive.offsetIndex_.end());
    return archive;
}

const IndexedFrame* ImgArchive::FrameAt(std::uint32_t offset) const
{
    if (!offset) return nullptr;
    const auto it = std::lower_bound(offsetIndex_.begin(), offsetIndex_.end(),
                                     std::pair<std::uint32_t, std::size_t>{offset, 0},
                                     [](const auto& a, const auto& b) { return a.first < b.first; });
    if (it == offsetIndex_.end() || it->first != offset) return nullptr;
    return &frames_[it->second];
}

const IndexedFrame* ImgArchive::WallFrame(std::uint8_t wallId) const
{
    return FrameAt(wallOffsets_[wallId]);
}

const IndexedFrame* ImgArchive::ObjectFrame(std::uint8_t objectId) const
{
    return FrameAt(objectOffsets_[objectId]);
}

GamePalette GamePalette::Load(const std::filesystem::path& path)
{
    const auto bytes = ReadFile(path);

    std::size_t offset = 0;
    if (bytes.size() == 5459) offset = 4691;      // Win16 PCX palette carrier
    else if (bytes.size() == 1924) offset = 1156; // DOS palette carrier
    else if (bytes.size() == 768) offset = 0;     // raw RGB table
    else throw std::runtime_error("Unsupported GAME.PAL layout");

    if (offset + 768 > bytes.size())
        throw std::runtime_error("Truncated GAME.PAL RGB table");

    GamePalette palette;
    for (std::size_t i = 0; i < 256; ++i)
    {
        palette.colors_[i][0] = bytes[offset + i * 3 + 0];
        palette.colors_[i][1] = bytes[offset + i * 3 + 1];
        palette.colors_[i][2] = bytes[offset + i * 3 + 2];
    }
    return palette;
}
