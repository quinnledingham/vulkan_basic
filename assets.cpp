//
// File
//

internal File
load_file(const char *filepath) {
    File result = {};
    
    FILE *in = fopen(filepath, "rb");
    if(in) {
        fseek(in, 0, SEEK_END);
        result.size = ftell(in);
        fseek(in, 0, SEEK_SET);
        
        result.memory = platform_malloc(result.size);
        fread(result.memory, result.size, 1, in);
        fclose(in);
    } else { 
    	logprint("load_file", "Cannot open file %s\n", filepath);
    }
    
    result.filepath = filepath;

    return result;
}

// returns the file with a 0 at the end of the memory.
// useful if you want to read the file like a string immediately.
internal File
load_file_terminated(const char *filename) {
    File result = {};
    File file = load_file(filename);

    //result = file;
    result.size = file.size + 1;
    result.filepath = filename;
    result.memory = platform_malloc(result.size);
    platform_memory_copy(result.memory, file.memory, file.size);

    char *r = (char*)result.memory;
    r[file.size] = 0; // last byte in result.memory
    
    return result;
}

//
// Bitmap
//

internal Bitmap
load_bitmap(const char *filename, bool8 flip_on_load) {
    if (flip_on_load) stbi_set_flip_vertically_on_load(true);
    else              stbi_set_flip_vertically_on_load(false);
    Bitmap bitmap = {};
    bitmap.channels = 4;
    // 4 arg always get filled in with the original amount of channels the image had.
    // Currently forcing it to have 4 channels.
    bitmap.memory = stbi_load(filename, &bitmap.width, &bitmap.height, 0, bitmap.channels);
    
    if (bitmap.memory == 0) logprint("load_bitmap()" "could not load bitmap %s\n", filename);
    bitmap.pitch = bitmap.width * bitmap.channels;
    return bitmap;
}

internal Bitmap
load_bitmap(const char *filename) {
    return load_bitmap(filename, true);
}

internal void
free_bitmap(Bitmap bitmap) {
    stbi_image_free(bitmap.memory);
}

#ifdef OPENGL

#elif VULKAN

#endif // OPENGL / VULKAN

//
// Mesh
//

#ifdef OPENGL

struct OpenGL_Mesh {
    u32 vao;
    u32 vbo;
    u32 ebo;
};

void init_gpu_mesh(Mesh *mesh) {
    OpenGL_Mesh *gl_mesh = (OpenGL_Mesh*)platform_malloc(sizeof(OpenGL_Mesh));

    glGenVertexArrays(1, &gl_mesh->vao);
    glGenBuffers(1, &gl_mesh->vbo);
    glGenBuffers(1, &gl_mesh->ebo);
    
    glBindVertexArray(gl_mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices_count * sizeof(Vertex), &mesh->vertices[0], GL_STATIC_DRAW);  
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_count * sizeof(u32), &mesh->indices[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0); // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); // vertex color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2); // vertex texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    
    glBindVertexArray(0);

    mesh->gpu_info = (void*)gl_mesh;
}

void draw_mesh(Mesh *mesh) {
    OpenGL_Mesh *gl_mesh = (OpenGL_Mesh*)mesh->gpu_info;
    glBindVertexArray(gl_mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indices_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void free_mesh(Mesh *mesh) {
    platform_free(mesh->vertices);
    platform_free(mesh->indices);
    platform_free(mesh->gpu_info);
}

#elif VULKAN

struct Vulkan_Mesh {
    //VkBuffer buffer;
    //VkDeviceMemory memory;
    //void *mapped_memory;

    u32 vertices_offset;
    u32 indices_offset;
    u32 uniforms_offsets[vulkan_info.MAX_FRAMES_IN_FLIGHT];
    u32 uniform_size; // size of the individual uniforms
};

void init_gpu_mesh(Mesh *mesh) {
/*
    Vulkan_Mesh *vulkan_mesh = (Vulkan_Mesh*)platform_malloc(sizeof(Vulkan_Mesh));

    u32 vertices_size = sizeof(mesh->vertices[0]) * sizeof(Vertex);
    u32 indices_size = sizeof(mesh->indices[0]) * sizeof(u32);

    u32 buffer_size = vertices_size + indices_size;
    void *memory = platform_malloc(buffer_size);

    memcpy(memory, (void*)mesh->vertices, vertices_size);
    memcpy((char*)memory + vertices_size, (void*)mesh->indices, indices_size);

    vulkan_mesh->

    platform_free(memory);
*/
}

void draw_mesh(Mesh *mesh) {
    
}


#endif // OPENGL / VULKAN

//
// Shader
//

#ifdef OPENGL

// loads the files
void load_shader(Shader *shader)
{
    if (shader->files[0].filepath == 0) {
        print("load_opengl_shader() must have a vertex shader\n");
        return;
    }

    printf("loaded shader: ");
    for (u32 i = 0; i < SHADER_TYPE_AMOUNT; i++) {
        if (shader->files[i].memory != 0) platform_free(&shader->files[i].memory);
        shader->files[i].memory = 0;

        if (shader->files[i].filepath != 0) {
            shader->files[i] = load_file_terminated(shader->files[i].filepath);
            printf("%s ", shader->files[i].filepath);
        }
    }
    printf("\n");
}

bool compile_shader(u32 handle, const char *file, int type)
{
    u32 shader =  glCreateShader((GLenum)type);
    glShaderSource(shader, 1, &file, NULL);
    glCompileShader(shader);
    
    GLint compiled_shader = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_shader);  
    if (!compiled_shader) {
        opengl_debug(GL_SHADER, shader);
    } else {
        glAttachShader(handle, shader);
    }
    
    glDeleteShader(shader);
    
    return compiled_shader;
}

// lines up with enum shader_types
const u32 file_types[5] = { 
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER,
};

// compiles the files
void compile_shader(Shader *shader)
{
    //shader->uniform_buffer_objects_generated = false;
    shader->compiled = false;
    if (shader->handle != 0) glDeleteProgram(shader->handle);
    shader->handle = glCreateProgram();
    
    if (shader->files[0].memory == 0) {
        print("vertex shader required\n");
        return;
    }

    for (u32 i = 0; i < SHADER_TYPE_AMOUNT; i++) {
        if (shader->files[i].memory == 0) continue; // file was not loaded

        if (!compile_shader(shader->handle, (char*)shader->files[i].memory, file_types[i])) {
            print("compile_shader() could not compile %s\n", shader->files[i].filepath); 
            return;
        }
    }

    // Link
    glLinkProgram(shader->handle);

    GLint linked_program = 0;
    glGetProgramiv(shader->handle, GL_LINK_STATUS, &linked_program);
    if (!linked_program) {
        opengl_debug(GL_PROGRAM, shader->handle);
        print("compile_shader() link failed\n");
        return;
    }

    shader->compiled = true;
}

u32 use_shader(Shader *shader) {
    glUseProgram(shader->handle);
    return shader->handle;
}

#endif // OPENGL / VULKAN