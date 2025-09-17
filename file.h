#pragma once

typedef struct File_st      File_t;


struct File_st {
    char*       data;
    size_t      size;
    const char* file_path;
};

File_t      read_file(const char* filepath);

#ifdef FILE_IMPLEMENTATION
File_t read_file(const char* filepath) {
    char* err_msg = NULL;
    FILE* fh      = fopen(filepath, "rb");

    if (!fh) goto error;

    size_t file_size = 0;
    if (fseek(fh, 0, SEEK_END)) goto error;

    file_size = ftell(fh);
    if (fseek(fh, 0, SEEK_SET)) goto error;
     
    char* data = (char*) malloc(file_size + 1);
    if (!data) {
        err_msg = "Couldn't allocate bytes to read file in RAM.";
        goto error;
    }

    if (fread(data, 1, file_size, fh) != file_size)
        goto error;

    data[file_size] = '\0';
    fclose(fh);

    return (File_t) {
        .data      = data,
        .size      = file_size + 1,
        .file_path = filepath
    };

error:
    char* error = strerror(errno);
    if (err_msg) error = err_msg;

    printf("read_file() Failed: %s\n", err_msg);

    if (fh) { fclose(fh); }
    return (File_t){ .data = NULL, .size = 0, .file_path = filepath };
}
#endif // FILE_IMPLEMENTATION