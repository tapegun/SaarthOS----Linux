/*
 * Create an exception in IDT by filling out
 * the struct values accordingly.
 * 
 * SET_IDT_ENTRY defined in x86_desc
 * IDT entries filled from:
 *      Intel Doc (Figure 5-2)
 *      https://wiki.osdev.org/Interrupt_Descriptor_Table
 */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#define add_exception(n, addr)          \
do {                                    \
    SET_IDT_ENTRY(idt[n], addr);        \
    idt[n].seg_selector = KERNEL_CS;    \
    idt[n].reserved3 = 1;               \
    idt[n].reserved2 = 1;               \
    idt[n].reserved1 = 1;               \
    idt[n].size = 1;                    \
    idt[n].reserved0 = 0;               \
    idt[n].dpl = 0;                     \
    idt[n].present = 1;                 \
} while (0)


/* exception addresses in IDT */
#define EXC_ADDR_DIV_ZERO       0x00
#define EXC_ADDR_DEBUG          0x01
#define EXC_ADDR_NMI            0x02
#define EXC_ADDR_BREAKPOINT     0x03
#define EXC_ADDR_OVERFLOW       0x04
#define EXC_ADDR_BOUND_EX       0x05
#define EXC_ADDR_INVALID_OP     0x06
#define EXC_ADDR_DEVICE_NA      0x07
#define EXC_ADDR_DOUBLEF        0x08
#define EXC_ADDR_COP_SEG_OVR    0x09
#define EXC_ADDR_TSS_INV        0x0A
#define EXC_ADDR_SEG_NA         0x0B
#define EXC_ADDR_STACK_SEGF     0x0C
#define EXC_ADDR_GEN_PROTF      0x0D
#define EXC_ADDR_PAGEF          0x0E
#define EXC_ADDR_RESERVEF       0x0F
#define EXC_ADDR_FPUF           0x10
#define EXC_ADDR_ALIGN_CK       0x11
#define EXC_ADDR_MACHINE_CK     0x12
#define EXC_ADDR_SIMD_EX        0x13



/* Populate the IDT with exceptions */
void init_idt_exception(void);

/*
 * how the OS handles/reports exceptions
 * will initiate a while(1); loop for now
 */
void do_exception(int n);

#endif /* EEXCEPTIONS_H */
