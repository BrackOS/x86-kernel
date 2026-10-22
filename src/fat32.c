#include "fat32.h"
#include "vga.h"
#include "io.h"
#include "string.h"
#include <stdint.h>

/* Global FAT32 state */
fat32_fs_t fat32_fs = {0};
static uint8_t sector_buffer[SECTOR_SIZE];

/* ATA PIO disk registers (primary bus, master drive) */
#define IDE_DATA        0x1F0
#define IDE_ERROR       0x1F1
#define IDE_SECTOR_CNT  0x1F2
#define IDE_LBA_LOW     0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HIGH    0x1F5
#define IDE_DRIVE_HEAD  0x1F6
#define IDE_COMMAND     0x1F7
#define IDE_STATUS      0x1F7

#define IDE_CMD_READ    0x20
#define IDE_CMD_WRITE   0x30
#define IDE_CMD_FLUSH   0xE7

#define IDE_STATUS_ERR  0x01
#define IDE_STATUS_DRQ  0x08
#define IDE_STATUS_BSY  0x80

#define FAT_EOC_MIN     0x0FFFFFF8   /* clusters >= this end a chain */
#define FAT_EOC_MARK    0x0FFFFFFF   /* value written at the end of a chain */
#define FAT_FREE        0x00000000
#define FAT_NO_CLUSTER  0xFFFFFFFF

#define ATTR_ARCHIVE    0x20
#define DIR_ENTRY_SIZE  32

/* ---------- ATA disk I/O ---------- */

static int ide_wait_not_busy(void) {
    for (int t = 100000; t > 0; t--) {
        if (!(inb(IDE_STATUS) & IDE_STATUS_BSY)) return 0;
    }
    return -1;
}

static int ide_wait_drq(void) {
    for (int t = 100000; t > 0; t--) {
        uint8_t s = inb(IDE_STATUS);
        if (s & IDE_STATUS_ERR) return -1;
        if (!(s & IDE_STATUS_BSY) && (s & IDE_STATUS_DRQ)) return 0;
    }
    return -1;
}

static void ide_select(uint32_t sector) {
    outb(IDE_SECTOR_CNT, 1);
    outb(IDE_LBA_LOW,  sector & 0xFF);
    outb(IDE_LBA_MID,  (sector >> 8) & 0xFF);
    outb(IDE_LBA_HIGH, (sector >> 16) & 0xFF);
    outb(IDE_DRIVE_HEAD, 0xE0 | ((sector >> 24) & 0x0F));  /* LBA mode */
}

/* Read one 512-byte sector */
int fat32_read_sector(uint32_t sector, uint8_t *buffer) {
    if (ide_wait_not_busy() != 0) return -1;
    ide_select(sector);
    outb(IDE_COMMAND, IDE_CMD_READ);
    if (ide_wait_drq() != 0) return -1;

    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(IDE_DATA);
        buffer[i * 2]     = data & 0xFF;
        buffer[i * 2 + 1] = (data >> 8) & 0xFF;
    }
    return 0;
}

/* Write one 512-byte sector */
int fat32_write_sector(uint32_t sector, uint8_t *buffer) {
    if (ide_wait_not_busy() != 0) return -1;
    ide_select(sector);
    outb(IDE_COMMAND, IDE_CMD_WRITE);
    if (ide_wait_drq() != 0) return -1;

    for (int i = 0; i < 256; i++) {
        outw(IDE_DATA, (uint16_t)(buffer[i * 2] | (buffer[i * 2 + 1] << 8)));
    }

    /* Flush the drive's write cache so the data is really on disk */
    outb(IDE_COMMAND, IDE_CMD_FLUSH);
    if (ide_wait_not_busy() != 0) return -1;
    return 0;
}

/* ---------- Cluster and FAT helpers ---------- */

static uint32_t cluster_to_sector(uint32_t cluster) {
    return fat32_fs.data_start_sector +
           (cluster - 2) * fat32_fs.boot.sectors_per_cluster;
}

/* Write a value into a cluster's FAT entry in every FAT copy */
static int fat_set(uint32_t cluster, uint32_t value) {
    uint8_t buf[SECTOR_SIZE];
    uint32_t off = cluster * 4;
    uint32_t idx = off / SECTOR_SIZE;
    uint32_t in_sector = off % SECTOR_SIZE;

    for (uint32_t f = 0; f < fat32_fs.boot.fats; f++) {
        uint32_t sec = fat32_fs.fat_start_sector + f * fat32_fs.boot.sectors_per_fat + idx;
        if (fat32_read_sector(sec, buf) != 0) return -1;
        value &= 0x0FFFFFFF;
        memcpy(&buf[in_sector], &value, 4);
        if (fat32_write_sector(sec, buf) != 0) return -1;
    }
    return 0;
}

