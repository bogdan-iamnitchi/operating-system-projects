#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NEW_LINE 10
#define MAX_SECTION_SIZE 1314
#define MAX_PATH_SIZE 512
#define READ_AMOUNT 64

typedef struct {
    char name[20];
    int type;
    off_t offset;
    off_t size;
} section_t;

typedef struct {
    char magic[3];
    int header_size;
    int version;
    int no_of_sections;
    section_t *sections;
}header_t;

int findAll(char *filePath);

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

int fileIsValid(int errMsg, int fd, header_t* header) {

    //---------------------------------------MAGIC
    if(read(fd, header->magic, 2) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    }
    header->magic[2] = '\0';
    if (strncmp(header->magic, "Q0", 2)) {
        if(errMsg) {printf("ERROR\nwrong magic\n");}
        return -1;
    }
    
    //---------------------------------------HEADER_SIZE
    header->header_size = 0;
    if(read(fd, &header->header_size, 2) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    }
    //----------------------------------------VERSION
    header->version = 0;
    if(read(fd, &header->version, 1) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    } 
    else if (header->version < 73 || header->version > 97)
    {
        if(errMsg) {printf("ERROR\nwrong version\n");}
        return -1;
    }
    
    //----------------------------------------NUMBER_OF_SECTIONS
    header->no_of_sections = 0;
    if(read(fd, &header->no_of_sections, 1) < 0){
        printf("ERROR\nCould not read from file\n");
        return -1;
    } 
    else if (header->no_of_sections < 3 || header->no_of_sections > 14) {
        if(errMsg) {printf("ERROR\nwrong sect_nr\n");}
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
        header->sections[i].name[20] = '\0';
        header->sections[i].type = 0;
        read(fd, &header->sections[i].type, 4);
        header->sections[i].offset = 0;
        read(fd, &header->sections[i].offset, 4);
        header->sections[i].size = 0;
        read(fd, &header->sections[i].size, 4);
        if(typeIsValid(header->sections[i].type)){
            if(errMsg) {printf("ERROR\nwrong sect_types\n");}
            free(header->sections);
            return -1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------------------------------------------------LIST_FUCNTION
int list(int checkValid, int recursive, long int min_size, int has_perm_write, const char* dirPath){
    static int cnt = 0;
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat fileMetadata;

    off_t size_path = MAX_PATH_SIZE;
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
                        if(cnt++ == 0) {
                            printf("SUCCESS\n");
                        }
                        printf("%s\n", fullPath);
                    }
                }
                else if(has_perm_write){
                    if(fileMetadata.st_mode & S_IWUSR) {
                        if(cnt++ == 0) {
                            printf("SUCCESS\n");
                        }
                        printf("%s\n", fullPath);
                    }
                }
                else if(checkValid && S_ISREG(fileMetadata.st_mode)) {
                    if(!findAll(fullPath)){
                        if(cnt++ == 0) {
                            printf("SUCCESS\n");
                        }
                        printf("%s\n", fullPath);
                    }
                }
                else if(!checkValid){
                    if(cnt++ == 0) {
                            printf("SUCCESS\n");
                        }
                        printf("%s\n", fullPath);
                }
                
                if(recursive) {
                    if(S_ISDIR(fileMetadata.st_mode)) {
                        list(checkValid, recursive, min_size, has_perm_write, fullPath);
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
    int checkValid = 0;
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

    return list(checkValid, recursive, min_size, has_perm_write, path);
}

//-------------------------------------------------------------------------------------------------------------------------------PARSE_FUCNTION
int parse(char *filePath){

    int fd = -1, errMsg = 1;
    header_t* header = (header_t*)malloc(sizeof(header_t));
    if(header == NULL) {
        printf("ERROR\nCould not allocate required memory\n");
        return -1;
    }

    fd = open(filePath, O_RDONLY);
    if(fd == -1) {
        printf("ERROR\nThis file can't be opened\n");
        free(header);
        return -1;
    }
    
    if(fileIsValid(errMsg, fd, header) != 0){
        close(fd);
        free(header);
        return -1; 
    }

    printf("SUCCESS\n");
	printf("version=%d\n", header->version);
	printf("nr_sections=%d\n", header->no_of_sections);
    
    for(int i=0; i<header->no_of_sections; i++) {

        printf("section%d: %s %d %ld\n", i+1, header->sections[i].name, header->sections[i].type, header->sections[i].size);
    }

    close(fd);
    free(header->sections);
    free(header);
    return 0;
}

//-------------------------------------------------------------------SOLVE_PARAMETERS_PARSE
int solveParseParameters(int size, char **argv){
    const int cnt = 2;
    char* path = NULL;
    struct stat fileMetadata;

    if(!strncmp(argv[cnt], "path=", 5)) {
        path = argv[cnt] + 5;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISREG(fileMetadata.st_mode)){
        printf("ERROR\nInvalid file path\n");
        return -1;
    }
    
    return parse(path);
}

//--------------------------------------------------------------------------------------------------------------------------ISVALID_EXTRACT_FUCNTION
int extractLinie(int fd, header_t* header, char* filePath, int sect_nr, int line_nr) {
    
    char c;
    int nr_lines = 1;
    int cnt_size = 1;
    off_t size_to_jump = header->sections[sect_nr-1].offset + header->sections[sect_nr-1].size;

    while (cnt_size <= header->sections[sect_nr-1].size) {
        if(nr_lines == line_nr) {
            printf("SUCCESS\n");
            break;
        }
		lseek(fd, (size_to_jump - (cnt_size++)), SEEK_SET);
        if(read(fd, &c, sizeof(char))< 0) {
            printf("ERROR\nCould not read from file\n");
            return -1;
        }
        if(c == NEW_LINE) {
            nr_lines++;
        }
	}

    if(cnt_size > header->sections[sect_nr-1].size) {
        printf("ERROR\ninvalid line\n");
        return -1;
    }

    while (nr_lines == line_nr && cnt_size <= header->sections[sect_nr-1].size) {
		lseek(fd, (size_to_jump - (cnt_size++)), SEEK_SET);
        if(read(fd, &c, sizeof(char)) < 0) {
            printf("ERROR\nCould not read from file\n");
            return -1;
        }
        if(c == NEW_LINE) {
            nr_lines++;
        }
        printf("%c", c);
	}

    return 0;
}


//-------------------------------------------------------------------------------------------------------------------------------EXTRACT_FUCNTION
int extract(char *filePath, int sect_nr, int line_nr){

    int fd = -1, errMsg = 0;
    header_t* header = (header_t*)malloc(sizeof(header_t));
    if(header == NULL) {
        printf("ERROR\nCould not allocate required memory\n");
        return -1;
    }

    fd = open(filePath, O_RDONLY);
    if(fd == -1) {
        printf("ERROR\nThis file can't be opened\n");
        close(fd);
        free(header);
        return -1;
    }
    if(fileIsValid(errMsg, fd, header) != 0){
        close(fd);
        free(header);
        return -1; 
    }
    if(sect_nr < 0 || sect_nr > header->no_of_sections) {
        printf("ERROR\ninvalid section\n");
        close(fd);
        free(header->sections);
        free(header);
        return -1;
    }
    if(extractLinie(fd, header, filePath, sect_nr, line_nr) != 0) {
        close(fd);
        free(header->sections);
        free(header);
        return -1;
    }

    close(fd);
    free(header->sections);
    free(header);
    return 0;
}

//---------------------------------------------------------SOLVE_PARAMETERS_EXTRACT
int solveExtractParameters(int size, char **argv){
    int cnt = 2;
    int sect_nr = 0;
    int line_nr = 0;
    char* path = NULL;
    struct stat fileMetadata;

    while(cnt < size){
        if(!strncmp(argv[cnt], "section=", 8)) {
            sect_nr = atoi(argv[cnt] + 8);
        }
        if(!strncmp(argv[cnt], "line=", 5)) {
            line_nr = atoi(argv[cnt] + 5);
        }
        if(!strncmp(argv[cnt], "path=", 5)) {
            path = argv[cnt] + 5;
        }
        cnt++;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISREG(fileMetadata.st_mode)){
        printf("ERROR\ninvalid file\n");
        return -1;
    }

    return extract(path, sect_nr, line_nr);
}

//-------------------------------------------------------------------------------------------------------------------------------EXTRACT_FUCNTION
int findAll(char *filePath){

    int fd = -1, errMsg = 0;
    header_t* header = (header_t*)malloc(sizeof(header_t));
    if(header == NULL) {
        printf("ERROR\nCould not allocate required memory\n");
        return -1;
    }

    fd = open(filePath, O_RDONLY);
    if(fd == -1) {
        printf("ERROR\nThis file can't be opened\n");
        close(fd);
        free(header);
        return -1;
    }

    if(fileIsValid(errMsg, fd, header)!=0) {
        close(fd);
        free(header);
        return -1;
    }
    for(int i=0; i<header->no_of_sections; i++){
        if(header->sections[i].size > MAX_SECTION_SIZE) {
            close(fd);
            free(header->sections);
            free(header);
            return -1;
        }
    }

    free(header->sections);
    free(header);
    close(fd);
    return 0;
}


//---------------------------------------------------------SOLVE_PARAMETERS_FINDALL
int solveFindAllParameters(int size, char **argv){
    int cnt = 2;
    int recursive = 1;
    long int min_size = -1;
    int has_perm_write = 0;
    int checkValid = 1;
    char* path = NULL;
    struct stat fileMetadata;

    if(!strncmp(argv[cnt], "path=", 5)) {
        path = argv[cnt] + 5;
    }

    if(lstat(path, &fileMetadata) < 0 || !S_ISDIR(fileMetadata.st_mode)){
        printf("ERROR\ninvalid directory path\n");
        return -1;
    }

    return list(checkValid, recursive, min_size, has_perm_write, path);
}

//------------------------------------------------------------------------------------------------------------------------------MAIN_FUNCTION
int main(int argc, char **argv){

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
        if(!strcmp(argv[1], "extract")) {
            solveExtractParameters(argc, argv);
        }
        if(!strcmp(argv[1], "findall")) {
            solveFindAllParameters(argc, argv);
        }
    }

    return 0;
}