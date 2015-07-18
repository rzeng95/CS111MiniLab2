#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"

/*****************************************************************************
 * schedos-kern
 *
 *   This is the schedos's kernel.
 *   It sets up process descriptors for the 4 applications, then runs
 *   them in some schedule.
 *
 *****************************************************************************/

// The program loader loads 4 processes, starting at PROC1_START, allocating
// 1 MB to each process.
// Each process's stack grows down from the top of its memory space.
// (But note that SchedOS processes, like MiniprocOS processes, are not fully
// isolated: any process could modify any part of memory.)

#define NPROCS		5
#define PROC1_START	0x200000
#define PROC_SIZE	0x100000

// +---------+-----------------------+--------+---------------------+---------/
// | Base    | Kernel         Kernel | Shared | App 0         App 0 | App 1
// | Memory  | Code + Data     Stack | Data   | Code + Data   Stack | Code ...
// +---------+-----------------------+--------+---------------------+---------/
// 0x0    0x100000               0x198000 0x200000              0x300000
//
// The program loader puts each application's starting instruction pointer
// at the very top of its stack.
//
// System-wide global variables shared among the kernel and the four
// applications are stored in memory from 0x198000 to 0x200000.  Currently
// there is just one variable there, 'cursorpos', which occupies the four
// bytes of memory 0x198000-0x198003.  You can add more variables by defining
// their addresses in schedos-symbols.ld; make sure they do not overlap!


// A process descriptor for each process.
// Note that proc_array[0] is never used.
// The first application process descriptor is proc_array[1].
static process_t proc_array[NPROCS];

// A pointer to the currently running process.
// This is kept up to date by the run() function, in mpos-x86.c.
process_t *current;

// The preferred scheduling algorithm.
int scheduling_algorithm;

/*****************************************************************************
 * start
 *
 *   Initialize the hardware and process descriptors, then run
 *   the first process.
 *
 *****************************************************************************/

void
start(void)
{
	int i;

	// Set up hardware (schedos-x86.c)
	segments_init();
	interrupt_controller_init(1);
	console_clear();

	// Initialize process descriptors as empty
	memset(proc_array, 0, sizeof(proc_array));
	for (i = 0; i < NPROCS; i++) {
		proc_array[i].p_pid = i;
		proc_array[i].p_state = P_EMPTY;
		proc_array[i].p_priority = 0; //4A (initialized)
		proc_array[i].p_share = 0; //4B (initialized)
	}
	
	// Set up process descriptors (the proc_array[])
	for (i = 1; i < NPROCS; i++) {
		process_t *proc = &proc_array[i];
		uint32_t stack_ptr = PROC1_START + i * PROC_SIZE;

		// Initialize the process descriptor
		special_registers_init(proc);

		// Set ESP
		proc->p_registers.reg_esp = stack_ptr;

		// Load process and set EIP, based on ELF image
		program_loader(i - 1, &proc->p_registers.reg_eip);

		// Mark the process as runnable!
		proc->p_state = P_RUNNABLE;
		
		// Give priority = corresponding PID (1 through 4) 
		proc->p_priority = proc->p_pid;

		//give share amt = corresponding PID (1 gets 1, 2 gets 2, etc)
		proc->p_share = proc->p_pid;
	}

	// Initialize the cursor-position shared variable to point to the
	// console's first character (the upper left).
	cursorpos = (uint16_t *) 0xB8000;

	// Initialize the scheduling algorithm.
	scheduling_algorithm = 0;

	// Switch to the first process.
	run(&proc_array[1]);

	// Should never get here!
	while (1)
		/* do nothing */;
}



/*****************************************************************************
 * interrupt
 *
 *   This is the weensy interrupt and system call handler.
 *   The current handler handles 4 different system calls (two of which
 *   do nothing), plus the clock interrupt.
 *
 *   Note that we will never receive clock interrupts while in the kernel.
 *
 *****************************************************************************/

