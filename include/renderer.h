#ifndef RENDERER_H
#define RENDERER_H

#include "data_types.h"

void renderer_draw_3d(app_state_t *app_state);
void renderer_draw_frame(app_state_t *app_state);

// Draw mesh instanced using pre-uploaded static VBO (avoids per-frame VBO creation)
void renderer_draw_mesh_instanced_static(Mesh mesh, Material material, u32 instance_vbo, i32 instances);

#endif // RENDERER_H
