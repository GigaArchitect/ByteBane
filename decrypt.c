#include <glib-2.0/glib.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>

void remove_padding(char * block) {
    // The last byte in the block contains the number of padding bytes
    int padding = block[15];  // PKCS5 padding is the value of the last byte
    
    if (padding > 0 && padding <= 16) {
        // Null-terminate the string at the correct position
        block[16 - padding] = '\0';
    }
}

int decrypt_file(char * file_path, AES_KEY key){
    if (!file_path){
        perror("File Path Is Not Correct !");
        return 1;
    }

    GList * linked_list = NULL;
    FILE * buffer;
    char * buffer_line;

    buffer_line = malloc(sizeof(char)*16);
    buffer = fopen(file_path, "rb");
    if (!buffer){
        perror("Unable To Read File !");
        return 1;
    }

    while(fread(buffer_line, 1, 16, buffer) == 16){
        char * read_line = strdup(buffer_line);
        linked_list = g_list_append(linked_list, read_line);
    }

    for (GList * l = linked_list; l != NULL; l = l->next){
        char * block = strdup((char *) l->data);
        AES_decrypt(block, block, &key);

        remove_padding(block);

        free(l->data);
        l->data = block;
    }
    fclose(buffer);

    const char * file_extension = ".dec";
    char * new_file_path = malloc(strlen(file_path) + strlen(file_extension) + 1);
    if (new_file_path) {
        strcpy(new_file_path, file_path);
        strcat(new_file_path, file_extension);
    }

    buffer = fopen(new_file_path, "w");
    for(GList * l = linked_list; l != NULL; l = l->next){
        fputs(l->data, buffer);
    }
    fclose(buffer);

    g_list_free(linked_list);

    return 0;
}

int has_extension(const char *filename, const char *extension) {
    // Find the last occurrence of the dot in the filename
    const char *dot = strrchr(filename, '.');
    
    // If there's no dot or the dot is at the start of the filename, return 0 (no extension)
    if (!dot || dot == filename) {
        return 0;
    }
    
    // Compare the file extension with the provided extension
    return strcmp(dot + 1, extension) == 0;
}

int main() {
    char * key = getenv("BYTE_BANE_KEY");
    AES_KEY aes_key;
    AES_set_decrypt_key(key, 128, &aes_key);

    DIR * current_dir;
    struct dirent * dir;
    
    current_dir = opendir(".");
    if (current_dir){
        while ((dir = readdir(current_dir)) != NULL){
            if (has_extension(dir->d_name, "enc")){
                decrypt_file(dir->d_name, aes_key);
            }
        }
    }

    return 0;
}
