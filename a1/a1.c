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

typedef struct {
    char magic[3];
    int header_size;
    int version;
    int no_of_sections;
    section_t *sections;
}header_t;


//-----------------------------------------------------------------------------------------------------------------------LIST_FUCNTION
int list(int recursive, long int min_size, int has_perm_write, const char* dirPath){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat fileMetadata;
    off_t size_path = 512;
    char *fullPath = (char *)malloc(size_path * sizeof(char));
    if(fullPath == NULL) {
        printf("ERROR\nCould not allocate required memory\n");
        return -1;
    }

    dir = opendir(dirPath);
    if(dir == NULL) {
        printf("ERROR\nThis directory can't be opened\n");
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

//---------------------------------------------------------SOLVE_PARAMETERS_LIST
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
        printf("ERROR\nInvalid directory path\n");
        return -1;
    }

    printf("SUCCESS\n");
    
    return list(recursive, min_size, has_perm_write, path);
}


//------------------------------------------------------------------------------------------------------------------------------ISVALID_FUCNTION
int typeIsValid(int type){
    const int vec[] = {56, 85, 61, 57, 18, 19, 64};
    for(int i=0; i < sizeof(vec)/sizeof(int); i++) {
        if(vec[i] == type) {
            return 0;
        }
    }
    return 1;
}

int fileIsValid(int fd, header_t* header) {
    //---------------------------------------MAGIC
    if(read(fd, header->magic, 2) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    } else if (strcmp(header->magic, "Q0")) {
        printf("ERROR\nwrong magic\n");
        return -1;
    }

    //---------------------------------------HEADER_SIZE
    if(read(fd, &header->header_size, 2) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    }
   
    //----------------------------------------VERSION
    if(read(fd, &header->version, 1) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    }
    else if (header->version < 73 || header->version > 97)
    {
        printf("ERROR\nwrong version\n");
        return -1;
    }

    //----------------------------------------NUMBER_OF_SECTIONS
    if(read(fd, &header->no_of_sections, 1) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    } else if (header->no_of_sections < 3 || header->no_of_sections > 14) {
        printf("ERROR\nwrong sect_nr\n");
        return -1;
    }
    
    //---------------------------------------SECTIONS
    header->sections = (section_t*)malloc(header->no_of_sections*sizeof(section_t));
    if(header->sections == NULL) {
        printf("ERROR\nCould not allocate required memory\n");
        return -1;
    }
    for(int i=0; i<header->no_of_sections; i++) {

        read(fd, header->sections[i].name, 19);
        read(fd, &header->sections[i].type, 4);
        read(fd, &header->sections[i].offset, 4);
        read(fd, &header->sections[i].size, 4);
        if(typeIsValid(header->sections[i].type)){
            printf("ERROR\nwrong sect_types\n");
            free(header->sections);
            return -1;
        }
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------------------------------PARSE_FUCNTION
header_t* parse(char *filePath){

    int fd = -1;
    header_t* header = (header_t*)malloc(sizeof(header_t));

    fd = open(filePath, O_RDONLY);
    if(fd == -1) {
        printf("ERROR\nThis file can't be opened\n");
        return NULL;
    }
    
    if(fileIsValid(fd, header)){
        return NULL; 
    }

    printf("SUCCESS\n");
	printf("version=%d\n", header->version);
	printf("nr_sections=%d\n", header->no_of_sections);
    
    for(int i=0; i<header->no_of_sections; i++) {

        printf("section%d: %s %d %d\n", i+1, header->sections[i].name, header->sections[i].type, header->sections[i].size);
    }

    close(fd);
    return header;
}

//-------------------------------------------------------------------SOLVE_PARAMETERS_PARSE
header_t* solveParseParameters(int size, char **argv){
    const int cnt = 2;
    char* path = NULL;
    struct stat fileMetadata;

    if(!strncmp(argv[cnt], "path=", 5)) {
        path = argv[cnt] + 5;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISREG(fileMetadata.st_mode)){
        printf("ERROR\nInvalid file path\n");
        return NULL;
    }
    
    return parse(path);
}

//------------------------------------------------------------------------------------------------------------------------------MAIN_FUNCTION
int main(int argc, char **argv){

    // header_t* header = NULL;

    if(argc >= 2){
        if(!strcmp(argv[1], "variant")){
            printf("11416\n");
        }
        if(!strcmp(argv[1], "list")) {
            solveListParameters(argc, argv);
        }
        if(!strcmp(argv[1], "parse")) {
            solveParseParameters(argc, argv);
        }
    }
    return 0;
}