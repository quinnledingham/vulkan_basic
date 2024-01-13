// block index is from glUniformBlockBinding or binding == #
internal Uniform_Buffer_Object
opengl_init_uniform_buffer_object(u32 size, u32 binding)
{
    Uniform_Buffer_Object ubo = {};
    ubo.size = size;
    ubo.handle = platform_malloc(sizeof(u32));
    glGenBuffers(1, (u32*)ubo.handle);
    
    // clearing buffer
    glBindBuffer(GL_UNIFORM_BUFFER, *(u32*)ubo.handle);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, *(u32*)ubo.handle);
    
    return ubo;
}

// returns the new offset
u32 opengl_buffer_sub_data(u32 target, u32 offset, u32 size, void *data) {
    glBufferSubData(target, offset, size, data);
    return (offset + size);
}

#define BUFFER_SUB_DATA(target, offset, n) opengl_buffer_sub_data(target, offset, sizeof(n), (void *)&n)

internal void
opengl_update_uniform_buffer_object(Uniform_Buffer_Object ubo, Matrices matrices) {
    GLenum target = GL_UNIFORM_BUFFER;
    u32 offset = 0;

    glBindBuffer(target, *(u32*)ubo.handle);
    offset = BUFFER_SUB_DATA(target, offset, matrices.model);
    offset = BUFFER_SUB_DATA(target, offset, matrices.view);
    offset = BUFFER_SUB_DATA(target, offset, matrices.projection);
    glBindBuffer(target, 0);
}

//
// Render
//

struct OpenGL_Info {
    
}

void render_clear_color(Vector4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void render_start_frame() {
    u32 gl_clear_flags = 
            GL_COLOR_BUFFER_BIT  | 
            GL_DEPTH_BUFFER_BIT  | 
            GL_STENCIL_BUFFER_BIT;
    
    glClear(gl_clear_flags);
}

void render_end_frame() {
#ifdef SDL
    SDL_GL_SwapWindow(sdl_window); 
#endif // SDL
}