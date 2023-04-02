#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    char name[19];
    int type;
    int offset;
    int size;
} section_t;

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
        free(fullPath);
        return -1;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
            if(strlen(fullPath) + strlen(entry->d_name) >= size_path) {
                size_path *= 2;
                fullPath = (char *)realloc(fullPath, size_path * sizeof(char));
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

    free(fullPath);
    closedir(dir);
    return 0;
}


int typeIsValid(int type){
    const int vec[] = {56, 85, 61, 57, 18, 19, 64};
    for(int i=0; i < sizeof(vec)/sizeof(int); i++) {
        if(vec[i] == type) {
            return 0;
        }
    }
    return 1;
}

section_t* fileIsValid(int fd, int *no_of_sections, int *version) {
    char magic[2];
    int header_size;
   
    //---------------------------------------MAGIC
    if(read(fd, &magic, 2) < 0){
        fprintf(stderr, "ERROR\nCould not read from file\n");
        return NULL;
    } else if (strcmp(magic, "Q0")) {
        fprintf(stderr, "ERROR\nwrong magic\n");
        return NULL;
    }

    //---------------------------------------HEADER_SIZE
    if(read(fd, &header_size, 2) < 0){
        fprintf(stderr, "ERROR\nCould not read from file\n");
        return NULL;
    }

    //----------------------------------------VERSION
    if(read(fd, version, 1) < 0){
        fprintf(stderr, "ERROR\nCould not read from file\n");
        return NULL;
    }
    else if (*version < 73 || *version > 97)
    {
        fprintf(stderr, "ERROR\nwrong version\n");
        return NULL;
    }

    //----------------------------------------NUMBER_OF_SECTIONS
    if(read(fd, no_of_sections, 1) < 0){
        fprintf(stderr, "ERROR\nCould not read from file\n");
        return NULL;
    } else if (*no_of_sections < 3 || *no_of_sections > 14) {
        fprintf(stderr, "ERROR\nwrong sect_nr\n");
        return NULL;
    }

    //---------------------------------------SECTIONS
    section_t *sections = (section_t*)malloc((*no_of_sections)*sizeof(section_t));
    if(sections == NULL) {
        fprintf(stderr, "ERROR\nCould not allocate required memory\n");
        return NULL;
    }

    for(int i=0; i<*no_of_sections; i++) {

        read(fd, &sections[i].name, 19);
        read(fd, &sections[i].type, 4);
        read(fd, &sections[i].offset, 4);
        read(fd, &sections[i].size, 4);
        if(typeIsValid(sections[i].type)){
            fprintf(stderr, "ERROR\nwrong sect_type\n");
            return NULL;
        }
       
    }

    return sections;
}

int parse(char *filePath){

    int fd = -1;
    int version;
    int no_of_sections = 0;
    section_t *sections = NULL;

    fd = open(filePath, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "ERROR\nThis file can't be opened\n");
        return -1;
    }
    sections = fileIsValid(fd, &no_of_sections, &version);
    if(sections == NULL){
       return -1; 
    }

    printf("SUCCESS\n");
	printf("version=%d\n", version);
	printf("nr_sections=%d\n", no_of_sections);
    
    for(int i=0; i<no_of_sections; i++) {

        printf("section%d: %s %d %d\n", i+1, sections[i].name, sections[i].type, sections[i].size);
    }

    free(sections);
    close(fd);
    return 0;
}

//------------------------------------------------------------------------------------------------SOLVE_PARAMETERS_FOR_FUNCTIONS
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

    printf("SUCCESS\n");
    list(recursive, min_size, has_perm_write, path);

    return 0;
}

int solveParseParameters(int size, char **argv){
    const int cnt = 2;
    char* path = NULL;
    struct stat fileMetadata;

    if(!strncmp(argv[cnt], "path=", 5)) {
        path = argv[cnt] + 5;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISREG(fileMetadata.st_mode)){
        fprintf(stderr, "ERROR\nInvalid file path\n");
        return -1;
    }
    //printf("Path = %s\n", path);

    parse(path);

    return 0;
}

//------------------------------------------------------------------------------------------------------MAIN_FUNCTION
int main(int argc, char **argv){

    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("11416\n");
        }
        if(strcmp(argv[1], "list") == 0) {
            solveListParameters(argc, argv);
        }
        if(strcmp(argv[1], "parse") == 0) {
            solveParseParameters(argc, argv);
        }
    }
    return 0;
}