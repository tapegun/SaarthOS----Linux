
.data					# section declaration

        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %al - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
        
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret


mp1_rtc_tasklet:
	pushl %ebp					
	movl %esp, %ebp 			# register saving
	pushl %esi 					
	pushl %edi					
	pushl %ebx 					
	xorl %edx, %edx				
	movl (mp1_list_head), %edx 	# load list head
loop_p1:
	cmpl $0, %edx	 			
	je finish_tasklet			# check for null terminator
	movw COUNTDOWN(%edx), %bx 	# load countdown for comparison and decrement
	subl $1, %ebx				
	movw %bx, COUNTDOWN(%edx)	
	cmpw $0, %bx  				
	jg loop_p2				
	movw STATUS(%edx), %di	
	not %di 						# load status LSB
	and $1, %di
	movw %di, STATUS(%edx)
	cmpw $1, %di 				
	ja char_off
	je char_on
char_off:
	movw OFF_LENGTH(%edx), %bx 		# for resetting countdown
	movb OFF_CHAR(%edx), %cl		# cl for poke
	xorl %eax, %eax				
	movw LOCATION(%edx), %ax 	# load eax for poke
	shlw $1, %ax				# make the weird colors go away.. idk why
	pushl %edx 					
	call mp1_poke 				
	popl %edx 				
	movw %bx, COUNTDOWN(%edx)	
	jmp loop_p2 
char_on:	
	movw ON_LENGTH(%edx), %bx 		# for resetting countdown
	movb ON_CHAR(%edx), %cl			# cl for poke
	xorl %eax, %eax				
	movw LOCATION(%edx), %ax 	# load eax for poke
	shlw $1, %ax				# make the weird colors go away.. idk why
	pushl %edx 					
	call mp1_poke 				
	popl %edx 				
	movw %bx, COUNTDOWN(%edx)	

loop_p2:
	movl NEXT(%edx), %edx 			# moving to next struct
	jmp loop_p1 			
finish_tasklet:
	popl %ebx					# stack restore
	popl %edi
	popl %esi
	leave
	ret



mp1_ioctl:  # no stack registers needed
    movl	8(%esp), %edx	        # load input
	cmpl	$3, %edx	
	ja	invalid_oper
	cmpl	$0, %edx
	jb	invalid_oper
	jmp	*function_list(, %edx, 4)
function_list:
	.long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync
	ret
invalid_oper:
	movl	$-1, %eax		# return -1 if invalid cmd
	ret



mp1_ioctl_add:
	pushl	%ebp			# save register values to use registers
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	
	movl	8(%ebp), %esi	
	cmpl	$0, %esi	   # is it a valid location	
	je		fail_finish
	
	pushl 	$STRUCT_SIZE	
	call	mp1_malloc			# provided function to malloc some mem
	addl	$4, %esp		
	movl	%eax, %ebx		
	cmpl	$0, %eax			# check if function worked out 
	je 		fail_finish
	
	pushl	$STRUCT_SIZE	
	pushl	%esi			
	pushl	%ebx			
	call 	mp1_copy_from_user	# copy the struct, make sure registers are saved above ^
	addl	$12, %esp		
	cmpl	$0, %eax		
	jne		free_add			# free using provided function
	movw	ON_LENGTH(%ebx), %ax	
	movw	%ax, COUNTDOWN(%ebx)	# reset the countdown
	movw 	$1, STATUS(%ebx)		# change status
	movl	mp1_list_head, %edi		# change the list head to a next struct
	movl	%edi, NEXT(%ebx)
	movl	%ebx, mp1_list_head		
	cmpw	$0, LOCATION(%ebx)		# check if the location is NULL 
	jb		free_add
	cmpw	$1999, LOCATION(%ebx) # check if the location is in the range
	ja		free_add
	

	movb	ON_CHAR(%ebx), %cl		# setup registers for poke to screen		
	movw	LOCATION(%ebx), %ax			
	addw	%ax, %ax
	call	mp1_poke
	
add_finish:
	movl	$0, %eax				# "return 0"
	popl	%edi					# restore registers
	popl	%esi		
	popl	%ebx
	leave
	ret

free_add:
	push	%ebx					# free mem
	call 	mp1_free
	addl	$4, %esp
	jmp 	fail_finish