/* Initialize FAT32 filesystem */
int fat32_init(uint32_t drive_lba) {
    if (fat32_read_sector(drive_lba, sector_buffer) != 0) {
        vga_print("FAT32: Failed to read boot sector\n");
        return -1;
    }

    /* Validate boot sector signature */
    if (sector_buffer[510] != 0x55 || sector_buffer[511] != 0xAA) {
        vga_print("FAT32: Invalid boot signature\n");
        return -1;
    }

    memcpy(&fat32_fs.boot, sector_buffer, sizeof(boot_sector_t));

    /* Sanity checks: this must look like a FAT32 volume */
    if (fat32_fs.boot.bytes_per_sector != SECTOR_SIZE ||
        fat32_fs.boot.sectors_per_cluster == 0 ||
        fat32_fs.boot.fats == 0 ||
        fat32_fs.boot.sectors_per_fat == 0 ||
        fat32_fs.boot.root_cluster < 2) {
        vga_print("FAT32: Not a FAT32 volume\n");
        return -1;
    }

    /* Calculate filesystem layout (all absolute sector numbers) */
    fat32_fs.fat_start_sector = drive_lba + fat32_fs.boot.reserved_sectors;
    fat32_fs.data_start_sector = fat32_fs.fat_start_sector +
                                 (fat32_fs.boot.fats * fat32_fs.boot.sectors_per_fat);
    fat32_fs.root_dir_cluster = fat32_fs.boot.root_cluster;

    vga_print("FAT32: Initialized (");
    vga_print_int(fat32_fs.boot.bytes_per_sector);
    vga_print(" bytes/sector, cluster=");
    vga_print_int(fat32_fs.boot.sectors_per_cluster);
    vga_print(" sectors)\n");

    return 0;
}

/* Get next cluster in FAT chain */
uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat32_fs.fat_start_sector + (fat_offset / SECTOR_SIZE);
    uint32_t offset_in_sector = fat_offset % SECTOR_SIZE;

    if (fat32_read_sector(fat_sector, sector_buffer) != 0) {
        return FAT_NO_CLUSTER;
    }

    uint32_t next_cluster;
    memcpy(&next_cluster, &sector_buffer[offset_in_sector], 4);
    return next_cluster & 0x0FFFFFFF;  /* Mask to 28 bits */
}

/* Get free cluster (simple linear search) */
uint32_t fat32_get_free_cluster(void) {
    for (uint32_t cluster = 2; cluster < FAT_EOC_MIN; cluster++) {
        if (fat32_get_next_cluster(cluster) == FAT_FREE) {
            return cluster;
        }
    }
    return FAT_NO_CLUSTER;
}

/* Link current_cluster -> new_cluster and mark new_cluster as end of chain */
int fat32_allocate_cluster(uint32_t current_cluster, uint32_t new_cluster) {
    if (new_cluster == FAT_NO_CLUSTER) return -1;
    if (fat_set(new_cluster, FAT_EOC_MARK) != 0) return -1;
    if (current_cluster >= 2 && fat_set(current_cluster, new_cluster) != 0) return -1;
    return 0;
}

/* Free every cluster in a chain (used when a file is overwritten) */
static void free_chain(uint32_t cluster) {
    while (cluster >= 2 && cluster < FAT_EOC_MIN) {
        uint32_t next = fat32_get_next_cluster(cluster);
        fat_set(cluster, FAT_FREE);
        cluster = next;
    }
}

/* ---------- Directory helpers ---------- */

/* Find an entry by its 8.3 name. Returns 1 if found and sets sector and offset.
 * If not found, returns 0 and sets sector and offset to the first free slot
 * (offset is -1 if the directory is full). last receives the last cluster in the chain. */
