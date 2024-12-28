#include <glib-2.0/glib.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#define MAX_LINE_LEN 512

int encrypt_file(char * file_path, AES_KEY key){
    if (!file_path){
        perror("File Path Is Not Correct !");
        return 1;
    }
    
    GList * linked_list = NULL;
    FILE * buffer;
    char * buffer_line;


    buffer_line = malloc(sizeof(char)*MAX_LINE_LEN);
    buffer = fopen(file_path, "r");
    if (!buffer){
        perror("Uable To Open File !");
        return 1;
    }
 
    while (fgets(buffer_line, MAX_LINE_LEN, buffer)){
        char * copied_line = strdup(buffer_line);
        linked_list = g_list_append(linked_list, copied_line);
    }

    for (GList * l = linked_list; l != NULL; l = l->next){
        char * line = strdup((char *) l->data);
        int len = strlen(line);
        int padding = AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE);
        line = realloc(line, len + padding);
        memset(line + len, padding, padding);  // Fill all padding bytes with padding length
        int blocks = len + padding;
        for(int i = 0; i < blocks; i+=16){
            AES_encrypt(line+i, line+i, &key);
        }

        free(l->data);
        l->data = line;
    }
    fclose(buffer);

    const char* file_extension = ".enc";
    char* new_file_path = malloc(strlen(file_path) + strlen(file_extension) + 1);
    if (new_file_path) {
        strcpy(new_file_path, file_path);
        strcat(new_file_path, file_extension);
    }

    buffer = fopen(new_file_path, "wb");
    for(GList * l = linked_list; l != NULL; l = l->next){
        fwrite(l->data, strlen(l->data), 1, buffer);
    }
    fclose(buffer);

    return 0;
}


int main() {
    char * key = getenv("BYTE_BANE_KEY");
    char * file_path = "command.txt";
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    encrypt_file(file_path, aes_key);
    return 0;
}
