/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"

#include <lib.h>


#define MASTER_DATA	(MASTER_8259_PORT+1)     // data port for master
#define SLAVE_DATA	(SLAVE_8259_PORT+1)      // data port for slave

#define NUM_IRQS   8    // number of IRQs per PIC

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/*
 * i8259_init
 * DESCRIPTION: initialize the 8259 PIC
 * REFERENCE: https://wiki.osdev.org/PIC
 * INPUTS: none
 * OUTPUTS: start/end of PIC init
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void i8259_init(void) {
    /* disable interrupts */
    cli();

    printf("Starting PIC INIT\n");

    /* mask PIC interrutps */
    outb(MASK_ALL, MASTER_DATA);
    outb(MASK_ALL, SLAVE_DATA);

    printf("resulting config byte %x\n", inb(MASTER_DATA));
  

    // sending all the ICW's to the ports of master and slave to initialize the PIC
    // ICW Macros can be found in the i8259.h file

    outb(ICW1, MASTER_8259_PORT);           //Init master
    outb(ICW2_MASTER, MASTER_DATA);         //IRQ 7 mapped from 0x20-0x27
    outb(ICW3_MASTER, MASTER_DATA);         //Tell master that slave is on port 4
    outb(ICW4, MASTER_DATA); 


    outb(ICW1, SLAVE_8259_PORT);            //Init Slave
    outb(ICW2_SLAVE, SLAVE_DATA);           //IRQ-7 mapped 0x28 to 0x2f
    outb(ICW3_SLAVE, SLAVE_DATA);           //Slave is master on IR2
    outb(ICW4, SLAVE_DATA);

    outb(MASK_ALL, MASTER_DATA);            //masks interrupts on PIC
    outb(MASK_ALL, SLAVE_DATA); 

    enable_irq(IRQ2_SLAVE_PIN);             // enable slave PIC

    /* re-enable interrupts */
    sti();
    printf("Ending PIC INIT\n");
}

/*
 * enable_irq
 * DESCRIPTION: Enable (unmask) the specified IRQ
 * REFERENCE: https://wiki.osdev.org/PIC  section: Unmasking
 * INPUTS: irq_num -- between 0 and 15, id of irq
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void enable_irq(uint32_t irq_num) {

    uint16_t port;
    uint8_t value;

    /* boundary check */
    if (irq_num > 15 || irq_num < 0){
        printf("enable_irq: invalid irq %n\n", irq_num);
        return;
    }
 
    /* select master or slave chip and pin on chip */
    if(irq_num < NUM_IRQS) {
        port = MASTER_DATA;
    } else {
        port = SLAVE_DATA;
        irq_num -= NUM_IRQS;
    }

    /* select and write unmasked version */
    value = inb(port) & ~(1 << irq_num); 
    outb(value, port);
}

/*
 * disable_irq
 * DESCRIPTION: Disable (mask) the specified IRQ
 * REFERENCE: https://wiki.osdev.org/PIC  section: Masking
 * INPUTS: irq_num -- between 0 and 15, id of irq
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void disable_irq(uint32_t irq_num) {

    uint16_t port;
    uint8_t value;

    /* boundary check */
    if (irq_num > 15 || irq_num < 0){
        printf("disable_irq: invalid irq %n\n", irq_num);
        return;
    }

    /* differentiate between master and slave chips */
    if(irq_num < NUM_IRQS) {
        port = MASTER_DATA;
    } else {
        port = SLAVE_DATA;
        irq_num -= NUM_IRQS;
    }

    /* select and write masked version */
    value = inb(port) | (1 << irq_num);
    outb(value, port);
}

/*
 * send_eoi
 * DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 * REFERENCE: https://wiki.osdev.org/PIC
 * INPUTS: irq_num -- between 0 and 15, id of irq
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void send_eoi(uint32_t irq_num) {
    // Code from: https://wiki.osdev.org/PIC  

    /* check for invalid */
    if (irq_num > 15 || irq_num < 0){
        printf("send_eoi: invalid irq %n\n", irq_num);
        return;
    }

    if(irq_num >= NUM_IRQS){
        /* slave PIC */
        outb((irq_num-NUM_IRQS)|EOI,SLAVE_8259_PORT);
        outb(IRQ2_SLAVE_PIN|EOI, MASTER_8259_PORT); //Slave connected to pin 2
    }else{
        /* master PIC */
        outb(irq_num|EOI,MASTER_8259_PORT);
    }
}