void
interrupt(registers_t *reg)
{
	// Save the current process's register state
	// into its process descriptor
	current->p_registers = *reg;

	switch (reg->reg_intno) {

	case INT_SYS_YIELD:
		// The 'sys_yield' system call asks the kernel to schedule
		// the next process.
		schedule();

	case INT_SYS_EXIT:
		// 'sys_exit' exits the current process: it is marked as
		// non-runnable.
		// The application stored its exit status in the %eax register
		// before calling the system call.  The %eax register has
		// changed by now, but we can read the application's value
		// out of the 'reg' argument.
		// (This shows you how to transfer arguments to system calls!)
		current->p_state = P_ZOMBIE;
		current->p_exit_status = reg->reg_eax;
		schedule();

	case INT_SYS_USER1:
		// 'sys_user*' are provided for your convenience, in case you
		// want to add a system call.
		/* Your code here (if you want). */
		run(current);

	case INT_SYS_USER2:
		/* Your code here (if you want). */
		run(current);

	case INT_CLOCK:
		// A clock interrupt occurred (so an application exhausted its
		// time quantum).
		// Switch to the next runnable process.
		schedule();

	case INT_SYS_PRINT:
		*cursorpos++ = reg->reg_eax; //6
		run(current);

	default:
		while (1)
			/* do nothing */;

	}
}



/*****************************************************************************
 * schedule
 *
 *   This is the weensy process scheduler.
 *   It picks a runnable process, then context-switches to that process.
 *   If there are no runnable processes, it spins forever.
 *
 *   This function implements multiple scheduling algorithms, depending on
 *   the value of 'scheduling_algorithm'.  We've provided one; in the problem
 *   set you will provide at least one more.
 *
 *****************************************************************************/

void
schedule(void)
{
	pid_t pid = current->p_pid;

	if (scheduling_algorithm == 0)
		while (1) {
			pid = (pid + 1) % NPROCS;

			// Run the selected process, but skip
			// non-runnable processes.
			// Note that the 'run' function does not return.
			if (proc_array[pid].p_state == P_RUNNABLE)
				run(&proc_array[pid]);
		}
	
	if (scheduling_algorithm == 1) {
		//strict priority scheduling
		//In this case, we want to run schedos-1 stuff before schedos-2, 			//schedos-2 before schedos-3, etc. 
		//proc_array[1] is the processes corresponding to schedos-1
		//hence, do all the proc_array[x] in order from 1 to 4
		while(1) {
			int i;		
			for (i = 1; i <= 4; i++) { 
				if (proc_array[i].p_state == P_RUNNABLE)
					run(&proc_array[i]);
			}
		}
		//When I test this config with 'make run', it displays:
		//1111...2222.....3333....44444....which demonstrates priority
		//scheduling. 

	}
	if (scheduling_algorithm == 2) {
		//We want to do the same thing as above but using a priority variable.
		//In schedos-kern.h, we can define p_priority in the process_t struct
		//Then, initialize it above in start(void)
		//I initialize each process with a priority equal to its pid
		//Hence, our result should be 1111...2222....3333....4444 
		while(1) {			
			int i;
			for (i = 1; i <= 4; i++) {
				if (proc_array[i].p_state == P_RUNNABLE && 
				    proc_array[i].p_priority == i)
					run(&proc_array[i]);				
			}
		}
		//The expected result is achieved 
	}
	if (scheduling_algorithm == 3) {
		//Each p_share value has been assigned the value of the pid
		//Decrement it each time we find that the proc_array[pid] state is runnable
		//Do this until we hit 0, then move on to the next pid value
		while(1) {
			while (proc_array[pid].p_share > 0) {
				if (proc_array[pid].p_state == P_RUNNABLE) {
					proc_array[pid].p_share--;
					run(&proc_array[pid]);
				}
				else
					pid =(pid + 1)% NPROCS;				
			}
			pid = (pid + 1) % NPROCS;
			proc_array[pid].p_share = pid;
		}	
		//Therefore, the result is:
		//122333444122333444....... (note: somehow I'm getting double 1s in the beginning)
	}
	// If we get here, we are running an unknown scheduling algorithm.
	cursorpos = console_printf(cursorpos, 0x100, "\nUnknown scheduling algorithm %d\n", scheduling_algorithm);
	while (1)
		/* do nothing */;
}
