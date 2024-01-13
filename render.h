struct Render {
	u32 clear_flags;
	Vector4 clear_color;
	float32 clear_depth;
	float32 clear_stencil;
};

void render_clear_color(Vector4 color);
void render_clear_depth_stencil(float32 depth_value, float32 stencil_value);
void render_start_frame();
void render_end_frame();