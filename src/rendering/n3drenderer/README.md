# Nitemare 3D renderer

This directory contains the experimental Nitemare 3D compatibility renderer for this GZDoom fork.

## Goal

The long-term goal is to reproduce the original Nitemare 3D rendering behaviour as closely as possible while using GZDoom for the surrounding modern platform layer.

The classic path is intended to preserve:

- original projection and field of view
- integer/fixed-point arithmetic where required for deterministic output
- wall/ray traversal
- wall-column height calculation
- texture X/Y stepping
- clipping and visibility rules
- sprite/object projection and occlusion
- original palette and distance shading
- original framebuffer-style output

A later enhanced path may keep Nitemare 3D world/game semantics while allowing widescreen, high resolutions, interpolation and modern presentation features.

## Initial architecture

```text
GZDoom platform/input/audio
          |
          v
  FNitemareRenderer
          |
   N3D view/projection
          |
   N3D wall traversal
          |
  N3D sprite + clipping
          |
 N3D palette/framebuffer
          |
          v
 GZDoom presentation layer
```

The initial commit only adds the isolated renderer skeleton and build integration. It deliberately does not replace or alter the existing GZDoom software/hardware rendering paths.
