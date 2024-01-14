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

//
// Mesh
//

void free_mesh(Mesh *mesh) {
    platform_free(mesh->vertices);
    platform_free(mesh->indices);
    platform_free(mesh->gpu_info);
}

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
const u32 opengl_shader_file_types[5] = { 
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

        if (!compile_shader(shader->handle, (char*)shader->files[i].memory, opengl_shader_file_types[i])) {
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
