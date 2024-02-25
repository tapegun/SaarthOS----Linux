/*
 * Functions for processing user input
 * from the keyboard
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <types.h>

/* reads and interprets keyboard input based on scan set */
void handle_keyboard(void);
/* initialize keyboard IRQ */
void init_keyboard(void);
/* get last key pressed */
int8_t keyboard_get_key();

#define KBD_IRQ         1

#define COMMAND         0x64
#define STATUS          0x64
#define DATA            0x60

#define READ_CONFIG     0x20
#define WRITE_CONFIG    0x60

#define TRANSLATION_BIT 6

#define SCANSET         1

#define EXTENDED_CODE   0xe0
#define RELEASE_CODE    0x80

#define LSHIFT          0x2a
#define RSHIFT          0x36
#define LCTRL           0x1d
#define LALT            0x38
#define CAPS            0x3a

#endif /* KEYBOARD_H */
