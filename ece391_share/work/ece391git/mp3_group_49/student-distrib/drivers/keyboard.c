#include "keyboard.h"

#include <lib.h>
#include <interrupts/i8259.h>

/* KEYBOARD FUNCTIONS */

static int is_extended = 0;     /*  */
static int is_released = 0;     /* key up */
static int is_lcontrol = 0;     /* ctrl key check */
static int is_lshift = 0;       /* shift key checks */
static int is_rshift = 0;
static int is_caps = 0;         /* caps lock check */

static int8_t last_key;
static volatile int data_available = 0;

// The normal letters numbers and punctuation symbols in scan set 1
static const char normal_map[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08,
    0x09, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0x0a,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
    '7', '8', '9', '-',
    '4', '5', '6', '+',
    '1', '2', '3',
    '0', '.',
    0, 0, 0, 0, 0, 0, 0, 0,
};

/* scan set 1 with shift pressed */
static const char shift_map[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',  '_', '+', 0x08,
    0x09, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a,
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
    '7', '8', '9', '-',
    '4', '5', '6', '+',
    '1', '2', '3',
    '0', '.',
    0, 0, 0, 0, 0, 0, 0, 0,
};


/* handle_keyboard
 * Inputs: None
 * Outputs: none
 * Return Value: 0 on success
 * Function: reads input from keyboard and interprets values baseed on toggle keys
 */
void handle_keyboard() {
    unsigned char scancode;

    /* receive keyboard activity */
    scancode = inb(DATA);
    send_eoi(KBD_IRQ);

    /* determine activity type */
    if (scancode & RELEASE_CODE) {
        is_released = 1;    // don't return since there isn't a second code
    } else if (scancode == EXTENDED_CODE) {
        is_extended = 1;
        return;
    }

    /* key up event for special keys */
    if (is_released) {
        switch (scancode ^ RELEASE_CODE) {
            case LCTRL:
                is_lcontrol = 0;
                break;
            case LSHIFT:
                is_lshift = 0;
                break;
            case RSHIFT:
                is_rshift = 0;
                break;
        }
        is_released = 0;
        return;
    }

    if (is_extended) {
        is_extended = 0;
        return;
    }

    if (!is_extended && !is_released) {
        if  (normal_map[scancode]) {
            if (is_lcontrol){
                /* CTRL + L clears the screen */
                if (normal_map[scancode] == 'l') {
                    int x = get_cursor_x();
                    int y = get_cursor_y();
                    for (; y > 0; y--) {
                        scroll_one_unit_down();
                    }
                    set_cursor_position(x, 0);
                }
            } else {
                if (is_caps && (normal_map[scancode] >= 'a' && normal_map[scancode] <= 'z')){
                    /* capitalize chars */
                    if (is_lshift || is_rshift){
                        /* caps and shift = default */
                        last_key = normal_map[scancode];
                    }else{
                        /* caps and no shift = capitalize */
                        last_key = normal_map[scancode]-('a'-'A');
                    }
                }else if (is_lshift || is_rshift){
                    /* just shift */
                    last_key = shift_map[scancode];
                }else{
                    last_key = normal_map[scancode];
                }
                data_available = 1;
            }
        }
        else {
            /* key down event for special keys */
            switch (scancode) {
                case LCTRL:
                    is_lcontrol = 1;
                    break;
                case LSHIFT:
                    is_lshift = 1;
                    break;
                case RSHIFT:
                    is_rshift = 1;
                    break;
                case CAPS:
                    is_caps = !is_caps;
                    break;
            }
        }
    }
}

/* keyboard_get_key
 * Inputs: None
 * Outputs: none
 * Return Value: the last key inputted
 * Side effects: waits for a key to be pressed
 * Function: returns the last key that was pressed
 */
int8_t keyboard_get_key() {
    /* wait for key press */
    while (!data_available);
    /* clear flag and return */
    data_available = 0;
    return last_key;
}

/* init_keyboard
 * Inputs: None
 * Outputs: none
 * Return Value: void
 * Function: enables the interrupt request for keyboard
 */
void init_keyboard() {
    enable_irq(KBD_IRQ);
}
