#include "fs_shell.h"
#include "vga.h"
#include "fat32.h"
#include "string.h"
#include <stdint.h>

cwd_t current_dir = {0};
static uint8_t sector_buffer[512];

void fs_shell_init(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_print("BrackOS v1.5 Filesystem Shell\n");
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print("================================\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Initialize to root (cluster 2 is FAT32 root) */
    current_dir.cluster = fat32_fs.root_dir_cluster;
    strcpy(current_dir.path, "A:/");
    current_dir.depth = 0;
    
    vga_print("Welcome to A: drive\n");
    vga_print("Type 'help' for commands\n\n");
}

void fs_get_cwd(char *path) {
    strcpy(path, current_dir.path);
}

/* Format filename from FAT32 8.3 to readable format */
void format_filename(uint8_t *name_83, char *output) {
    int i = 0, j = 0;
    
    /* Copy name part, skip trailing spaces */
    for (i = 0; i < 8; i++) {
        if (name_83[i] != ' ') {
            output[j++] = name_83[i];
        }
    }
    
    /* Add extension if present */
    if (name_83[8] != ' ') {
        output[j++] = '.';
        for (i = 8; i < 11; i++) {
            if (name_83[i] != ' ') {
                output[j++] = name_83[i];
            }
        }
    }
    
    output[j] = 0;
}

/* Convert filename to FAT32 8.3 format */
void to_fat32_name(const char *filename, uint8_t *name_83) {
    memset(name_83, ' ', 11);
    
    const char *dot = strchr(filename, '.');
    int name_len = dot ? (int)(dot - filename) : (int)strlen(filename);
    int ext_len = dot ? strlen(dot + 1) : 0;
    
    if (name_len > 8) name_len = 8;
    if (ext_len > 3) ext_len = 3;
    
    /* Copy name (uppercase) */
    for (int i = 0; i < name_len; i++) {
        name_83[i] = (filename[i] >= 'a' && filename[i] <= 'z') ? 
                     filename[i] - 32 : filename[i];
    }
    
    /* Copy extension (uppercase) */
    if (dot) {
        for (int i = 0; i < ext_len; i++) {
            name_83[8 + i] = (dot[i + 1] >= 'a' && dot[i + 1] <= 'z') ? 
                             dot[i + 1] - 32 : dot[i + 1];
        }
    }
}

/* List directory contents */
void fs_ls(const char *path) {
    uint32_t target_cluster = current_dir.cluster;
    
    /* If path is specified, navigate first */
    if (path && strlen(path) > 0 && strcmp(path, ".") != 0) {
        if (strcmp(path, "..") == 0) {
            if (current_dir.depth > 0) {
                current_dir.depth--;
                target_cluster = current_dir.cluster_stack[current_dir.depth];
            }
        } else {
            /* Search for directory in current folder */
            uint8_t name_83[11];
            to_fat32_name(path, name_83);
            
            /* Read directory entries */
            uint32_t sector = fat32_fs.data_start_sector + 
                             ((target_cluster - 2) * fat32_fs.boot.sectors_per_cluster);
            
            int found = 0;
            for (int s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
                if (fat32_read_sector(sector + s, sector_buffer) != 0) continue;
                
                for (int i = 0; i < 512; i += 32) {
                    uint8_t *ent = &sector_buffer[i];
                    
                    if (ent[0] == 0) break;
                    if (ent[0] == 0xE5 || ent[0] == 0x2E) continue;
                    
                    if (memcmp(ent, name_83, 11) == 0) {
                        uint8_t attr = ent[11];
                        if (attr & 0x10) { /* Directory */
                            uint32_t hi = (ent[21] << 8) | ent[20];
                            uint32_t lo = (ent[27] << 8) | ent[26];
                            uint32_t new_cluster = (hi << 16) | lo;
                            
                            current_dir.cluster_stack[current_dir.depth] = target_cluster;
                            current_dir.depth++;
                            target_cluster = new_cluster;
                            found = 1;
                        }
                        break;
                    }
                }
                if (found) break;
            }
            
            if (!found) {
                vga_print("Directory not found: ");
                vga_print(path);
                vga_print("\n");
                return;
            }
        }
    }
    
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print("Contents of A:/");
    if (current_dir.depth > 0) {
        for (int d = 0; d < current_dir.depth; d++) {
            vga_print("DIR/");
        }
    }
    vga_print(":\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((target_cluster - 2) * fat32_fs.boot.sectors_per_cluster);
    
    int count = 0;
    for (int s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
        if (fat32_read_sector(sector + s, sector_buffer) != 0) continue;
        
        for (int i = 0; i < 512; i += 32) {
            uint8_t *ent = &sector_buffer[i];
            
            if (ent[0] == 0) break;
            if (ent[0] == 0xE5) continue;
            if (ent[0] == 0x2E) continue; /* . and .. */
            
            uint8_t attr = ent[11];
            uint32_t size = (ent[31] << 24) | (ent[30] << 16) | 
                           (ent[29] << 8) | ent[28];
            
            if (attr & 0x10) { /* Directory */
                vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
                vga_print("[DIR] ");
            } else { /* File */
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_print("[FILE] ");
            }
            
            char fname[13];
            format_filename(ent, fname);
            vga_print(fname);
            
            if (!(attr & 0x10)) {
                vga_print(" (");
                vga_print_int(size);
                vga_print(" bytes)");
            }
            
            /* Print modification date from FAT32 entry (bytes 24-25) */
            uint16_t mod_date = (ent[25] << 8) | ent[24];
            uint16_t mod_time = (ent[23] << 8) | ent[22];
            vga_print(" [");
            uint16_t year = 1980 + (mod_date >> 9);
            uint8_t month = (mod_date >> 5) & 0x0F;
            uint8_t day = mod_date & 0x1F;
            uint8_t hour = (mod_time >> 11) & 0x1F;
            uint8_t minute = (mod_time >> 5) & 0x3F;
            vga_print_int(year);
            vga_putchar('-');
            if (month < 10) vga_putchar('0');
            vga_print_int(month);
            vga_putchar('-');
            if (day < 10) vga_putchar('0');
            vga_print_int(day);
            vga_putchar(' ');
            if (hour < 10) vga_putchar('0');
            vga_print_int(hour);
            vga_putchar(':');
            if (minute < 10) vga_putchar('0');
            vga_print_int(minute);
            vga_print("]");
            vga_print("\n");
            
            count++;
        }
    }
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("\nTotal: ");
    vga_print_int(count);
    vga_print(" entries\n");
}

/* Change directory */
void fs_cd(const char *path) {
    if (!path || strlen(path) == 0) {
        vga_print("Usage: cd <directory>\n");
        return;
    }
    
    if (strcmp(path, "..") == 0) {
        if (current_dir.depth > 0) {
            current_dir.depth--;
            current_dir.cluster = current_dir.cluster_stack[current_dir.depth];
        } else {
            vga_print("Already at root\n");
        }
        return;
    }
    
    if (strcmp(path, "/") == 0 || strcmp(path, "A:/") == 0) {
        current_dir.cluster = fat32_fs.root_dir_cluster;
        current_dir.depth = 0;
        strcpy(current_dir.path, "A:/");
        return;
    }
    
    /* Search for subdirectory */
    uint8_t name_83[11];
    to_fat32_name(path, name_83);
    
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((current_dir.cluster - 2) * fat32_fs.boot.sectors_per_cluster);
    
    for (int s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
        if (fat32_read_sector(sector + s, sector_buffer) != 0) continue;
        
        for (int i = 0; i < 512; i += 32) {
            uint8_t *ent = &sector_buffer[i];
            
            if (ent[0] == 0) break;
            if (ent[0] == 0xE5 || ent[0] == 0x2E) continue;
            
            if (memcmp(ent, name_83, 11) == 0) {
                uint8_t attr = ent[11];
                if (attr & 0x10) { /* Directory */
                    uint32_t hi = (ent[21] << 8) | ent[20];
                    uint32_t lo = (ent[27] << 8) | ent[26];
                    uint32_t new_cluster = (hi << 16) | lo;
                    
                    current_dir.cluster_stack[current_dir.depth] = current_dir.cluster;
                    current_dir.depth++;
                    current_dir.cluster = new_cluster;
                    return;
                } else {
                    vga_print("Not a directory: ");
                    vga_print(path);
                    vga_print("\n");
                    return;
                }
            }
        }
    }
    
    vga_print("Directory not found: ");
    vga_print(path);
    vga_print("\n");
}

/* Create directory */
void fs_mkdir(const char *name) {
    if (!name || strlen(name) == 0) {
        vga_print("Usage: mkdir <name>\n");
        return;
    }
    
    vga_print("Creating directory: ");
    vga_print(name);
    vga_print("\n");
    
    /* Get free cluster */
    uint32_t new_cluster = fat32_get_free_cluster();
    if (new_cluster == 0xFFFFFFFF || fat32_allocate_cluster(0, new_cluster) != 0) {
        vga_print("Error: No free clusters\n");
        return;
    }
    
    /* Create directory entry */
    uint8_t name_83[11];
    to_fat32_name(name, name_83);
    
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((current_dir.cluster - 2) * fat32_fs.boot.sectors_per_cluster);
    
    int written = 0;
    for (int s = 0; s < fat32_fs.boot.sectors_per_cluster && !written; s++) {
        if (fat32_read_sector(sector + s, sector_buffer) != 0) continue;
        
        for (int i = 0; i < 512; i += 32) {
            uint8_t *ent = &sector_buffer[i];
            
            if (ent[0] == 0 || ent[0] == 0xE5) { /* Free slot */
                /* Write directory entry */
                memcpy(ent, name_83, 11);
                ent[11] = 0x10; /* Directory attribute */
                ent[12] = 0;    /* Reserved */
                ent[13] = 0;    /* Creation time tenth */
                
                uint16_t now = 0x2100; /* 33:00 (placeholder) */
                uint16_t today = 0x2815; /* 2026-10-05 */
                
                ent[14] = now & 0xFF;
                ent[15] = (now >> 8) & 0xFF;
                ent[16] = today & 0xFF;
                ent[17] = (today >> 8) & 0xFF;
                ent[18] = today & 0xFF;
                ent[19] = (today >> 8) & 0xFF;
                
                /* High cluster word */
                ent[20] = (new_cluster >> 16) & 0xFF;
                ent[21] = (new_cluster >> 24) & 0xFF;
                
                /* Write time/date */
                ent[22] = now & 0xFF;
                ent[23] = (now >> 8) & 0xFF;
                ent[24] = today & 0xFF;
                ent[25] = (today >> 8) & 0xFF;
                
                /* Low cluster word */
                ent[26] = new_cluster & 0xFF;
                ent[27] = (new_cluster >> 8) & 0xFF;
                
                /* Size = 0 for directories */
                ent[28] = 0;
                ent[29] = 0;
                ent[30] = 0;
                ent[31] = 0;
                
                if (fat32_write_sector(sector + s, sector_buffer) == 0) {
                    vga_print("Directory created successfully\n");
                    written = 1;
                } else {
                    vga_print("Error writing directory entry\n");
                }
                break;
            }
        }
    }
    
    if (!written) {
        vga_print("Error: Directory full\n");
    }
}

/* Create new file */
void fs_nf(const char *name) {
    if (!name || strlen(name) == 0) {
        vga_print("Usage: nf <filename>\n");
        return;
    }

    dir_entry_t existing;
    if (fat32_find_in_dir(current_dir.cluster, name, &existing) == 0) {
        vga_print("Error: file already exists\n");
        return;
    }

    /* An empty write creates the directory entry with no clusters allocated */
    if (fat32_write_file(current_dir.cluster, name, 0, 0) == 0) {
        vga_print("File created successfully\n");
    } else {
        vga_print("Error: could not create file\n");
    }
}

/* Remove file */
void fs_rm(const char *name) {
    if (!name || strlen(name) == 0) {
        vga_print("Usage: rm <filename>\n");
        return;
    }

    int r = fat32_delete_file(current_dir.cluster, name);
    if (r == 0) {
        vga_print("File deleted\n");
    } else if (r == -2) {
        vga_print("Error: that is a directory (use rmdir)\n");
    } else {
        vga_print("Error: file not found\n");
    }
}

/* Remove directory */
void fs_rmdir(const char *name) {
    if (!name || strlen(name) == 0) {
        vga_print("Usage: rmdir <dirname>\n");
        return;
    }
    
    uint8_t name_83[11];
    to_fat32_name(name, name_83);
    
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((current_dir.cluster - 2) * fat32_fs.boot.sectors_per_cluster);
    
    for (int s = 0; s < fat32_fs.boot.sectors_per_cluster; s++) {
        if (fat32_read_sector(sector + s, sector_buffer) != 0) continue;
        
        for (int i = 0; i < 512; i += 32) {
            uint8_t *ent = &sector_buffer[i];
            
            if (ent[0] == 0) break;
            if (ent[0] == 0xE5 || ent[0] == 0x2E) continue;
            
            if (memcmp(ent, name_83, 11) == 0) {
                uint8_t attr = ent[11];
                if (!(attr & 0x10)) {
                    vga_print("Not a directory: ");
                    vga_print(name);
                    vga_print("\n");
                    return;
                }
                
                ent[0] = 0xE5; /* Mark as deleted */
                if (fat32_write_sector(sector + s, sector_buffer) == 0) {
                    vga_print("Directory deleted: ");
                    vga_print(name);
                    vga_print("\n");
                } else {
                    vga_print("Error deleting directory\n");
                }
                return;
            }
        }
    }
    
    vga_print("Directory not found: ");
    vga_print(name);
    vga_print("\n");
}
