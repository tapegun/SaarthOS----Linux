/*
 * Create an interrupt in IDT by filling out
 * the struct values accordingly.
 * 
 * SET_IDT_ENTRY defined in x86_desc
 * IDT entries filled from:
 *      Intel Doc (Figure 5-2)
 *      https://wiki.osdev.org/Interrupt_Descriptor_Table#I386_Interrupt_Gate
 * 
 * interrupts.* is analogous to exceptions.*
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#define add_interrupt(n, addr)          \
do {                                    \
    SET_IDT_ENTRY(idt[n], addr);        \
    idt[n].seg_selector = KERNEL_CS;    \
    idt[n].reserved3 = 0;               \
    idt[n].reserved2 = 1;               \
    idt[n].reserved1 = 1;               \
    idt[n].size = 1;                    \
    idt[n].reserved0 = 0;               \
    idt[n].dpl = 0;                     \
    idt[n].present = 1;                 \
} while (0)

#define add_sys_call(n, addr)           \
do {                                    \
    SET_IDT_ENTRY(idt[n], addr);        \
    idt[n].seg_selector = KERNEL_CS;    \
    idt[n].reserved3 = 0;               \
    idt[n].reserved2 = 1;               \
    idt[n].reserved1 = 1;               \
    idt[n].size = 1;                    \
    idt[n].reserved0 = 0;               \
    idt[n].dpl = 3;                     \
    idt[n].present = 1;                 \
} while (0)

/* IDT addresses for interrupts */
#define INTR_ADDR_START         0x20
#define INTR_ADDR_KEYB          0x21
#define INTR_ADDR_RTC           0x28

#define SYSCALL_VEC             0x80

/* Populate the IDT with interrupts */
void init_idt_interrupts(void);

/*
 * how the OS handles/reports exceptions
 * will initiate a while(1); loop for now
 */
void do_IRQ(int irqn);

#endif /* INTERRUPTS_H */
