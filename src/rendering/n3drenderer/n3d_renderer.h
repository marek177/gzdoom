#pragma once

#include "swrenderer/r_renderer.h"

// Experimental Nitemare 3D renderer backend.
//
// This class intentionally starts as an isolated renderer implementation.
// It does not alter the existing GZDoom software or hardware renderers.
// Original Nitemare 3D projection, wall traversal, palette/shading and
// sprite clipping will be ported here incrementally from reverse-engineered
// behaviour.
struct FNitemareRenderer final : public FRenderer
{
	FNitemareRenderer() = default;
	~FNitemareRenderer() override = default;

	void Precache(uint8_t *texhitlist, TMap<PClassActor*, bool> &actorhitlist) override;
	void RenderView(player_t *player, DCanvas *target, void *videobuffer, int bufferpitch) override;
	void WriteSavePic(player_t *player, FileWriter *file, int width, int height) override;
	void DrawRemainingPlayerSprites() override;
	void SetColormap(FLevelLocals *level) override;
	void SetClearColor(int color) override;
	void Init() override;

private:
	int mClearColor = 0;
};

FRenderer *CreateNitemareRenderer();
