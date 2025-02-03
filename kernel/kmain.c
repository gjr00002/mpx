#include <mpx/gdt.h>
#include <mpx/interrupts.h>
#include <mpx/serial.h>
#include <mpx/pcb.h>
#include <mpx/vm.h>
#include <sys_req.h>
#include <string.h>
#include <memory.h>
#include <mpx/commhand.h>
#include <mpx/pcb.h>
#include <sys_call.h>
#include <stdint.h>

static void klogv(device dev, const char* msg)
{
	char prefix[] = "klogv: ";
	serial_out(dev, prefix, strlen(prefix));
	serial_out(dev, msg, strlen(msg));
	serial_out(dev, "\r\n", 2);
}

void kmain(void)
{
	// 0) Serial I/O -- <mpx/serial.h>
	// If we don't initialize the serial port, we have no way of
	// performing I/O. So we need to do that before anything else so we
	// can at least get some output on the screen.
	// Note that here, you should call the function *before* the output
	// via klogv(), or the message won't print. In all other cases, the
	// output should come first as it describes what is about to happen.
	klogv(COM1, "Initialized serial I/O on COM1 device...");
	serial_init(COM1);
	// 1) Global Descriptor Table (GDT) -- <mpx/gdt.h>
	// Keeps track of the various memory segments (Code, Data, Stack, etc.)
	// required by the x86 architecture. This needs to be initialized before
	// interrupts can be configured.
	klogv(COM1, "Initializing Global Descriptor Table...");
	gdt_init();
	// 2) Interrupt Descriptor Table (IDT) -- <mpx/interrupts.h>
	// Keeps track of where the various Interrupt Vectors are stored. It
	// needs to be initialized before Interrupt Service Routines (ISRs) can
	// be installed.
	klogv(COM1, "Initializing Interrupt Descriptor Table...");
	idt_init();
	// 3) Disable Interrupts -- <mpx/interrupts.h>
	// You'll be modifying how interrupts work, so disable them to avoid
	// crashing.
	klogv(COM1, "Disabling interrupts...");
	cli();
	// 4) Interrupt Request (IRQ) -- <mpx/interrupts.h>
	// The x86 architecture requires ISRs for at least the first 32
	// Interrupt Request (IRQ) lines.
	klogv(COM1, "Initializing Interrupt Request routines...");
	irq_init();
	// 5) Programmable Interrupt Controller (PIC) -- <mpx/interrupts.h>
	// The x86 architecture uses a Programmable Interrupt Controller (PIC)
	// to map hardware interrupts to software interrupts that the CPU can
	// then handle via the IDT and its list of ISRs.
	klogv(COM1, "Initializing Programmable Interrupt Controller...");
	pic_init();
	// 6) Reenable interrupts -- <mpx/interrupts.h>
	// Now that interrupt routines are set up, allow interrupts to happen
	// again.
	klogv(COM1, "Enabling Interrupts...");
	sti();
	// 7) Virtual Memory (VM) -- <mpx/vm.h>
	// Virtual Memory (VM) allows the CPU to map logical addresses used by
	// programs to physical address in RAM. This allows each process to
	// behave as though it has exclusive access to memory. It also allows,
	// in more advanced systems, the kernel to swap programs between RAM and
	// storage (such as a hard drive or SSD), and to set permissions such as
	// Read, Write, or Execute for pages of memory. VM is managed through
	// Page Tables, data structures that describe the logical-to-physical
	// mapping as well as manage permissions and other metadata.
	klogv(COM1, "Initializing Virtual Memory...");
	vm_init();

	initialize_heap(50000);
	sys_set_heap_functions(allocate_memory, free_memory);
	

	// 8) MPX Modules -- *headers vary*
	// Module specific initialization -- not all modules require this.
	klogv(COM1, "Initializing MPX modules...");
	// R5: sys_set_heap_functions(...);
	// R4: create commhand and idle processes
	serial_open(COM1, 19200);
	struct pcb* commhand_proc = pcb_setup("Command Handler", 0, 1);
	struct context* new_context1 = (struct context*)commhand_proc->stack_top;
	memset(new_context1, 0, sizeof(struct context));
	new_context1->cs = 0x08;
	new_context1->ds = 0x10;
	new_context1->es = 0x10;
	new_context1->fs = 0x10;
	new_context1->gs = 0x10;
	new_context1->ss = 0x10;
	new_context1->ebp = (uint32_t)(commhand_proc->stack);
	new_context1->esp = (uint32_t)(commhand_proc->stack_top);
	new_context1->eip = (uint32_t)commhand;
	new_context1->flags = 0x0202;
	pcb_insert(commhand_proc);

	struct pcb* idle_proc = pcb_setup("Idle Process", 0, 9);
	struct context* new_context2 = (struct context*)idle_proc->stack_top;
	memset(new_context2, 0, sizeof(struct context));
	new_context2->cs = 0x08;
	new_context2->ds = 0x10;
	new_context2->es = 0x10;
	new_context2->fs = 0x10;
	new_context2->gs = 0x10;
	new_context2->ss = 0x10;
	new_context2->ebp = (uint32_t)(idle_proc->stack);
	new_context2->esp = (uint32_t)(idle_proc->stack_top);
	new_context2->eip = (uint32_t)sys_idle_process;
	new_context2->flags = 0x0202;
	pcb_insert(idle_proc);
	// 9) YOUR command handler -- *create and #include an appropriate .h file*
	// Pass execution to your command handler so the user can interact with
	// the system.
	klogv(COM1, "Transferring control to commhand...");
	//commhand();
	__asm__ volatile ("int $0x60" :: "a"(IDLE));

//	serial_close(COM1);

	// 10) System Shutdown -- *headers to be determined by your design*
	// After your command handler returns, take care of any clean up that
	// is necessary.
	klogv(COM1, "Starting system shutdown procedure...");
	//serial_close(COM1);


	// 11) Halt CPU -- *no headers necessary, no changes necessary*
	// Execution of kmain() will complete and return to where it was called
	// in boot.s, which will then attempt to power off Qemu or halt the CPU.
	klogv(COM1, "Halting CPU...");
}