static int dir_slot(uint32_t dir_cluster, const uint8_t name83[11],
                    uint32_t *sector, int *offset, uint32_t *last) {
    uint32_t cluster = dir_cluster;
    int have_free = 0;
    uint32_t free_sector = 0;
    int free_offset = 0;

    *last = dir_cluster;
    while (cluster >= 2 && cluster < FAT_EOC_MIN) {
        *last = cluster;
        for (uint32_t s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
            uint32_t sec = cluster_to_sector(cluster) + s;
            if (fat32_read_sector(sec, sector_buffer) != 0) return -1;

            for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
                dir_entry_t *ent = (dir_entry_t *)&sector_buffer[i];
                if (ent->filename[0] == 0x00 || ent->filename[0] == 0xE5) {
                    if (!have_free) {
                        have_free = 1;
                        free_sector = sec;
                        free_offset = i;
                    }
                    if (ent->filename[0] == 0x00) goto done_scanning;
                    continue;
                }
                if (memcmp(ent->filename, name83, 11) == 0) {
                    *sector = sec;
                    *offset = i;
                    return 1;
                }
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }

done_scanning:
    if (have_free) {
        *sector = free_sector;
        *offset = free_offset;
        return 0;
    }
    *offset = -1;
    return 0;
}

/* Write a directory entry for name83, adding a cluster to the directory if full */
static int dir_store(uint32_t dir_cluster, const uint8_t name83[11],
                     uint32_t first_cluster, uint32_t size) {
    uint32_t sector, last;
    int offset;
    int found = dir_slot(dir_cluster, name83, &sector, &offset, &last);
    if (found < 0) return -1;

    if (offset < 0) {
        /* Directory is full: chain on a fresh, zeroed cluster */
        uint32_t nc = fat32_get_free_cluster();
        if (nc == FAT_NO_CLUSTER) return -1;
        if (fat32_allocate_cluster(last, nc) != 0) return -1;

        uint8_t zero[SECTOR_SIZE];
        memset(zero, 0, SECTOR_SIZE);
        for (uint32_t s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
            if (fat32_write_sector(cluster_to_sector(nc) + s, zero) != 0) return -1;
        }
        sector = cluster_to_sector(nc);
        offset = 0;
    }

    if (fat32_read_sector(sector, sector_buffer) != 0) return -1;
    dir_entry_t *e = (dir_entry_t *)&sector_buffer[offset];
    memset(e, 0, DIR_ENTRY_SIZE);
    memcpy(e->filename, name83, 11);
    e->attributes = ATTR_ARCHIVE;
    e->first_cluster_high = (uint16_t)(first_cluster >> 16);
    e->first_cluster_low  = (uint16_t)(first_cluster & 0xFFFF);
    e->file_size = size;
    return fat32_write_sector(sector, sector_buffer);
}

/* Find file in a directory by name. Returns 0 on success. */
int fat32_find_in_dir(uint32_t dir_cluster, const char *filename, dir_entry_t *entry) {
    uint8_t search_name[11];
    uint32_t sector, last;
    int offset;

    fat32_filename_to_8_3(filename, search_name);
    if (dir_slot(dir_cluster, search_name, &sector, &offset, &last) != 1) {
        return -1;
    }
    if (fat32_read_sector(sector, sector_buffer) != 0) return -1;
    memcpy(entry, &sector_buffer[offset], sizeof(dir_entry_t));
    return 0;
}

/* Find file in the root directory */
int fat32_find_file(const char *filename, dir_entry_t *entry) {
    return fat32_find_in_dir(fat32_fs.root_dir_cluster, filename, entry);
}

/* List files in directory */
int fat32_list_directory(uint32_t cluster, dir_entry_t *entries, int max_entries) {
    int count = 0;

    while (cluster >= 2 && cluster < FAT_EOC_MIN && count < max_entries) {
        for (uint32_t s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
            if (fat32_read_sector(cluster_to_sector(cluster) + s, sector_buffer) != 0) {
                continue;
            }

            for (int i = 0; i < SECTOR_SIZE && count < max_entries; i += DIR_ENTRY_SIZE) {
                dir_entry_t *ent = (dir_entry_t *)&sector_buffer[i];

                if (ent->filename[0] == 0) {
                    return count;  /* End of directory */
                }
                if (ent->filename[0] == 0xE5 || ent->filename[0] == 0x2E) {
                    continue;      /* Deleted entry or . / .. */
                }
                memcpy(&entries[count], ent, sizeof(dir_entry_t));
                count++;
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }

    return count;
}

/* ---------- File read / write ---------- */

/* Read up to cap bytes of a file. Returns bytes read, or -1 on error. */
int fat32_read_file(uint32_t dir_cluster, const char *name, uint8_t *buf, uint32_t cap) {
    dir_entry_t e;
    if (fat32_find_in_dir(dir_cluster, name, &e) != 0) return -1;

    uint32_t first = ((uint32_t)e.first_cluster_high << 16) | e.first_cluster_low;
    uint32_t limit = (e.file_size < cap) ? e.file_size : cap;
    uint32_t done = 0;
    uint32_t cluster = first;
    uint8_t tmp[SECTOR_SIZE];

    while (cluster >= 2 && cluster < FAT_EOC_MIN && done < limit) {
        for (uint32_t s = 0; s < fat32_fs.boot.sectors_per_cluster && done < limit; s++) {
            if (fat32_read_sector(cluster_to_sector(cluster) + s, tmp) != 0) return -1;
            for (uint32_t i = 0; i < SECTOR_SIZE && done < limit; i++) {
                buf[done++] = tmp[i];
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }
    return (int)done;
}

/* Create or overwrite a file in a directory. Returns 0 on success. */
int fat32_write_file(uint32_t dir_cluster, const char *name,
                     const uint8_t *data, uint32_t size) {
    uint8_t name83[11];
    uint32_t sector, last;
    int offset;
    fat32_filename_to_8_3(name, name83);

    /* Release the old data first so its clusters can be reused */
    if (dir_slot(dir_cluster, name83, &sector, &offset, &last) == 1) {
        if (fat32_read_sector(sector, sector_buffer) != 0) return -1;
        dir_entry_t *old = (dir_entry_t *)&sector_buffer[offset];
        free_chain(((uint32_t)old->first_cluster_high << 16) | old->first_cluster_low);
    }

    /* Build the new cluster chain and write the data */
    uint32_t first = 0, prev = 0, done = 0;
    uint32_t spc = fat32_fs.boot.sectors_per_cluster;

    while (done < size) {
        uint32_t c = fat32_get_free_cluster();
        if (c == FAT_NO_CLUSTER) return -1;   /* disk full */
        if (fat32_allocate_cluster(prev, c) != 0) return -1;
        if (prev == 0) first = c;

        for (uint32_t s = 0; s < spc; s++) {
            uint8_t buf[SECTOR_SIZE];
            for (uint32_t i = 0; i < SECTOR_SIZE; i++) {
                uint32_t pos = done + s * SECTOR_SIZE + i;
                buf[i] = (pos < size) ? data[pos] : 0;
            }
            if (fat32_write_sector(cluster_to_sector(c) + s, buf) != 0) return -1;
        }
        done += spc * SECTOR_SIZE;
        prev = c;
    }

    return dir_store(dir_cluster, name83, first, size);
}

/* Delete a file: free its clusters and mark its directory entry deleted.
 * Returns 0 on success, -1 if not found or on error, -2 if it is a directory. */
int fat32_delete_file(uint32_t dir_cluster, const char *name) {
    uint8_t name83[11];
    uint32_t sector, last;
    int offset;
    fat32_filename_to_8_3(name, name83);

    if (dir_slot(dir_cluster, name83, &sector, &offset, &last) != 1) return -1;
    if (fat32_read_sector(sector, sector_buffer) != 0) return -1;

    dir_entry_t *e = (dir_entry_t *)&sector_buffer[offset];
    if (e->attributes & 0x10) return -2;   /* directory */
    uint32_t first = ((uint32_t)e->first_cluster_high << 16) | e->first_cluster_low;

    free_chain(first);   /* overwrites sector_buffer, so re-read below */

    if (fat32_read_sector(sector, sector_buffer) != 0) return -1;
    e = (dir_entry_t *)&sector_buffer[offset];
    e->filename[0] = 0xE5;
    return fat32_write_sector(sector, sector_buffer);
}

/* ---------- Filename conversion ---------- */

/* Convert filename to FAT 8.3 format (uppercase, space padded) */
void fat32_filename_to_8_3(const char *filename, uint8_t *name_83) {
    memset(name_83, ' ', 11);

    const char *dot = strchr(filename, '.');
    int name_len = dot ? (int)(dot - filename) : (int)strlen(filename);
    int ext_len = dot ? (int)strlen(dot + 1) : 0;

    if (name_len > 8) name_len = 8;
    if (ext_len > 3) ext_len = 3;

    for (int i = 0; i < name_len; i++) {
        name_83[i] = (filename[i] >= 'a' && filename[i] <= 'z') ?
                     filename[i] - 32 : filename[i];
    }

    if (dot) {
        for (int i = 0; i < ext_len; i++) {
            name_83[8 + i] = (dot[i + 1] >= 'a' && dot[i + 1] <= 'z') ?
                             dot[i + 1] - 32 : dot[i + 1];
        }
    }
}

/* Convert 8.3 FAT32 filename to normal format */
void fat32_8_3_to_filename(uint8_t *name_83, char *filename) {
    int i, j = 0;

    for (i = 0; i < 8; i++) {
        if (name_83[i] != ' ') {
            filename[j++] = name_83[i];
        }
    }

    if (name_83[8] != ' ') {
        filename[j++] = '.';
        for (i = 8; i < 11; i++) {
            if (name_83[i] != ' ') {
                filename[j++] = name_83[i];
            }
        }
    }

    filename[j] = 0;
}
