#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>

#define RESP_PIPE "RESP_PIPE_11416"
#define REQ_PIPE "REQ_PIPE_11416"

#define SHM_NAME "/gDeNr7EE"
#define SHM_PERMISSION 0664

#define BUFFER_SIZE 10
#define H_SECTION_SIZE 31
#define ALIGNMENT_SIZE 5120

const unsigned int VARIANT = 11416;
const char* SUCCESS = "SUCCESS";
const char* ERROR = "ERROR";
const char* BEGIN = "BEGIN";
const char* PING = "PING";
const char* PONG = "PONG";
const char* EXIT = "EXIT";
const char* CREATE_SHM = "CREATE_SHM";
const char* WRITE_TO_SHM = "WRITE_TO_SHM";
const char* MAP_THE_FILE = "MAP_FILE";
const char* READ_FROM_FILE_OFFSET = "READ_FROM_FILE_OFFSET";
const char* READ_FROM_FILE_SECTION = "READ_FROM_FILE_SECTION";
const char* READ_FROM_LOGICAL_SPACE_OFFSET = "READ_FROM_LOGICAL_SPACE_OFFSET";
const char END_CHAR = '$';

int fd_RESP = -1, fd_REQ = -1;
int fd_FILE = -1, fd_SHM = -1;
unsigned int shm_size = 0, file_size = 0;
volatile char *sharedData = NULL;
char *fileData = NULL;

int exit_program = 0;

int read_string(char** string_out)
{
    char* string = (char*)malloc(BUFFER_SIZE * sizeof(char));
    if (string == NULL) {
        printf("Memory allocation failed.\n");
        return -1;
    }
    int cnt = 0;
    char c;
    while (read(fd_REQ, &c, sizeof(char)) == sizeof(char)) {
        if (c == END_CHAR) {
            break;
        }
        string[cnt++] = c;
        if (cnt % BUFFER_SIZE == 0) {
            string = (char*)realloc(string, (cnt + BUFFER_SIZE) * sizeof(char));
            if (string == NULL) {
                printf("Memory reallocation failed.\n");
                free(string);
                return -1;
            }
        }
    }
    string[cnt] = '\0';
    *string_out = string;
    return 0;
}
void write_string(const char *string)
{
    for(int i=0; i<strlen(string); i++){
        write(fd_RESP, &string[i], sizeof(char));
    }
    write(fd_RESP, &END_CHAR, sizeof(char));
}

void read_uint(unsigned int *value)
{
    read(fd_REQ, value, sizeof(unsigned int));
}
void write_uint(const unsigned int value)
{
    write(fd_RESP, &value, sizeof(unsigned int));
}

int connect_pipes()
{ 
    if(mkfifo(RESP_PIPE, 0644) != 0) {
        printf("ERROR\n");
        printf("cannot create the response pipe\n");
        return -1;
    }

    fd_REQ = open(REQ_PIPE, O_RDONLY);
    if(fd_REQ == -1) {
        printf("ERROR\n");
        printf("cannot open the request pipe\n");
        return -1;
    }
    fd_RESP = open(RESP_PIPE, O_WRONLY);
    if(fd_RESP == -1) {
        printf("ERROR\n");
        printf("cannot open the response pipe\n");
        return -1;
    }
   
    write_string(BEGIN);

    return 0;
}

void ping()
{
    write_string(PING);

    write_uint(VARIANT);

    write_string(PONG);
}

void solve_exit() 
{
    //---------------------------------------SHM
    munmap((void*)fileData, file_size);
    fileData = NULL;
    close(fd_FILE);

    //---------------------------------------FILE
    munmap((void*)sharedData, shm_size);
    sharedData = NULL;
    close(fd_SHM);

    //---------------------------------------EXIT
    close(fd_REQ);
    close(fd_RESP);
    unlink(RESP_PIPE);
    exit_program = 1;
}

