#include <stddef.h>
#include "mouse.h"
#include "io.h"

// PS/2 Controller ports
#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

// PS/2 Status flags
#define PS2_OUTPUT_BUFFER_FULL 0x01
#define PS2_INPUT_BUFFER_FULL  0x02

// PS/2 Mouse commands
#define MOUSE_ENABLE_STREAM    0xF4
#define MOUSE_DISABLE_STREAM   0xF5
#define MOUSE_RESET            0xFF
#define MOUSE_SET_DEFAULTS     0xF6
#define MOUSE_SET_RESOLUTION   0xE8
#define MOUSE_SET_SAMPLE_RATE  0xF3

// PS/2 Mouse responses
#define MOUSE_ACK              0xFA
#define MOUSE_NACK             0xFE
#define MOUSE_RESEND           0xFE

mouse_state_t mouse_state = {0};
mouse_driver_t *active_mouse_driver = NULL;

// Forward declarations
void ps2_mouse_init(void);
void ps2_mouse_update(void);
mouse_state_t* ps2_mouse_get_state(void);
const char* ps2_mouse_get_name(void);

// PS/2 Mouse driver structure
mouse_driver_t ps2_mouse_driver = {
    .init = ps2_mouse_init,
    .update = ps2_mouse_update,
    .get_state = ps2_mouse_get_state,
    .get_name = ps2_mouse_get_name,
};

// Helper: Wait for data to be available
static void ps2_wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS_PORT) & PS2_OUTPUT_BUFFER_FULL) {
            return;
        }
    }
}

// Helper: Wait for input buffer to be ready
static void ps2_wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS_PORT) & PS2_INPUT_BUFFER_FULL)) {
            return;
        }
    }
}

// Helper: Read byte from PS/2
static uint8_t ps2_read_byte(void) {
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

// Helper: Write byte to PS/2
static void ps2_write_byte(uint8_t byte) {
    ps2_wait_write();
    outb(PS2_DATA_PORT, byte);
}

// Helper: Send command and wait for ACK
static int ps2_send_command(uint8_t cmd) {
    ps2_write_byte(cmd);
    uint8_t response = ps2_read_byte();
    return (response == MOUSE_ACK) ? 0 : -1;
}

// Initialize PS/2 mouse
void ps2_mouse_init(void) {
    // Reset mouse
    if (ps2_send_command(MOUSE_RESET) != 0) {
        return;
    }
    
    // Wait for self-test
    uint8_t test_result = ps2_read_byte();
    (void)test_result;  // Suppress unused warning
    
    // Set to defaults
    ps2_send_command(MOUSE_SET_DEFAULTS);
    ps2_read_byte();  // Read response
    
    // Set sample rate (100 Hz)
    ps2_send_command(MOUSE_SET_SAMPLE_RATE);
    ps2_read_byte();
    ps2_write_byte(100);
    ps2_read_byte();
    
    // Set resolution (1 count per mm)
    ps2_send_command(MOUSE_SET_RESOLUTION);
    ps2_read_byte();
    ps2_write_byte(0);  // 0 = 1 count per mm
    ps2_read_byte();
    
    // Enable data reporting
    ps2_send_command(MOUSE_ENABLE_STREAM);
    ps2_read_byte();
    
    // Initialize state
    mouse_state.x = 512;  // Center of 1024×768
    mouse_state.y = 384;
    mouse_state.buttons = 0;
}

// Update mouse state from PS/2 packets
void ps2_mouse_update(void) {
    // Check if data available
    if (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_BUFFER_FULL)) {
        return;
    }
    
    // Read 3-byte packet
    static uint8_t packet[3];
    static int packet_index = 0;
    
    uint8_t byte = inb(PS2_DATA_PORT);
    
    // First byte should have bit 3 set (always 1)
    if (packet_index == 0 && !(byte & 0x08)) {
        return;  // Discard invalid packet start
    }
    
    packet[packet_index] = byte;
    packet_index++;
    
    if (packet_index < 3) {
        return;  // Wait for full packet
    }
    
    packet_index = 0;  // Reset for next packet
    
    // Parse packet
    uint8_t status = packet[0];
    int8_t dx = (int8_t)packet[1];
    int8_t dy = (int8_t)packet[2];
    
    // Update button states
    mouse_state.buttons = status & 0x07;
    mouse_state.left_pressed = (status & 0x01) ? 1 : 0;
    mouse_state.right_pressed = (status & 0x02) ? 1 : 0;
    mouse_state.middle_pressed = (status & 0x04) ? 1 : 0;
    
    // Update relative movement
    mouse_state.dx = dx;
    mouse_state.dy = dy;
    
    // Update absolute position with bounds checking
    int new_x = mouse_state.x + dx;
    int new_y = mouse_state.y + dy;
    
    // Clamp to screen bounds (1024×768)
    if (new_x < 0) new_x = 0;
    if (new_x >= 1024) new_x = 1023;
    if (new_y < 0) new_y = 0;
    if (new_y >= 768) new_y = 767;
    
    mouse_state.x = new_x;
    mouse_state.y = new_y;
}

// Get current mouse state
mouse_state_t* ps2_mouse_get_state(void) {
    return &mouse_state;
}

// Get driver name
const char* ps2_mouse_get_name(void) {
    return "PS/2 Mouse";
}

// Universal mouse interface (can be switched to USB later)
void mouse_init(void) {
    active_mouse_driver = &ps2_mouse_driver;
    active_mouse_driver->init();
}

void mouse_update(void) {
    if (active_mouse_driver) {
        active_mouse_driver->update();
    }
}

mouse_state_t* mouse_get(void) {
    return &mouse_state;
}

const char* mouse_get_driver_name(void) {
    if (active_mouse_driver) {
        return active_mouse_driver->get_name();
    }
    return "No mouse";
}
