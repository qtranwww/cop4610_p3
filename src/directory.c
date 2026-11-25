#include "directory.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

uint32_t get_directory_entry_cluster(const Fat32DirectoryEntry *entry)
{
    return ((uint32_t)entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

bool is_directory_entry(const Fat32DirectoryEntry *entry)
{
    return (entry->DIR_Attr & ATTR_DIRECTORY) != 0;
}

bool is_long_name_entry(const Fat32DirectoryEntry *entry)
{
    return entry->DIR_Attr == ATTR_LONG_NAME;
}

bool is_volume_label_entry(const Fat32DirectoryEntry *entry)
{
    return (entry->DIR_Attr & ATTR_VOLUME_ID) != 0 && !is_long_name_entry(entry);
}

bool is_deleted_entry(const Fat32DirectoryEntry *entry)
{
    return entry->DIR_Name[0] == 0xE5;
}

bool is_end_of_directory(const Fat32DirectoryEntry *entry)
{
    return entry->DIR_Name[0] == 0x00;
}

void format_short_name(const Fat32DirectoryEntry *entry,
                       char output[FAT32_SHORT_NAME_LENGTH])
{
    size_t input_index;
    size_t output_index = 0;

    if (entry->DIR_Name[0] == '.') {
        output[output_index++] = '.';
        if (entry->DIR_Name[1] == '.')
            output[output_index++] = '.';
        output[output_index] = '\0';
        return;
    }

    for (input_index = 0; input_index < 8 && entry->DIR_Name[input_index] != ' ';
         input_index++) {
        output[output_index++] = (char)entry->DIR_Name[input_index];
    }

    if (entry->DIR_Name[8] != ' ') {
        output[output_index++] = '.';
        for (input_index = 8; input_index < 11 && entry->DIR_Name[input_index] != ' ';
             input_index++) {
            output[output_index++] = (char)entry->DIR_Name[input_index];
        }
    }

    output[output_index] = '\0';
}

bool directory_name_matches(const Fat32DirectoryEntry *entry, const char *name)
{
    char entry_name[FAT32_SHORT_NAME_LENGTH];
    size_t index = 0;

    format_short_name(entry, entry_name);

    while (entry_name[index] != '\0' && name[index] != '\0') {
        unsigned char entry_character = (unsigned char)entry_name[index];
        unsigned char name_character = (unsigned char)name[index];

        if (toupper(entry_character) != toupper(name_character))
            return false;
        index++;
    }

    return entry_name[index] == '\0' && name[index] == '\0';
}

bool list_directory(FILE *image, const Fat32BootSector *boot_sector,
                    uint32_t starting_cluster)
{
    long initial_position = ftell(image);
    uint32_t current_cluster = starting_cluster;
    uint32_t cluster_count = get_cluster_count(boot_sector);
    uint32_t entries_per_cluster =
        (boot_sector->BPB_BytsPerSec * boot_sector->BPB_SecPerClus) /
        FAT32_DIRECTORY_ENTRY_SIZE;
    uint32_t visited_clusters = 0;
    bool succeeded = false;

    if (initial_position < 0)
        return false;

    while (visited_clusters < cluster_count) {
        uint32_t entry_index;
        uint32_t next_cluster;

        if (current_cluster < 2 || current_cluster > cluster_count + 1)
            break;
        if (fseek(image, get_cluster_byte_offset(boot_sector, current_cluster),
                  SEEK_SET) != 0) {
            break;
        }

        visited_clusters++;
        for (entry_index = 0; entry_index < entries_per_cluster; entry_index++) {
            Fat32DirectoryEntry entry;
            char name[FAT32_SHORT_NAME_LENGTH];

            if (fread(&entry, sizeof(entry), 1, image) != 1)
                goto restore_position;
            if (is_end_of_directory(&entry)) {
                succeeded = true;
                goto restore_position;
            }
            if (is_deleted_entry(&entry) || is_long_name_entry(&entry) ||
                is_volume_label_entry(&entry)) {
                continue;
            }

            format_short_name(&entry, name);
            puts(name);
        }

        if (!get_next_cluster(image, boot_sector, current_cluster, &next_cluster))
            break;
        if (is_end_of_cluster_chain(next_cluster)) {
            succeeded = true;
            break;
        }

        current_cluster = next_cluster;
    }

restore_position:
    if (fseek(image, initial_position, SEEK_SET) != 0)
        return false;
    return succeeded;
}

DirectorySearchResult find_directory_entry(FILE *image,
                                           const Fat32BootSector *boot_sector,
                                           uint32_t starting_cluster,
                                           const char *name,
                                           Fat32DirectoryEntry *result)
{
    long initial_position = ftell(image);
    uint32_t current_cluster = starting_cluster;
    uint32_t cluster_count = get_cluster_count(boot_sector);
    uint32_t entries_per_cluster =
        (boot_sector->BPB_BytsPerSec * boot_sector->BPB_SecPerClus) /
        FAT32_DIRECTORY_ENTRY_SIZE;
    uint32_t visited_clusters = 0;
    DirectorySearchResult search_result = DIRECTORY_SEARCH_ERROR;

    if (initial_position < 0)
        return DIRECTORY_SEARCH_ERROR;

    while (visited_clusters < cluster_count) {
        uint32_t entry_index;
        uint32_t next_cluster;

        if (current_cluster < 2 || current_cluster > cluster_count + 1)
            break;
        if (fseek(image, get_cluster_byte_offset(boot_sector, current_cluster),
                  SEEK_SET) != 0) {
            break;
        }

        visited_clusters++;
        for (entry_index = 0; entry_index < entries_per_cluster; entry_index++) {
            Fat32DirectoryEntry entry;

            if (fread(&entry, sizeof(entry), 1, image) != 1)
                goto restore_position;
            if (is_end_of_directory(&entry)) {
                search_result = DIRECTORY_NOT_FOUND;
                goto restore_position;
            }
            if (is_deleted_entry(&entry) || is_long_name_entry(&entry) ||
                is_volume_label_entry(&entry)) {
                continue;
            }
            if (directory_name_matches(&entry, name)) {
                *result = entry;
                search_result = DIRECTORY_FOUND;
                goto restore_position;
            }
        }

        if (!get_next_cluster(image, boot_sector, current_cluster, &next_cluster))
            break;
        if (is_end_of_cluster_chain(next_cluster)) {
            search_result = DIRECTORY_NOT_FOUND;
            break;
        }

        current_cluster = next_cluster;
    }

restore_position:
    if (fseek(image, initial_position, SEEK_SET) != 0)
        return DIRECTORY_SEARCH_ERROR;
    return search_result;
}
