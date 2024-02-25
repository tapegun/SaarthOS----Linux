#include "exceptions.h"
#include "interrupts.h"

#include <x86_desc.h>
#include <lib.h>
#include <syscall/syscalls.h>


/*
 * Exceptions: The following is a list of all exceptions in IDT
 * Implementation of these assembly functions can be found in Handlers.S.
 */
void divide_error(void);
void debug(void);
void nmi(void);
void breakpoint(void);
void overflow(void);
void bound_exceed(void);
void invalid_opcode(void);
void device_not_available(void);
void double_fault(void);
void coprocessor_segment_overrun(void);
void tss_invalid(void);
void seg_not_present(void);
void stack_seg_fault(void);
void general_protection_fault(void);
void page_fault(void);
void reserved_fault(void);
void fpu_fault(void);
void alignment_check(void);
void machine_check(void);
void simd_exception(void);

void unhandled_exception(void);

void common_handler(void);


/*
 * do_exception
 * DESCRIPTION: handle all exceptions passed in from
 *              common exception handler in handlers.S
 * INPUTS: n -- exception number
 * OUTPUTS: name of exception
 * RETURN VALUE: none
 * SIDE EFFECTS: kernel gets stuck in some sort of
 *               "blue screen of death"
 */
void do_exception(int n) {
    // clear();
    switch (n){
        case EXC_ADDR_DIV_ZERO:
            printf("divide_error\n");
            break;
        case EXC_ADDR_DEBUG:
            printf("debug\n");
            break;
        case EXC_ADDR_NMI:
            printf("nmi\n");
            break;
        case EXC_ADDR_BREAKPOINT:
            printf("breakpoint\n");
            break;
        case EXC_ADDR_OVERFLOW:
            asm volatile ("int $0x0b"); // try another exception (seg not present)
            printf("overflow\n");
            break;
        case EXC_ADDR_BOUND_EX:
            printf("bound_exceeded\n");
            break;
        case EXC_ADDR_INVALID_OP:
            printf("invalid_opcode\n");
            break;
        case EXC_ADDR_DEVICE_NA:
            printf("device_not_available\n");
            break;
        case EXC_ADDR_DOUBLEF:
            printf("double_fault\n");
            break;
        case EXC_ADDR_COP_SEG_OVR:
            printf("coprocess_segment_overrun\n");
            break;
        case EXC_ADDR_TSS_INV:
            printf("invalid_tss\n");
            break;
        case EXC_ADDR_SEG_NA:
            printf("seg_not_present\n");
            break;
        case EXC_ADDR_STACK_SEGF:
            printf("stack_seg_fault\n");
            break;
        case EXC_ADDR_GEN_PROTF:
            printf("general_protection_fault\n");
            break;
        case EXC_ADDR_PAGEF:
            asm volatile("movl %cr2, %eax");
            printf("page_fault\n");
            break;
        case EXC_ADDR_RESERVEF:
            printf("reserved_fault\n");
            break;
        case EXC_ADDR_FPUF:
            printf("fpu_exception\n");
            break;
        case EXC_ADDR_ALIGN_CK:
            printf("alignment_check\n");
            break;
        case EXC_ADDR_MACHINE_CK:
            printf("machine_check\n");
            break;
        case EXC_ADDR_SIMD_EX:
            printf("simd_exception\n");
            break;
    }
    // blue screen of death placeholder
    // while(1);
    halt(0);
}


/*
 * init_idt_exception
 * DESCRIPTION: adds exceptions to IDT
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: populates IDT with exceptions
 */
void init_idt_exception(){
    // add exceptions to their IDT addresses
    add_exception(EXC_ADDR_DIV_ZERO, &divide_error);
    add_exception(EXC_ADDR_DEBUG, &debug);
    add_exception(EXC_ADDR_NMI, &nmi);
    add_exception(EXC_ADDR_BREAKPOINT, &breakpoint);
    add_exception(EXC_ADDR_OVERFLOW, &overflow);
    add_exception(EXC_ADDR_BOUND_EX, &bound_exceed);
    add_exception(EXC_ADDR_INVALID_OP, &invalid_opcode);
    add_exception(EXC_ADDR_DEVICE_NA, &device_not_available);
    add_exception(EXC_ADDR_DOUBLEF, &double_fault);
    add_exception(EXC_ADDR_COP_SEG_OVR, &coprocessor_segment_overrun);
    add_exception(EXC_ADDR_TSS_INV, &tss_invalid);
    add_exception(EXC_ADDR_SEG_NA, &seg_not_present);
    add_exception(EXC_ADDR_STACK_SEGF, &stack_seg_fault);
    add_exception(EXC_ADDR_GEN_PROTF, &general_protection_fault);
    add_exception(EXC_ADDR_PAGEF, &page_fault);
    add_exception(EXC_ADDR_RESERVEF, &reserved_fault);
    add_exception(EXC_ADDR_FPUF, &fpu_fault);
    add_exception(EXC_ADDR_ALIGN_CK, &alignment_check);
    add_exception(EXC_ADDR_MACHINE_CK, &machine_check);
    add_exception(EXC_ADDR_SIMD_EX, &simd_exception);

    // load the idt
    lidt(idt_desc_ptr);
}
