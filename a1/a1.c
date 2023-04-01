#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int list(int recursive, long int min_size, int has_perm_write, const char* dirPath){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat fileMetadata;
    off_t size_path = 512;
    char *fullPath = (char *)malloc(size_path * sizeof(char));
    if(fullPath == NULL) {
        fprintf(stderr, "ERROR\nCould not allocate required memory\n");
        return -1;
    }

    dir = opendir(dirPath);
    if(dir == NULL) {
        fprintf(stderr, "ERROR\nThis directory can't be opened\n");
        return -1;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
            if(strlen(fullPath) + strlen(entry->d_name) >= size_path) {
                size_path *= 2;
                fullPath = (char *)realloc(fullPath, size_path * sizeof(char));
                printf("aici");
            }
            snprintf(fullPath, size_path, "%s/%s", dirPath, entry->d_name);
            if(!lstat(fullPath, &fileMetadata)) {
                if(min_size != -1) {
                    if(fileMetadata.st_size > min_size) {
                        printf("%s\n", fullPath);
                    }
                }
                else if(has_perm_write){
                    if(fileMetadata.st_mode & S_IWUSR) {
                        printf("%s\n", fullPath);
                    }
                }
                else {
                    printf("%s\n", fullPath);
                }
                
                if(recursive) {
                    if(S_ISDIR(fileMetadata.st_mode)) {
                        list(recursive, min_size, has_perm_write, fullPath);
                    }
                }
                
                
            }
        }
    }

    closedir(dir);
    return 0;
}

int solveListParameters(int size, char **argv){
    int cnt = 2;
    int recursive = 0;
    long int min_size = -1;
    int has_perm_write = 0;
    char* path = NULL;
    struct stat fileMetadata;

    while(cnt < size){
        if(!strcmp(argv[cnt], "recursive")) {
            recursive = 1;
        }
        if(!strcmp(argv[cnt], "has_perm_write")) {
            has_perm_write = 1;
        }
        if(!strncmp(argv[cnt], "size_greater=", 13)) {
            min_size = atoi(argv[cnt] + 13);
        }
        if(!strncmp(argv[cnt], "path=", 5)) {
            path = argv[cnt] + 5;
        }
        cnt++;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISDIR(fileMetadata.st_mode)){
        fprintf(stderr, "ERROR\nInvalid directory path\n");
        return -1;
    }
    //printf(" Recursive = %d\n Min_Size = %d\n Has_perm_write = %d\n Path = %s\n", recursive, min_size, has_perm_write, path);

    printf("SUCCESS\n");
    list(recursive, min_size, has_perm_write, path);

    return 0;
}

int main(int argc, char **argv){

    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("11416\n");
        }
        if(strcmp(argv[1], "list") == 0) {
            solveListParameters(argc, argv);
        }
    }
    return 0;
}