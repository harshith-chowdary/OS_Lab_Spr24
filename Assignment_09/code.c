#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <utime.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define MAX_PATH_LENGTH 1024

void sync_directories(const char *src_path, const char *dst_path);
void sync_items(const char *src_item_path, const char *dst_item_path);
void copy_file(const char *src_path, const char *dst_path);
void report_change(char type, const char *item_path);
int remove_directory(const char *path);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <source directory> <destination directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    sync_directories(src_path, dst_path);

    return 0;
}

void sync_directories(const char *src_path, const char *dst_path) {
    struct stat src_stat;
    struct stat dst_stat;

    if (stat(src_path, &src_stat) == -1) {
        // If source item doesn't exist, delete the corresponding item in the destination
        if (remove(dst_path) == 0) {
            report_change('-', dst_path);
        }

        return;
    }

    if (stat(dst_path, &dst_stat) == -1) {
        // If destination item doesn't exist, copy the source item to the destination
        mkdir(dst_path, src_stat.st_mode);

        // Change timestamps and permissions
        struct utimbuf times;
        times.actime = src_stat.st_atime;
        times.modtime = src_stat.st_mtime;
        utime(dst_path, &times);

        // Report changes
        report_change('+', dst_path);
        report_change('t', dst_path);

        if(src_stat.st_mode != dst_stat.st_mode) {
            chmod(dst_path, src_stat.st_mode);
            report_change('p', dst_path);
        }
    }

    DIR *src_dir = opendir(src_path);
    DIR *dst_dir = opendir(dst_path);

    struct dirent *src_entry;
    struct dirent *dst_entry;

    if (!src_dir || !dst_dir) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    while ((src_entry = readdir(src_dir)) != NULL) {
        if (strcmp(src_entry->d_name, ".") == 0 || strcmp(src_entry->d_name, "..") == 0) {
            continue;
        }

        char src_item_path[MAX_PATH_LENGTH];
        char dst_item_path[MAX_PATH_LENGTH];

        snprintf(src_item_path, sizeof(src_item_path), "%s/%s", src_path, src_entry->d_name);
        snprintf(dst_item_path, sizeof(dst_item_path), "%s/%s", dst_path, src_entry->d_name);

        sync_items(src_item_path, dst_item_path);
    }

    while ((dst_entry = readdir(dst_dir)) != NULL) {
        if (strcmp(dst_entry->d_name, ".") == 0 || strcmp(dst_entry->d_name, "..") == 0) {
            continue;
        }

        char src_item_path[MAX_PATH_LENGTH];
        char dst_item_path[MAX_PATH_LENGTH];

        snprintf(src_item_path, sizeof(src_item_path), "%s/%s", src_path, dst_entry->d_name);
        snprintf(dst_item_path, sizeof(dst_item_path), "%s/%s", dst_path, dst_entry->d_name);

        sync_items(src_item_path, dst_item_path);
    }

    closedir(src_dir);
    closedir(dst_dir);
}

