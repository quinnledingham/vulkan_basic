// block index is from glUniformBlockBinding or binding == #
Uniform_Buffer_Object opengl_init_uniform_buffer_object(u32 size, u32 binding)
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

void opengl_clear_color(Vector4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void opengl_start_frame() {
    u32 gl_clear_flags = 
            GL_COLOR_BUFFER_BIT  | 
            GL_DEPTH_BUFFER_BIT  | 
            GL_STENCIL_BUFFER_BIT;
    
    glClear(gl_clear_flags);
}

void opengl_end_frame() {
#ifdef SDL
    SDL_GL_SwapWindow(opengl_info.sdl_window); 
#endif // SDL
}

//
// Mesh
//

internal void
opengl_get_vertex_input_info(u32 binding) {

}

void opengl_init_mesh(Mesh *mesh) {
    OpenGL_Mesh *gl_mesh = (OpenGL_Mesh*)platform_malloc(sizeof(OpenGL_Mesh));

    // allocating buffer
    glGenVertexArrays(1, &gl_mesh->vao);
    glGenBuffers(1, &gl_mesh->vbo);
    glGenBuffers(1, &gl_mesh->ebo);
    
    glBindVertexArray(gl_mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_mesh->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_mesh->ebo);

    // defining a vertex
    glEnableVertexAttribArray(0); // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1); // vertex color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2); // vertex texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices_count * sizeof(Vertex), &mesh->vertices[0], GL_STATIC_DRAW);  
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_count * sizeof(u32), &mesh->indices[0], GL_STATIC_DRAW);
    
    glBindVertexArray(0);

    mesh->gpu_info = (void*)gl_mesh;
}

void opengl_draw_mesh(Mesh *mesh) {
    OpenGL_Mesh *gl_mesh = (OpenGL_Mesh*)mesh->gpu_info;
    glBindVertexArray(gl_mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indices_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
