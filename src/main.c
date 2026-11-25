#include "directory.h"
#include "fat32.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void print_prompt(const char *image_name)
{
    printf("%s/> ", image_name);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    bool running = true;
    const char *image_name;
    Fat32BootSector boot_sector;
    uint32_t current_directory_cluster;

    if (argc != 2) {
        fprintf(stderr, "Error: expected one FAT32 image path.\n");
        fprintf(stderr, "Usage: %s [FAT32 image]\n", argv[0]);
        return EXIT_FAILURE;
    }

    image_file = fopen(argv[1], "r+");
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

        print_prompt(image_name);
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
