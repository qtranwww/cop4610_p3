#include "fat32.h"
#include <stdio.h>

#define FAT32_BPB_SIZE 90
#define FAT32_ENTRY_SIZE 4

static bool is_valid_fat32_bpb(const Fat32BootSector *boot_sector)
{
    return boot_sector->BPB_BytsPerSec != 0 &&
           boot_sector->BPB_SecPerClus != 0 &&
           boot_sector->BPB_RsvdSecCnt != 0 &&
           boot_sector->BPB_NumFATs != 0 &&
           boot_sector->BPB_RootEntCnt == 0 &&
           boot_sector->BPB_TotSec32 != 0 &&
           boot_sector->BPB_FATSz32 != 0 &&
           boot_sector->BPB_RootClus >= 2;
}

bool load_boot_sector(FILE *image, Fat32BootSector *boot_sector)
{
    long initial_position = ftell(image);
    bool read_succeeded;

    if (initial_position < 0 || sizeof(*boot_sector) != FAT32_BPB_SIZE)
        return false;
    if (fseek(image, 0, SEEK_SET) != 0)
        return false;

    read_succeeded = fread(boot_sector, sizeof(*boot_sector), 1, image) == 1;

    if (fseek(image, initial_position, SEEK_SET) != 0)
        return false;

    return read_succeeded && is_valid_fat32_bpb(boot_sector);
}

uint32_t get_total_sectors(const Fat32BootSector *boot_sector)
{
    if (boot_sector->BPB_TotSec16 != 0)
        return boot_sector->BPB_TotSec16;
    return boot_sector->BPB_TotSec32;
}

uint32_t get_fat_size(const Fat32BootSector *boot_sector)
{
    if (boot_sector->BPB_FATSz16 != 0)
        return boot_sector->BPB_FATSz16;
    return boot_sector->BPB_FATSz32;
}

uint32_t get_root_directory_sectors(const Fat32BootSector *boot_sector)
{
    uint32_t root_directory_bytes = boot_sector->BPB_RootEntCnt * 32;

    return (root_directory_bytes + (boot_sector->BPB_BytsPerSec - 1)) /
           boot_sector->BPB_BytsPerSec;
}

uint32_t get_first_data_sector(const Fat32BootSector *boot_sector)
{
    return boot_sector->BPB_RsvdSecCnt +
           (boot_sector->BPB_NumFATs * get_fat_size(boot_sector)) +
           get_root_directory_sectors(boot_sector);
}

uint32_t get_cluster_count(const Fat32BootSector *boot_sector)
{
    uint32_t data_sectors = get_total_sectors(boot_sector) -
                            get_first_data_sector(boot_sector);

    return data_sectors / boot_sector->BPB_SecPerClus;
}

uint32_t get_fat_entry_count(const Fat32BootSector *boot_sector)
{
    return (get_fat_size(boot_sector) * boot_sector->BPB_BytsPerSec) /
           FAT32_ENTRY_SIZE;
}

uint32_t get_first_sector_of_cluster(const Fat32BootSector *boot_sector, uint32_t cluster)
{
    return ((cluster - 2) * boot_sector->BPB_SecPerClus) +
           get_first_data_sector(boot_sector);
}

bool print_file_system_info(FILE *image, const Fat32BootSector *boot_sector)
{
    long initial_position = ftell(image);
    long image_size;

    if (initial_position < 0 || fseek(image, 0, SEEK_END) != 0)
        return false;

    image_size = ftell(image);
    if (fseek(image, initial_position, SEEK_SET) != 0 || image_size < 0)
        return false;

    printf("Position of root cluster: %u\n", boot_sector->BPB_RootClus);
    printf("Bytes per sector: %u\n", boot_sector->BPB_BytsPerSec);
    printf("Sectors per cluster: %u\n", boot_sector->BPB_SecPerClus);
    printf("Total clusters in data region: %u\n", get_cluster_count(boot_sector));
    printf("Entries in one FAT: %u\n", get_fat_entry_count(boot_sector));
    printf("Size of image in bytes: %ld\n", image_size);

    return true;
}
