# Nitemare 3D renderer preview harness

This is a standalone preview harness for the experimental Nitemare 3D renderer branch.

It renders raw 320x200 RGB frames to stdout. The current scene uses synthetic geometry and procedural textures so renderer work can be tested before the original MAP/IMG data loaders are connected.

Current prototype exercises:

- column/DDA ray traversal
- perspective wall-height projection
- 64x64 texture-column stepping
- side and distance shading
- sprite projection with wall occlusion
- flat ceiling/floor background
- a simple N3D-style HUD test area
- a deterministic two-minute camera route

Example:

```sh
g++ -std=c++17 -O2 main.cpp n3d_preview_core.cpp -o n3d_preview
./n3d_preview 30 120 | ffmpeg -f rawvideo -pix_fmt rgb24 -s 320x200 -r 30 -i - out.mp4
```

This is not yet a 1:1 E1M1 renderer. The next major step is replacing the synthetic map/materials with the original Nitemare 3D MAP/IMG/WALLS/OBJECTS data and the reverse-engineered projection constants.

## Original IMG.1 and GAME.PAL support

The preview tooling now has clean-room loaders for original Nitemare 3D wall/object images and palette carriers.

Verified reference geometry/constants currently used by the reconstruction:

- framebuffer: 320x200 indexed color
- 3-D viewport: x 8..311, y 4..155 (304x152)
- horizontal projection factor: 178 pixels
- full-wall vertical projection scale: 155.859375 pixels at one tile
- default ceiling palette index: 0x11
- default floor palette index: 0x0C
- IMG wall directory: 256 little-endian dwords at 0x0000
- IMG object directory: 256 little-endian dwords at 0x0400
- frame payload: width, height, 8 metadata bytes, then width*height indexed pixels stored x-major/column-major
- Win16 sprite transparent index: 0x29

No proprietary Nitemare 3D data files are committed to this repository. The loaders expect legally obtained external MAP/IMG/WALLS/OBJECTS/GAME.PAL assets at runtime.
