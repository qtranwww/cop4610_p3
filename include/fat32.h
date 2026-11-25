#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct __attribute__((packed)) {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
} Fat32BootSector;

bool load_boot_sector(FILE *image, Fat32BootSector *boot_sector);
uint32_t get_total_sectors(const Fat32BootSector *boot_sector);
uint32_t get_fat_size(const Fat32BootSector *boot_sector);
uint32_t get_root_directory_sectors(const Fat32BootSector *boot_sector);
uint32_t get_first_data_sector(const Fat32BootSector *boot_sector);
uint32_t get_cluster_count(const Fat32BootSector *boot_sector);
uint32_t get_fat_entry_count(const Fat32BootSector *boot_sector);
uint32_t get_first_sector_of_cluster(const Fat32BootSector *boot_sector, uint32_t cluster);
long get_cluster_byte_offset(const Fat32BootSector *boot_sector, uint32_t cluster);
bool get_next_cluster(FILE *image, const Fat32BootSector *boot_sector,
                      uint32_t cluster, uint32_t *next_cluster);
bool is_end_of_cluster_chain(uint32_t cluster);
bool print_file_system_info(FILE *image, const Fat32BootSector *boot_sector);
