#ifndef PAPER_H
#define PAPER_H

#include <stdint.h>

#define PAPER_MAX_LINES     256
#define PAPER_MAX_LINE_LEN  80     /* 79 visible characters + terminator */

/* Open a file in the paper editor. dir_cluster is the directory it lives in.
 * ESC saves the file and returns to the shell. */
void paper_open_file(const char *name, uint32_t dir_cluster);

#endif