int create_shm()
{
    read_uint(&shm_size);

    fd_SHM = shm_open(SHM_NAME, O_RDWR | O_CREAT, SHM_PERMISSION);
    if(fd_SHM == -1){
        perror("Error open");
        write_string(CREATE_SHM);
        write_string(ERROR);
        return -1;
    }

    if(ftruncate(fd_SHM, shm_size) == -1){
        perror("Error ftruncate");
        write_string(CREATE_SHM);
        write_string(ERROR);
        return -1;
    }

    sharedData = (volatile char*)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_SHM, 0);
    if(sharedData == MAP_FAILED){
        perror("Error mmap");
        write_string(CREATE_SHM);
        write_string(ERROR);
        return -1;
    }

    write_string(CREATE_SHM);
    write_string(SUCCESS);

    return 0;
}

int write_to_shm()
{
    unsigned int offset = 0, value = 0;
    read_uint(&offset);
    read_uint(&value);

    unsigned int byte0 = (value >> 24) & 0xFF;
    unsigned int byte1 = (value >> 16) & 0xFF;
    unsigned int byte2 = (value >> 8) & 0xFF;
    unsigned int byte3 = value & 0xFF;

    if(offset <= 0 || offset >= (shm_size-sizeof(value))){
        write_string(WRITE_TO_SHM);
        write_string(ERROR);
        return -1;
    }

    sharedData[offset] = byte3;
    sharedData[offset + 1] = byte2;
    sharedData[offset + 2] = byte1;
    sharedData[offset + 3] = byte0;

    write_string(WRITE_TO_SHM);
    write_string(SUCCESS);

    return 0;
}

int map_file()
{
    char *filename = "";
    read_string(&filename);

    fd_FILE = open(filename, O_RDONLY);
    if(fd_FILE == -1) {
        write_string(MAP_THE_FILE);
        write_string(ERROR);
        return -1;
    }

    file_size = lseek(fd_FILE, 0, SEEK_END);
    lseek(fd_FILE, 0, SEEK_SET);

    fileData = (char *)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd_FILE, 0);
    if(fileData == MAP_FAILED) {
        write_string(MAP_THE_FILE);
        write_string(ERROR);
        return -1;
    }

    write_string(MAP_THE_FILE);
    write_string(SUCCESS);
    return 0;
}

int read_from_file_offset()
{
    unsigned int offset = 0, no_of_bytes = 0;
    read_uint(&offset);
    read_uint(&no_of_bytes);

    if (fd_SHM == -1) {
        write_string(READ_FROM_FILE_OFFSET);
        write_string(ERROR);
        return -1;
    }
    if (fileData == MAP_FAILED) {
        write_string(READ_FROM_FILE_OFFSET);
        write_string(ERROR);
        return -1;
    }
    if (offset + no_of_bytes > file_size) {
        write_string(READ_FROM_FILE_OFFSET);
        write_string(ERROR);
        return -1;
    }

    int cnt = 0;
    for(int i=offset; i<=offset+no_of_bytes; i++) {
        sharedData[cnt++] = fileData[i]; 
    }

    write_string(READ_FROM_FILE_OFFSET);
    write_string(SUCCESS);
    return 0;
}

int find_section_data(int section_no, unsigned int *section_offset, unsigned int *section_size)
{
    unsigned int index = 2+2+1;
    unsigned int no_of_section = fileData[index];

    if(section_no < 1 || section_no > no_of_section){
        return -1;
    }

    index += H_SECTION_SIZE * (section_no-1) + 1;
    index += 19 + 4;

    unsigned int byte0 = fileData[index] & 0xFF;
    unsigned int byte1 = fileData[index+1] & 0xFF;
    unsigned int byte2 = fileData[index+2] & 0xFF;
    unsigned int byte3 = fileData[index+3] & 0xFF;

    *section_offset = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
    if (*section_offset < 0) {
        return -1;
    }

    index += 4;
    byte0 = fileData[index] & 0xFF;
    byte1 = fileData[index+1] & 0xFF;
    byte2 = fileData[index+2] & 0xFF;
    byte3 = fileData[index+3] & 0xFF;

    *section_size = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
    if (*section_size < 0) {
        return -1;
    }
    return 0;
}

