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

enum Texture_Parameters
{
    TEXTURE_PARAMETERS_DEFAULT,
    TEXTURE_PARAMETERS_CHAR,
};

internal void
opengl_init_bitmap_handle(Bitmap *bitmap, u32 texture_parameters)
{
    GLenum target = GL_TEXTURE_2D;
    
    glGenTextures(1, &bitmap->handle);
    glBindTexture(target, bitmap->handle);
    
    GLint internal_format = 0;
    GLenum data_format = 0;
    GLint pixel_unpack_alignment = 0;
    
    switch(bitmap->channels) {
        case 1: {
            internal_format = GL_RED,
            data_format = GL_RED,
            pixel_unpack_alignment = 1; 
        } break;

        case 3: {
            internal_format = GL_RGB;
            data_format = GL_RGB;
            pixel_unpack_alignment = 1; // because RGB is weird case unpack alignment can't be 3
        } break;
        
        case 4: {
            internal_format = GL_RGBA;
            data_format = GL_RGBA;
            pixel_unpack_alignment = 4;
        } break;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_unpack_alignment);
    glTexImage2D(target, 0, internal_format, bitmap->dim.width, bitmap->dim.height, 0, data_format, GL_UNSIGNED_BYTE, bitmap->memory);
    glGenerateMipmap(target);
    
    switch(texture_parameters) {
        case TEXTURE_PARAMETERS_DEFAULT:
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;

        case TEXTURE_PARAMETERS_CHAR:
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }
    
    glBindTexture(target, 0);
}
