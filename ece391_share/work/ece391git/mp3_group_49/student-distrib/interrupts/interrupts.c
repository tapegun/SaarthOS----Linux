#include "interrupts.h"

#include <x86_desc.h>
#include <lib.h>
#include <drivers/terminal.h>
#include <drivers/keyboard.h>
#include <drivers/rtc.h>
#include <interrupts/i8259.h>

/*
 * Exceptions: The following is a list of interrupts in IDT
 * Implementation of these assembly functions can be found in Handlers.S.
 */
void keyboard_interrupt(void);
void rtc_interrupt (void);

void sys_call(void);

/*
 * do_IRQ
 * DESCRIPTION: handle all interrupts passed in from
 *              common exception handler in handlers.S
 * INPUTS: irqn -- can be mapped to irq address
 * OUTPUTS: name of exception
 * RETURN VALUE: none
 * SIDE EFFECTS: kernel gets stuck in some sort of
 *               "blue screen of death"
 */
void do_IRQ(int irqn){
    /* 
     * recall that handlers.S sends
     * -1 * <offset from 0x20> of irq
     */
    irqn = INTR_ADDR_START - irqn; //map to irq addr
    /* select handler to redirect to */
    switch (irqn) {
        case INTR_ADDR_KEYB:
            handle_keyboard();
            break;
        case INTR_ADDR_RTC:
            handle_rtc();
            break;
        default:
            printf("Unhandled IRQ %d\n", irqn);
    }
}

/*
 * init_idt_interrupts
 * DESCRIPTION: adds interrupts and syscall to IDT
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void init_idt_interrupts(){
    /* add interrupts to IDT */
    add_interrupt(INTR_ADDR_KEYB, &keyboard_interrupt);
    add_interrupt(INTR_ADDR_RTC, &rtc_interrupt);
    add_sys_call(SYSCALL_VEC ,&sys_call);
}
