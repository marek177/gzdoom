#include "n3d_renderer.h"

// Factory kept separate from GZDoom's renderer selection for now.
// A later commit will add an opt-in renderer mode once the framebuffer
// path is functional.
FRenderer *CreateNitemareRenderer()
{
	return new FNitemareRenderer;
}

void FNitemareRenderer::Init()
{
	// Phase 1 foundation only.
	// N3D palette/shading tables and projection state will be initialized here.
}

void FNitemareRenderer::Precache(uint8_t *texhitlist, TMap<PClassActor*, bool> &actorhitlist)
{
	// TODO: Precache N3D wall textures and sprite/object images.
}

void FNitemareRenderer::RenderView(player_t *player, DCanvas *target, void *videobuffer, int bufferpitch)
{
	// TODO: Implement the original Nitemare 3D rendering pipeline:
	//  1. original viewpoint/FOV setup
	//  2. fixed-point world/ray traversal
	//  3. wall-column projection and texture stepping
	//  4. clipping/depth rules
	//  5. sprite/object projection
	//  6. palette/shading output to the framebuffer
}

void FNitemareRenderer::WriteSavePic(player_t *player, FileWriter *file, int width, int height)
{
	// TODO: Route through the N3D framebuffer once rendering is functional.
}

void FNitemareRenderer::DrawRemainingPlayerSprites()
{
	// N3D weapon/HUD rendering will be added after the world renderer.
}

void FNitemareRenderer::SetColormap(FLevelLocals *level)
{
	// TODO: Map this to the original N3D palette/shading model.
}

void FNitemareRenderer::SetClearColor(int color)
{
	mClearColor = color;
}
