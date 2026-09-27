#include "n3d_preview_core.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
    int fps = argc > 1 ? std::atoi(argv[1]) : 30;
    int seconds = argc > 2 ? std::atoi(argv[2]) : 120;
    if (fps <= 0 || seconds <= 0) return 2;

    n3dpreview::PreviewRenderer renderer;
    std::vector<std::uint8_t> frame;
    for (int i = 0; i < fps * seconds; ++i)
    {
        renderer.Render(double(i) / fps, frame);
        if (std::fwrite(frame.data(), 1, frame.size(), stdout) != frame.size()) return 3;
    }
    return 0;
}
