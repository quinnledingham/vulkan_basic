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
    	logprint("load_file", "Cannot open file %s", filepath);
    }
    
    result.filepath = filepath;

    return result;
}