#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
    int file_desc = open("dup.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    // close(0); // close stdin (fd = 0)
    close(1); // close stdout (fd = 1)
    // close(2); // close stderr (fd = 2)

    if(file_desc < 0){
        printf("Error opening file\n");
        exit(1);
    }

    int copy_desc = dup(file_desc);

    printf("file_desc = %d\n", file_desc);
    printf("copy_desc = %d\n", copy_desc);

    char buffer[256];
    sprintf(buffer, "copy_desc = %d\n", copy_desc);

    write(file_desc, buffer, 14);

    if(copy_desc < 0){
        printf("Error copying file descriptor\n");
        exit(1);
    }

    write(copy_desc, "This will be output to the file named dup.txt\n", 46);
    write(file_desc, "This will also be output to the file named dup.txt\n", 51);

    close(file_desc);
    close(copy_desc);

    return 0;
}