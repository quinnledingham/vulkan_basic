struct Vertex_Info {

};

struct Render {
	u32 clear_flags;
	Vector4 clear_color;
	float32 clear_depth;
	float32 clear_stencil;
};

#if OPENGL

#define GPU_EXT(n) opengl_##n

#elif VULKAN

#define GPU_EXT(n) vulkan_##n

#endif // OPENGL / VULKAN

void (*render_clear_color)(Vector4 color) = &GPU_EXT(clear_color);
void (*render_clear_depth_stencil)(float32 depth_value, float32 stencil_value);
void (*render_start_frame)() = &GPU_EXT(start_frame);
void (*render_end_frame)() = &GPU_EXT(end_frame);
void (*render_draw_mesh)(Mesh *mesh) = &GPU_EXT(draw_mesh);
void (*render_init_mesh)(Mesh *mesh) = &GPU_EXT(init_mesh);
void (*render_update_uniform_buffer_object)(Uniform_Buffer_Object ubo, Matrices matrices) = &GPU_EXT(update_uniform_buffer_object);
