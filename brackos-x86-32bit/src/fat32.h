#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stddef.h>

#define SECTOR_SIZE 512
#define MAX_FILENAME 12  // 8.3 format
#define MAX_FILES 32

// FAT32 Boot Sector
typedef struct {
    uint8_t jmp[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fats;
    uint16_t root_entries;
    uint16_t small_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat_small;
    uint16_t sectors_per_head;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved2;
    uint8_t boot_signature;
    uint32_t serial;
    uint8_t label[11];
    uint8_t system[8];
} __attribute__((packed)) boot_sector_t;

// Directory Entry (32 bytes)
typedef struct {
    uint8_t filename[11];
    uint8_t attributes;
    uint8_t nt_reserved;
    uint8_t creation_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) dir_entry_t;

// File descriptor
typedef struct {
    char filename[MAX_FILENAME];
    uint32_t first_cluster;
    uint32_t file_size;
    uint32_t current_position;
    dir_entry_t dir_entry;
    int in_use;
} file_descriptor_t;

// FAT32 filesystem state
typedef struct {
    boot_sector_t boot;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t root_dir_cluster;
    
    file_descriptor_t files[MAX_FILES];
} fat32_fs_t;

extern fat32_fs_t fat32_fs;

// Filesystem operations
int fat32_init(uint32_t drive_lba);
int fat32_read_sector(uint32_t sector, uint8_t *buffer);
int fat32_write_sector(uint32_t sector, uint8_t *buffer);

// Directory operations
int fat32_list_directory(uint32_t cluster, dir_entry_t *entries, int max_entries);
int fat32_find_file(const char *filename, dir_entry_t *entry);
int fat32_find_in_dir(uint32_t dir_cluster, const char *filename, dir_entry_t *entry);

// Whole-file operations (dir_cluster = directory the file lives in)
int fat32_read_file(uint32_t dir_cluster, const char *name, uint8_t *buf, uint32_t cap);
int fat32_write_file(uint32_t dir_cluster, const char *name, const uint8_t *data, uint32_t size);
int fat32_delete_file(uint32_t dir_cluster, const char *name);

// FAT chain navigation
uint32_t fat32_get_next_cluster(uint32_t cluster);
uint32_t fat32_get_free_cluster(void);
int fat32_allocate_cluster(uint32_t current_cluster, uint32_t new_cluster);

// Utility
void fat32_filename_to_8_3(const char *filename, uint8_t *name_83);
void fat32_8_3_to_filename(uint8_t *name_83, char *filename);

#endif