void sync_items(const char *src_item_path, const char *dst_item_path) {
    struct stat src_stat;
    struct stat dst_stat;

    int src_stat_result = stat(src_item_path, &src_stat);
    int dst_stat_result = stat(dst_item_path, &dst_stat);

    if (src_stat_result == -1) {
        // If source item doesn't exist, delete the corresponding item in the destination

        if (S_ISDIR(dst_stat.st_mode)) {
            // If destination item is a directory, remove it recursively
            // char rm_command[MAX_PATH_LENGTH + 6];  // Length of "rm -rf " plus the maximum path length
            // snprintf(rm_command, sizeof(rm_command), "rm -rf %s", dst_item_path);
            // if (system(rm_command) == 0) {
            //     report_change('-', dst_item_path);
            // } else {
            //     perror("Error removing destination directory");
            // }

            remove_directory(dst_item_path);
        } else {
            if (remove(dst_item_path) == 0) {
                report_change('-', dst_item_path);
            } else {
                perror("Error removing destination file");
            }
        }

        return;
    }

    if (S_ISDIR(src_stat.st_mode)) {
        // If both are directories, synchronize the directories
        if(S_ISDIR(dst_stat.st_mode)) {
            sync_directories(src_item_path, dst_item_path);
            return;
        }
        else{
            if (remove(dst_item_path) == 0) {
                report_change('-', dst_item_path);
            } else {
                perror("Error removing destination file");
            }
        }
    }

    dst_stat_result = stat(dst_item_path, &dst_stat);

    if (dst_stat_result == -1) {
        if(S_ISDIR(src_stat.st_mode)) {
            sync_directories(src_item_path, dst_item_path);

            return;
        }

        // If destination item doesn't exist, copy the source item to the destination
        copy_file(src_item_path, dst_item_path);

        // Change timestamps and permissions
        struct utimbuf times;
        times.actime = src_stat.st_atime;
        times.modtime = src_stat.st_mtime;
        utime(dst_item_path, &times);
        chmod(dst_item_path, src_stat.st_mode);

        // Report changes
        report_change('+', dst_item_path);
        report_change('t', dst_item_path);

        if(src_stat.st_mode != dst_stat.st_mode) {
            report_change('p', dst_item_path);
        }

        return;
    }

    // If both are regular files
    if (src_stat.st_size != dst_stat.st_size || src_stat.st_mtime != dst_stat.st_mtime) {
        // If sizes or modification timestamps differ, copy the source file to the destination
        copy_file(src_item_path, dst_item_path);

        // Change timestamps and permissions
        struct utimbuf times;
        times.actime = src_stat.st_atime;
        times.modtime = src_stat.st_mtime;
        utime(dst_item_path, &times);
        chmod(dst_item_path, src_stat.st_mode);

        // Report changes
        report_change('o', dst_item_path);
        report_change('t', dst_item_path);

        if(src_stat.st_mode != dst_stat.st_mode) {
            report_change('p', dst_item_path);
        }
    }
}

void copy_file(const char *src_path, const char *dst_path) {
    char buffer[BUFFER_SIZE];
    size_t bytes_read, bytes_written;

    FILE *src_file = fopen(src_path, "rb");
    if (!src_file) {
        perror("Error opening source file");
        exit(EXIT_FAILURE);
    }

    FILE *dst_file = fopen(dst_path, "wb");
    if (!dst_file) {
        perror("Error opening destination file");
        fclose(src_file);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_read, dst_file);
        if (bytes_written != bytes_read) {
            perror("Error writing to destination file");
            fclose(src_file);
            fclose(dst_file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(src_file);
    fclose(dst_file);
}

void report_change(char type, const char *item_path) {
    switch (type) {
        case '+':
            printf("[+] %s\n", item_path);
            break;
        case '-':
            printf("[-] %s\n", item_path);
            break;
        case 'o':
            printf("[o] %s\n", item_path);
            break;
        case 't':
            printf("[t] %s\n", item_path);
            break;
        case 'p':
            printf("[p] %s\n", item_path);
            break;
        default:
            break;
    }
}

int remove_directory(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int error = 0;

    if (!d) {
        fprintf(stderr, "Failed to open directory %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = path_len + strlen(entry->d_name) + 2;
        char *child_path = malloc(len);
        if (!child_path) {
            fprintf(stderr, "Memory allocation failed.\n");
            error = -1;
            break;
        }

        snprintf(child_path, len, "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            remove_directory(child_path);
        } else {
            if (unlink(child_path) != 0) {
                fprintf(stderr, "Failed to remove file %s: %s\n", child_path, strerror(errno));
                error = -1;
                free(child_path);
                break;
            }

            report_change('-', child_path);
        }
        free(child_path);
    }

    closedir(d);

    if (rmdir(path) != 0) {
        fprintf(stderr, "Failed to remove directory %s: %s\n", path, strerror(errno));
        return -1;
    }

    report_change('-', path);

    return error;
}