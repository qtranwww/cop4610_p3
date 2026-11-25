#pragma once

#include "fat32.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F

#define FAT32_DIRECTORY_ENTRY_SIZE 32
#define FAT32_SHORT_NAME_LENGTH 13

typedef struct __attribute__((packed)) {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} Fat32DirectoryEntry;

uint32_t get_directory_entry_cluster(const Fat32DirectoryEntry *entry);
bool is_directory_entry(const Fat32DirectoryEntry *entry);
bool is_long_name_entry(const Fat32DirectoryEntry *entry);
bool is_volume_label_entry(const Fat32DirectoryEntry *entry);
bool is_deleted_entry(const Fat32DirectoryEntry *entry);
bool is_end_of_directory(const Fat32DirectoryEntry *entry);
void format_short_name(const Fat32DirectoryEntry *entry,
                       char output[FAT32_SHORT_NAME_LENGTH]);
bool directory_name_matches(const Fat32DirectoryEntry *entry, const char *name);
bool list_directory(FILE *image, const Fat32BootSector *boot_sector,
                    uint32_t starting_cluster);
