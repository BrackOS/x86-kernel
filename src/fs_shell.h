#ifndef FS_SHELL_H
#define FS_SHELL_H

#include <stdint.h>
#include "fat32.h"

#define MAX_PATH_LEN 256
#define MAX_DIR_STACK 16

/* Current working directory state */
typedef struct {
    uint32_t cluster;
    char path[MAX_PATH_LEN];
    uint32_t cluster_stack[MAX_DIR_STACK];
    int depth;
} cwd_t;

/* Filesystem shell commands */
void fs_shell_init(void);
void fs_shell_loop(void);

/* Directory navigation */
void fs_cd(const char *path);
void fs_ls(const char *path);

/* File operations */
void fs_mkdir(const char *name);
void fs_nf(const char *name);
void fs_rm(const char *name);
void fs_rmdir(const char *name);

/* Path handling */
void fs_parse_path(const char *input, char *abs_path);
void fs_get_cwd(char *path);

/* Global state */
extern cwd_t current_dir;

#endif
