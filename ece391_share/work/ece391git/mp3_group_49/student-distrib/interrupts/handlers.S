/* exceptions */
.globl divide_error
.globl debug
.globl nmi
.globl breakpoint
.globl overflow
.globl bound_exceed
.globl invalid_opcode
.globl device_not_available
.globl double_fault
.globl coprocessor_segment_overrun
.globl tss_invalid
.globl seg_not_present
.globl stack_seg_fault
.globl general_protection_fault
.globl page_fault
.globl reserved_fault
.globl fpu_fault
.globl alignment_check
.globl machine_check
.globl simd_exception

/* interrupts */
.globl keyboard_interrupt
.globl rtc_interrupt

/* other */
.globl sys_call
.globl common_exception_handler

/*
 *  These are what are called from the the interrupts.
 *  They can set what irq or error code they want, then
 *  they call the common interrupt.
 * 
 *  CLI, push IDT number, use common handler
 */
divide_error:
    cli
    pushl $0
    jmp common_exception_handler

debug:
    cli
    pushl $1
    jmp common_exception_handler

nmi:
    cli
    pushl $2
    jmp common_exception_handler

breakpoint:
    cli
    pushl $3
    jmp common_exception_handler

overflow:
    cli
    pushl $4
    jmp common_exception_handler

bound_exceed:
    cli
    pushl $5
    jmp common_exception_handler

invalid_opcode:
    cli
    pushl $6
    jmp common_exception_handler

device_not_available:
    pushl $7
    jmp common_exception_handler

double_fault:
    cli
    pushl $8
    jmp common_exception_handler

coprocessor_segment_overrun:
    cli
    pushl $9
    jmp common_exception_handler

tss_invalid:
    cli
    pushl $10
    jmp common_exception_handler

seg_not_present:
    cli
    pushl $11
    jmp common_exception_handler

stack_seg_fault:
    cli
    pushl $12
    jmp common_exception_handler

general_protection_fault:
    cli
    pushl $13
    jmp common_exception_handler

page_fault:
    cli
    pushl $14
    jmp common_exception_handler

reserved_fault:
    cli
    pushl $15
    jmp common_exception_handler

fpu_fault:
    cli
    pushl $16
    jmp common_exception_handler

alignment_check:
    cli
    pushl $17
    jmp common_exception_handler

machine_check:
    cli
    pushl $18
    jmp common_exception_handler

simd_exception:
    cli
    pushl $19
    jmp common_exception_handler


/*  
 *  Interrupt handlers are similar to exception handlers
 *  Push the negative IRQ number and indicate where the irq should be handled.
 */

keyboard_interrupt:
    pushl $-1
    jmp common_interrupt_handler


rtc_interrupt:
    pushl $-8
    jmp common_interrupt_handler



# syscall dummy support
sys_call:
    /* assert the syscall number is between 1-10 */
    cmpl $0, %eax
    je sys_call_invalid
    cmpl $10, %eax
    ja sys_call_invalid

    /* save registers */
    pushl %fs
    pushl %es
    pushl %ds
    push %ebp
    push %edi
    push %esi
    push %edx
    push %ecx
    push %ebx

    sti

    call *sys_call_table(, %eax, 4)

    /* restore registers */
    pop %ebx
    pop %ecx
    pop %edx
    pop %esi
    pop %edi
    pop %ebp
    popl %ds
    popl %es
    popl %fs

    sti /* sanity check */

    iret

sys_call_invalid:
    /* return -1 if the syscall number is invalid */
    mov $-1, %eax
    iret

sys_call_table:
.long 0
.long halt
.long execute
.long read
.long write
.long open
.long close
.long getargs
.long vidmap
.long set_handler
.long sigreturn


/*  common exception handler
 *  Saves all registers then jump to the exception handler
 *  which was passed in through %eax
 */
common_exception_handler:
    popl irq_vector /* save irq num */
    pushl %fs       /* save registers */
    pushl %es
    pushl %ds
    pushal

    pushl irq_vector    /* bring back irq num */
    call do_exception   /* call common handler */

    popl %eax           /* restore registers */

    popal
    popl %ds
    popl %es
    popl %fs

    sti                 /* re-enable interrupts */

    iret

/*  common interrupt handler
 *  Saves all registers then jump to the exception handler
 *  which was passed in through the irq_vector location
 */
common_interrupt_handler:

    popl irq_vector /* save irq num */
    pushl %fs       /* save registers */
    pushl %es
    pushl %ds
    pushal

    pushl irq_vector    /* bring back irq num */
    call do_IRQ         /* call common handler */

    popl %eax           /* restore registers */

    popal
    popl %ds
    popl %es
    popl %fs
    iret

irq_vector:
    .long 0
