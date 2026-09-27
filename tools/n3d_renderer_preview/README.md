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