mp1_ioctl_remove:
	pushl %ebp					
	movl %esp, %ebp 			
	movl 8(%ebp), %edx			
	call search_objects_pair		# find pair of structs
	testw %ax, %ax	 				# check if last struct is NULL
	je fail_remove 		
	testl %ecx, %ecx 			
	je head_exchange				# switch at head... pointer manipulation 		
	pushl %eax 					
	movl NEXT(%eax), %edx 		
	movl %edx, NEXT(%ecx)		
	call mp1_free 					# free mem
	movl $0, %eax				
	leave						
	ret							
head_exchange:
	pushl %eax 					
	movl NEXT(%eax), %edx	 	
	movl %edx, mp1_list_head	
	call mp1_free 				
	movl $0, %eax				
	leave						
	ret							
fail_remove:
	movl $-1, %eax 				
	leave
	ret	




        
mp1_ioctl_find:
	pushl	%ebp			# store register values
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	
	movl	8(%ebp), %esi				 
	movw	LOCATION(%esi), %dx			# search for the struct with same location
	call search_obj
	testl %eax, %eax						
	je	fail_find						# not found for some reason
	movl %eax, %ebx
	pushl	$STRUCT_SIZE				# set up stack for copy call
	pushl	%ebx			
	pushl	%esi			
	call 	mp1_copy_from_user	
	addl	$12, %esp					# account for missing vars from function call
	cmpl	$0, %eax					# check null
	jne		fail_find
	movl	$0, %eax					# clear eax
	popl	%edi			
	popl	%esi		
	popl	%ebx
	leave
	ret
	
fail_find:
	movl	$-1, %eax






mp1_ioctl_sync:
	pushl	%ebp			
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	
	movl	8(%ebp), %ecx			
	movw	%cx, %dx				# load the location
	call 	search_obj
	testl   %eax, %eax				# did we find our structure?
	je		err_sync
	movl	%eax, %esi				# esi has second obj
	shrl	$16, %ecx				# shift for next local
	movw	%cx, %dx			
	call 	search_obj
	testl	%eax, %eax				# check return value of search
	je		err_sync
	movl	%eax, %edi				# edi now has first obj
	
    movw 	STATUS(%edi), %dx		# swapping the two structs with dx as a temp variable
    movw 	%dx, STATUS(%esi)
    movw 	COUNTDOWN(%edi), %dx
    movw 	%dx, COUNTDOWN(%esi)
	movw 	OFF_LENGTH(%edi), %dx
    movw 	%dx, OFF_LENGTH(%esi)
	movw 	ON_LENGTH(%edi), %dx	
    movw 	%dx, ON_LENGTH(%esi)
	
	cmpw	$0, STATUS(%esi)	# go to onsync or offsync for poking status onto image
	jne		onsync

offsync:
	movb	OFF_CHAR(%esi), %cl			# poke off char (cl) at a location (ax)	
	xorl	%eax, %eax
	movw	LOCATION(%esi), %ax			
	addw	%ax, %ax
	call	mp1_poke
	jmp		sync_success
onsync:
	movb	ON_CHAR(%esi), %cl			# poke on char (cl) at a location (ax)	
	xorl	%eax, %eax
	movw	LOCATION(%esi), %ax			
	addw	%ax, %ax
	call	mp1_poke
	jmp		sync_success
sync_success:		
	movl	$0, %eax
	popl	%edi			
	popl	%esi		
	popl	%ebx
	leave
	ret

err_sync:
	movl	$-1, %eax
	popl	%edi			
	popl	%esi		
	popl	%ebx
	leave
	ret







# returns an address to the object at LOCATION on the screen
# location is held in ddx
search_obj:							
	movl (mp1_list_head), %eax
	testl %eax, %eax
	je search_ended
search_loop:
	cmpw LOCATION(%eax), %dx 		# check if the current node is at our location else keep searching
	je search_ended
	movl NEXT(%eax), %eax			
	testl %eax, %eax
	je search_ended
	jmp search_loop
search_ended:
	ret


# same as above but storing second to last term as well
search_objects_pair:
	movl (mp1_list_head), %eax
	xorl %ecx, %ecx 			# initialize as NULL
	testl %eax, %eax
	je search_ended
search_loop_1:
	cmpw LOCATION(%eax), %dx 
	je search_ended_1
	movl %eax, %ecx   	# store "previous item" 
	movl NEXT(%eax), %eax
	testl %eax, %eax
	je search_ended_1
	jmp search_loop_1
search_ended_1:
	ret




fail_finish:				# error handling add function
	movl	$-1, %eax		
	popl	%edi			
	popl	%esi		
	popl	%ebx
	leave
	ret


.end

	