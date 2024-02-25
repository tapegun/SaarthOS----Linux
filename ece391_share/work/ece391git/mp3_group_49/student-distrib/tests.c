#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "drivers/fs.h"
#include "drivers/terminal.h"
#include "drivers/rtc.h"

#define PASS 1
#define FAIL 0


/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */


// To test RTC, go to rtc.c and define RTC_TEST.
// It will increment every character on the screen


/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* IDT Test - Divide by 0
 * Shows Divide by 0 Expection
 * Inputs: None
 * Outputs: FAIL if no exepction raised
 * Side Effects: None
 * Coverage: Exception 0
 * Files: exceptions.h/c, handlers.S
 */
int idt_test_div0(){
	TEST_HEADER;
	int i = 0;
	i = 9/i;
	return FAIL;
}

/* IDT Test - Divide double fault
 * calls overflow exception, which calls seg not present
 * Inputs: None
 * Outputs: FAIL if no exepction raised
 * Side Effects: None
 * Coverage: Exception 0
 * Files: exceptions.h/c, handlers.S
 */
int idt_test_multiple_exception(){
	TEST_HEADER;
	asm volatile ("int $0x04"); // overflow exception
	return FAIL;
}




/* Paging Test
 * Asserts: show that dereferencing location 0 produces a page fault
 * Inputs: None
 * Outputs: None--if no exepction raised--> FAIL
 * Side Effects: Pagefault generated
 * Coverage: Paging Implementation/Setup
 * Files: paging.h/c
 */
int paging_inalid_1(void){
	TEST_HEADER;
	//Mapping to Page 0 Produces a page fault
	int *a = 0; 
	*a = 3;
	return FAIL;
}


/* Paging Test
 * Asserts: show that dereferencing hat dereferencing 
 * 			locations that shouldn’t be accessible are not.
 * Inputs: None--if no exepction raised--> FAIL
 * Outputs: None
 * Side Effects: Pagefault generated
 * Coverage: Paging Implementation/Setup
 * Files: paging.h/c
 */
int paging_inalid_2(void){
	TEST_HEADER;
	//Attempbting to derefrence a location above 8MB
	int *a = (int *) 0x80000; 
	*a = 3;
	return FAIL;
}

/* Paging Test
 * Asserts: show that dereferencing locations 
 * 			that should be accessible are accessible
 * 			from VGA memory
 * Inputs: None
 * Outputs: PASS if no pagefault generated
 * Side Effects: Prints memory location & data, should not pagefault
 * Coverage:
 * Files: paging.h/c
 */
int valid_vga(void){
	TEST_HEADER;
	//Attemmpting to derefrence a location between 0xb8000 & 0xb9000
	int *a = (int *) 0xB8070; 
	printf(" Data at %p: %x\n", a, *a);
	return PASS;
}

/* Paging Test
 * Asserts: show that dereferencing locations 
 * 			that should be accessible are accessible
 * 			from kernal memory
 * Inputs: None
 * Outputs: PASS if no pagefault generated
 * Side Effects: Prints memory location & data, should not pagefault
 * Coverage:
 * Files: paging.h/c
 */
int valid_kernal(void){
	TEST_HEADER;
	//Attempting to derefrence a location between 0x400000 and 0x80000
	int *a = (int *) 0x600000; 
	printf(" Data at %p: %x\n", a, *a);
	return PASS;
}

int testing_terminal_loading_spec_chars(void){
	TEST_HEADER;
	int8_t kbd_buf[128];
	int j = 0;
    for(; j < 11; j++){
        kbd_buf[j] = j;
    }
    kbd_buf[11] = 0;
    terminal_write(1, kbd_buf, 10);
	putc('\n');
	return PASS;
}

int terminal_buffered(void){
	TEST_HEADER;
	int8_t kbd_buf[128];
	int i = 0;
	while (1) {
        //testing line buffered input
        int n = terminal_read(1, kbd_buf, 5);
        kbd_buf[n+1] = 0; // null terminate the string from 
        terminal_write(1, kbd_buf, 6);
		i++;
		if(i == 5){
			break;
		}
    }
	putc('\n');
	return PASS;
}

int buffer_overflow_terminal(void){
	TEST_HEADER;
	int8_t test_buf[135];
	int n = terminal_read(1, test_buf, 130);
	test_buf[n+1] = 0; // null terminate the string from 
	terminal_write(1, test_buf, 131);
	putc('\n');
	return PASS;
}
/*
Exception Tets/IDT:
1) Divide by 0
2) general protection fault by calling non existsing inerrupt
3) Overflow
4) MULTIPLE EXPECTIONS????
*/


/*RTC
One line note--
*/

//Write Test for sys CALL





/* Checkpoint 2 tests */

/* RTC Test
 * Asserts: show that RTC can change frequency by displaying
 * 			shapes on the screen on a changing interval
 * Inputs: None
 * Outputs: shapes on the screen at frequency intervals
 * Side Effects: modifies RTC frequency, progressively increasing it
 * Coverage:
 * Files: rtc.h/c
 */
int rtc_test_cp2(){
	TEST_HEADER;
	uint32_t freq, i;

	/* test rtc_open */
	(void)rtc_open(0);
	printf("Testing RTC open:   ");
	for(i=0; i<20; i++){
		(void)rtc_read(0, "", 0);
		printf("#");
	}

	/* test rtc_write */
	for(freq=4; freq<=1024; freq=freq<<1){
		printf("\nTesting RTC at frequency %d:   ", freq);
		(void)rtc_write(0, &freq, 0);
		for(i=0; i<20; i++){
			(void)rtc_read(0, "", 0);
			printf("#");
		}
	}

	/* finally, confirm that frequency has to be a power of 2 */
	printf("\n");
	freq = 768; //arbitrarily chosen
	if(rtc_write(0, &freq, 0) == -1){
		return PASS;
	}else{
		return FAIL;
	}
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */







/* Test suite entry point */
void launch_tests(){

	// //test syscall
	// asm volatile("int $0x80");
	
	// TEST_OUTPUT("idt_test", idt_test());

	// //TEST_OUTPUT("idt_test_div0", idt_test_multiple_exception());
	// //TEST_OUTPUT("idt_test_div0", idt_test_div0());
	// // launch your tests here

	// /*Paging Tests*/	
	// TEST_OUTPUT("valid_vga", valid_vga());
	// TEST_OUTPUT("valid_kernal", valid_kernal());

	// TEST_OUTPUT("di", valid_kernal());	

	// TEST_OUTPUT("di", valid_kernal());

	fs_test();

	/* test rtc */
	TEST_OUTPUT("rtc invalid frequency", rtc_test_cp2());


	// For terminal
	TEST_OUTPUT("line_buffered_input", terminal_buffered());		
	TEST_OUTPUT("buffer_overflow_terminal", buffer_overflow_terminal());
	TEST_OUTPUT("loading_buf_into_terminal_write", testing_terminal_loading_spec_chars());

	
	// /*Only run  to crash machine*/
	// //TEST_OUTPUT("paging_null_ptr", paging_inalid_1());
	// //TEST_OUTPUT("paging_out_of_bounds", paging_inalid_2());


}
