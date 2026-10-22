#ifndef RECOVERY_H
#define RECOVERY_H

#include <stdint.h>

/* Recovery modes */
typedef enum {
    RECOVERY_NONE = 0,
    RECOVERY_SAFE_MODE = 1,
    RECOVERY_DIAGNOSTIC = 2,
    RECOVERY_REPAIR = 3
} recovery_mode_t;

/* Recovery state */
typedef struct {
    recovery_mode_t mode;
    uint8_t boot_count;
    uint8_t last_error;
    uint32_t reserved;
} recovery_state_t;

/* Boot diagnostics */
void recovery_init(void);
recovery_mode_t recovery_get_mode(void);
void recovery_run_diagnostics(void);
void recovery_safe_mode(void);
void recovery_log_error(uint8_t error_code);

/* Error codes */
#define RECOVERY_ERR_NONE      0x00
#define RECOVERY_ERR_GDT       0x01
#define RECOVERY_ERR_IDT       0x02
#define RECOVERY_ERR_PAGING    0x03
#define RECOVERY_ERR_FB        0x04
#define RECOVERY_ERR_KEYBOARD  0x05
#define RECOVERY_ERR_MOUSE     0x06
#define RECOVERY_ERR_FILESYSTEM 0x07

#endif
