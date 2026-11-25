#include "directory.h"
#include "fat32.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CURRENT_PATH_LENGTH 4096

FILE *image_file = NULL;

static const char *get_image_name(const char *path)
{
    const char *forward_slash = strrchr(path, '/');
    const char *backward_slash = strrchr(path, '\\');
    const char *last_separator = forward_slash;

    if (backward_slash != NULL &&
        (last_separator == NULL || backward_slash > last_separator)) {
        last_separator = backward_slash;
    }

    if (last_separator == NULL)
        return path;
    return last_separator + 1;
}

static void print_prompt(const char *image_name, const char *current_path)
{
    if (current_path[0] == '\0')
        printf("%s/> ", image_name);
    else
        printf("%s/%s/> ", image_name, current_path);
    fflush(stdout);
}

static void move_path_to_parent(char *current_path)
{
    char *last_separator = strrchr(current_path, '/');

    if (last_separator == NULL)
        current_path[0] = '\0';
    else
        *last_separator = '\0';
}

static bool append_to_path(char *current_path, size_t path_capacity,
                           const char *directory_name)
{
    size_t path_length = strlen(current_path);
    size_t name_length = strlen(directory_name);
    size_t separator_length = path_length == 0 ? 0 : 1;

    if (path_length + separator_length + name_length + 1 > path_capacity)
        return false;

    if (separator_length != 0)
        strcat(current_path, "/");
    strcat(current_path, directory_name);
    return true;
}

int main(int argc, char *argv[])
{
    bool running = true;
    const char *image_name;
    Fat32BootSector boot_sector;
    uint32_t current_directory_cluster;
    char current_path[CURRENT_PATH_LENGTH] = "";

    if (argc != 2) {
        fprintf(stderr, "Error: expected one FAT32 image path.\n");
        fprintf(stderr, "Usage: %s [FAT32 image]\n", argv[0]);
        return EXIT_FAILURE;
    }

    image_file = fopen(argv[1], "r+b");
    if (image_file == NULL) {
        fprintf(stderr, "Error: image '%s' does not exist or cannot be opened.\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (!load_boot_sector(image_file, &boot_sector)) {
        fprintf(stderr, "Error: image '%s' does not contain a valid FAT32 BPB.\n", argv[1]);
        fclose(image_file);
        image_file = NULL;
        return EXIT_FAILURE;
    }

    image_name = get_image_name(argv[1]);
    current_directory_cluster = boot_sector.BPB_RootClus;

    while (running) {
        char *input;
        tokenlist *tokens;

        print_prompt(image_name, current_path);
        input = get_input();
        if (input == NULL) {
            putchar('\n');
            break;
        }

        tokens = get_tokens(input);
        if (tokens->size == 0) {
            /* An empty command returns to the prompt. */
        } else if (strcmp(tokens->items[0], "info") == 0) {
            if (tokens->size != 1) {
                fprintf(stderr, "Error: info does not accept arguments.\n");
            } else if (!print_file_system_info(image_file, &boot_sector)) {
                fprintf(stderr, "Error: could not read file system information.\n");
            }
        } else if (strcmp(tokens->items[0], "ls") == 0) {
            if (tokens->size != 1) {
                fprintf(stderr, "Error: ls does not accept arguments.\n");
            } else if (!list_directory(image_file, &boot_sector,
                                       current_directory_cluster)) {
                fprintf(stderr, "Error: could not read current directory.\n");
            }
        } else if (strcmp(tokens->items[0], "cd") == 0) {
            if (tokens->size != 2) {
                fprintf(stderr, "Error: cd requires one directory name.\n");
            } else if (strcmp(tokens->items[1], ".") == 0) {
                /* The current directory does not change. */
            } else if (strcmp(tokens->items[1], "..") == 0 &&
                       current_directory_cluster == boot_sector.BPB_RootClus) {
                /* The root directory is its own parent. */
            } else {
                Fat32DirectoryEntry entry;
                DirectorySearchResult search_result =
                    find_directory_entry(image_file, &boot_sector,
                                         current_directory_cluster,
                                         tokens->items[1], &entry);

                if (search_result == DIRECTORY_SEARCH_ERROR) {
                    fprintf(stderr, "Error: could not search current directory.\n");
                } else if (search_result == DIRECTORY_NOT_FOUND) {
                    fprintf(stderr, "Error: directory '%s' does not exist.\n",
                            tokens->items[1]);
                } else if (!is_directory_entry(&entry)) {
                    fprintf(stderr, "Error: '%s' is not a directory.\n",
                            tokens->items[1]);
                } else {
                    uint32_t next_directory_cluster =
                        get_directory_entry_cluster(&entry);

                    if (strcmp(tokens->items[1], "..") == 0) {
                        if (next_directory_cluster == 0)
                            next_directory_cluster = boot_sector.BPB_RootClus;
                        current_directory_cluster = next_directory_cluster;
                        move_path_to_parent(current_path);
                    } else {
                        char directory_name[FAT32_SHORT_NAME_LENGTH];

                        format_short_name(&entry, directory_name);
                        if (!append_to_path(current_path, sizeof(current_path),
                                            directory_name)) {
                            fprintf(stderr, "Error: current path is too long.\n");
                        } else {
                            current_directory_cluster = next_directory_cluster;
                        }
                    }
                }
            }
        } else if (strcmp(tokens->items[0], "exit") == 0) {
            if (tokens->size != 1)
                fprintf(stderr, "Error: exit does not accept arguments.\n");
            else
                running = false;
        } else {
            fprintf(stderr, "Error: unknown command '%s'.\n", tokens->items[0]);
        }

        free(input);
        free_tokens(tokens);
    }

    if (fclose(image_file) != 0) {
        fprintf(stderr, "Error: could not close image '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    image_file = NULL;
    return EXIT_SUCCESS;
}