int read_from_file_section()
{
    unsigned int section_no = 0, offset = 0, no_of_bytes = 0;
    unsigned int section_offset = 0, section_size = 0;
    read_uint(&section_no);
    read_uint(&offset);
    read_uint(&no_of_bytes);

    if (fd_SHM == -1) {
        write_string(READ_FROM_FILE_SECTION);
        write_string(ERROR);
        return -1;
    }
    if (fileData == MAP_FAILED) {
        write_string(READ_FROM_FILE_SECTION);
        write_string(ERROR);
        return -1;
    }

    if(find_section_data(section_no, &section_offset, &section_size) == -1){
        write_string(READ_FROM_FILE_SECTION);
        write_string(ERROR);
        return -1;
    }

    if (section_offset + offset + no_of_bytes > file_size) {
        write_string(READ_FROM_FILE_SECTION);
        write_string(ERROR);
        return -1;
    }

    unsigned int cnt = 0;
    unsigned int index = section_offset + offset;
    for(int i=index; i<=index + no_of_bytes; i++) {
        sharedData[cnt++] = fileData[i];
    }

    write_string(READ_FROM_FILE_SECTION);
    write_string(SUCCESS);
    return 0;
}

int read_from_logical_space_offset()
{
    unsigned int logical_offset = 0, no_of_bytes = 0;
    read_uint(&logical_offset);
    read_uint(&no_of_bytes);
    
    if (fd_SHM == -1) {
        write_string(READ_FROM_LOGICAL_SPACE_OFFSET);
        write_string(ERROR);
        return -1;
    }
    if (fileData == MAP_FAILED) {
        write_string(READ_FROM_LOGICAL_SPACE_OFFSET);
        write_string(ERROR);
        return -1;
    }

    unsigned int cur_section = 1;
    unsigned int section_offset = 0, section_size = 0;

    while(1) {
        if(find_section_data(cur_section, &section_offset, &section_size) == -1){
            write_string(READ_FROM_FILE_SECTION);
            write_string(ERROR);
            return -1;
        }
        if(logical_offset < section_size) {
            break;
        }
        else {
            cur_section++;
            logical_offset = logical_offset - (section_size / ALIGNMENT_SIZE + 1) * ALIGNMENT_SIZE;
        }
    }

    if (section_offset + logical_offset + no_of_bytes > file_size) {
        write_string(READ_FROM_LOGICAL_SPACE_OFFSET);
        write_string(ERROR);
        return -1;
    }

    unsigned int cnt = 0;
    unsigned int index = section_offset + logical_offset;
    for(int i=index; i<=index + no_of_bytes; i++) {
        sharedData[cnt++] = fileData[i];
    }

    write_string(READ_FROM_LOGICAL_SPACE_OFFSET);
    write_string(SUCCESS);
    return 0;
}

void solve_request()
{
    char *req_name = "";
    
    read_string(&req_name);
    printf("%s\n", req_name);

    if(!strcmp(req_name, PING)) {
        ping();
    }
    else if(!strcmp(req_name, CREATE_SHM)) {
        create_shm();
    }
    else if(!strcmp(req_name, WRITE_TO_SHM)) {
        write_to_shm();
    }
    else if(!strcmp(req_name, MAP_THE_FILE)) {
        map_file();
    }
    else if(!strcmp(req_name, READ_FROM_FILE_OFFSET)) {
        read_from_file_offset();
    }
    else if(!strcmp(req_name, READ_FROM_FILE_SECTION)) {
        
        read_from_file_section();
    }
    else if(!strcmp(req_name, READ_FROM_LOGICAL_SPACE_OFFSET)) {
        read_from_logical_space_offset();
    }
    else if(!strcmp(req_name, EXIT)) {
        solve_exit();
    }
    else{
        exit_program = 1;
    }
}

int main(int argc, char **argv)
{
    if(connect_pipes() == 0) {
        printf("SUCCESS\n");
        solve_request();
        // solve_request();
        while(exit_program == 0) {
            solve_request();
        }
    }
            
    return 0;
}