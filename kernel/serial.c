#include <stddef.h>
#include <mpx/io.h>
#include <memory.h>
#include <mpx/serial.h>
#include <mpx/device.h>
#include <sys_req.h>
#include <mpx/interrupts.h>
#include <string.h>
#include <mpx/pcb.h>
#include <stdint.h>
#include <sys_req.h>
#include <mpx/commhand.h>
 
extern void serial_isr(void*);
dcb* serial_port = NULL;

extern pcb* global_pcb; 

#define BUFFER_OVERFLOW -1

//Error codes for serial_read
#define READ_PORT_NOT_OPEN -301
#define READ_INV_BUF_ADDR -302
#define READ_INV_COUNT_ADDR -303
#define READ_DEV_BUSY -304

//Error codes for serial_write
#define WRITE_PORT_NOT_OPEN -401
#define WRITE_INV_BUF_ADDR -402
#define WRITE_INV_COUNT_ADDR -403
#define WRITE_DEV_BUSY -404

//Error codes for serial_open
#define NULL_FLAG_PTR -101
#define INVALID_RATE -102
#define PORT_IN_USE -103

//Error code for serial_close
#define PORT_NOT_OPEN -201

enum uart_registers {
	RBR = 0, // Receive Buffer
	THR = 0, // Transmitter Holding
	DLL = 0, // Divisor Latch LSB
	IER = 1, // Interrupt Enable
	DLM = 1, // Divisor Latch MSB
	IIR = 2, // Interrupt Identification
	FCR = 2, // FIFO Control
	LCR = 3, // Line Control
	MCR = 4, // Modem Control
	LSR = 5, // Line Status
	MSR = 6, // Modem Status
	SCR = 7, // Scratch
};

static int initialized[4] = {0};

/*dcb* create_dcb(void) {
	dcb* new_dcb = NULL;

	//initialize dcb fields 
	new_dcb->status = AVAILABLE;
	new_dcb->event_flag = 0;
	new_dcb->ring_head = NULL;
	new_dcb->ring_tail = NULL;

	return new_dcb;
}*/

int serial_close(device dev)
{
	if (dev == COM1){

		}
	//serial_port->status = AVAILABLE;
	// ensure port is open
	/*if (serial_port->status != AVAILABLE)
	{
		return PORT_NOT_OPEN;
	}*/

	// clear the open indicator in dcb
	//serial_port->status = AVAILABLE;

	// disable the appropriate level in pic mask register
	cli();
	int mask = inb(STATUS_REG);
	mask |= (1 << 3);
	outb(STATUS_REG, mask);
	sti();

	// Store the value 0 in the Interrupt Enable Reg
	outb(COM1 + 1, 0x01);

	// Disable overall serial port interrpts by storing the value 0in MCR
	outb(COM1 + 4, 0x03);

	return 0;
}

static int serial_devno(device dev) {
	switch (dev)
	{
	case COM1:
		return 0;
	case COM2:
		return 1;
	case COM3:
		return 2;
	case COM4:
		return 3;
	}
	return -1;
}

int serial_init(device dev) {
	int dno = serial_devno(dev);
	if (dno == -1)
	{
		return -1;
	}
	outb(dev + IER, 0x00);			// disable interrupts
	outb(dev + LCR, 0x80);			// set line control register
	outb(dev + DLL, 115200 / 9600); // set bsd least sig bit
	outb(dev + DLM, 0x00);			// brd most significant bit
	outb(dev + LCR, 0x03);			// lock divisor; 8bits, no parity, one stop
	outb(dev + FCR, 0xC7);			// enable fifo, clear, 14byte threshold
	outb(dev + MCR, 0x0B);			// enable interrupts, rts/dsr set
	(void)inb(dev);					// read bit to reset port
	initialized[dno] = 1;
	return 0;
}

/**
 Initialize a serial port. 
 @param device The device to open
 @param speed The deseried baud rate
 @return An integer indicating success or failure
*/ 
int serial_open(device dev, int speed) {
	//Initialize new dcb with value
	//dcb* new_dcb = create_dcb();

	serial_port = sys_alloc_mem(sizeof(dcb));
	serial_port->count = 0;
	serial_port->event_flag = 0;
	serial_port->counter = 0;
	serial_port->operation = IDLE;
	serial_port->ring_head = 0;
	serial_port->ring_tail = 15;
	serial_port->status = AVAILABLE;
	serial_port->curr_dev = COM1;

	serial_port->curr_dev = dev;
	//inb(dev);
	initialized[0] = 1;
	//Ensure that the parameters are valid
	if (dev != COM1 || speed < 0)
	{
		return INVALID_RATE;
	}
	
	//If the device is currently in user
	if (serial_port->status == IN_USE)
	{
		return PORT_IN_USE;
	}

	//install new interrupt vector handler
	idt_install(0x24, serial_isr);

	///compute baud rate
	int baud_rate = speed;
	int baud_rate_div = 115200 / (long) baud_rate;
	int remainder = 115200 % (long) baud_rate;
  

	//Disables Serial Interrupts
	outb(dev + IER, 0x00);

	//Allows access to baud rate registers
	outb(COM1+LCR, 0x80);

	//Store the value in the DLM
	outb(COM1+DLM, remainder);

	outb(COM1 + DLL, baud_rate_div);
	
	//Set Comms config to 8 data bits, 1 stop bit, no parity
	outb(COM1 + LCR, 0x03);
	int mask = inb(STATUS_REG);
	mask = mask & ~0x10; // PIC level four, AND current value with 0b11101111
	outb(STATUS_REG, mask);
	//Enables all serial interrupts from modem
	outb(dev + MCR, 0x0F); 

	//Enables Serial Interrupts (Read Only)
	outb(dev + IER, 0x01);

	return 0;
}

int serial_out(device dev, const char *buffer, size_t len) {
	int dno = serial_devno(dev);
	if (dno == -1 || initialized[dno] == 0)
	{
		return -1;
	}
	for (size_t i = 0; i < len; i++){
		outb(dev, buffer[i]);
	}
	return (int)len;
}

void serial_interrupt(void) {
	
	// Read the Interrupt ID Register to determine the exact type of interrupt
	uint8_t interruptID = inb(serial_port->curr_dev + IIR);

	// 7 = 0b00000111
	interruptID &= 7; 
	// Based on the type (input, output), pass the handling to a second function
	if (interruptID == 2) {
		// Output interrupt
		serial_output_interrupt(serial_port);
	}
	else if (interruptID == 4){
		// Input interrupt
		serial_input_interrupt(serial_port);
	}
	else if (interruptID == 0) {
		inb(serial_port + MSR);
	}
	else if (interruptID == 6) {
		inb(serial_port + LSR);
	}

	// Issue EOI (End of Interrupt) to the PIC
	outb(0x20, 0x20); // Replace with the correct PIC port address
	
	
}

/*
Function:
Serial Poll.
Captures keystroks of all ASCII values as a string.
Then should pass into a buffer to write and count to the maximum bits allowed for buffer.
If buffer exceeds buffer's maximum bits allowed, return BUFFER_OVER_FLOW = -1.
*/
int serial_poll(device dev, char *buffer, size_t len) {
	size_t bytes_read = 0;
	dev = COM1; // sets dev to access COM1
	char c;
	while (bytes_read < len)
	{
		if ((inb(dev + LSR) & 1))
		{ // Condition checks DR bit
			c = inb(dev);
			/* Update the user buffer or otherwise handle the data */
			buffer[bytes_read++] = c;
			outb(dev, c);

			// Recognize backspace for Windows and macOS
			if (c == 8 || c == 127)
			{
				if (bytes_read > 0)
				{
					bytes_read--;
					buffer[--bytes_read] = '\0';
					// Move cursor back
					outb(dev, '\b');
					// insert blank space
					outb(dev, ' ');
					// move cursor back
					outb(dev, '\b');
				}
			}
			// enter and return keys
			if (c == '\n' || c == '\r')
			{
				// Null-terminate the buffer to mark the end of the input
				buffer[bytes_read] = '\0';
				// Send a newline character to move to a new line
				outb(dev, '\n');
				return bytes_read; // Return the number of characters read
			}
		}
		if (bytes_read > len)
		{
			return BUFFER_OVERFLOW;
		}
	}
	return bytes_read;
}

// ring buffer is circular queue
/*
ring buffer

make it in a struct in dcb

represent a 2d array with a circular object?

if you aren't writing
    
make sure you're within the bounds of the ring for the linear
and the circular bounds

only is accessed in serial_read!!!

set n equal to the start because when you aren't moving, they start from the same spot

when you start reading from the buffer, update the end

make sure the index is within the bounds

index is char*

char* and int* are the same amount of data so indexing either or doesn't matter

make temp vari for start of ring buffer

check for newline and return, empty the buffer

access the temp variable and set everything back to the beginning

make sure that the index is equal to the size
*/

// check to make sure it is not already open
// enable interrupt register

int serial_read(device dev, char *buf, size_t len) {
	if (dev == COM1) {}

	dcb* curr_dev = serial_port;
		
	// ensure that the port is open and status is idle
	if (curr_dev->status == IN_USE) {
		return -301;
	}

	if (buf == NULL) {
		return -302;
	}

	// validate the supplied parameters
	if (len == 0) {
		return -303;
	}

	if (curr_dev->operation != IDLE) {
		return -304;
	}
		
	// initialize the input buffer variable (not the ring biffr!) 
	//and set the status to reading
	size_t count = 0;
	int buffer_index = 0;
	curr_dev->operation = READ;

	// clear the caller's event flag
	curr_dev->event_flag = 0;

	// copy characters from the ring buffer to the requestor's buffer
	// until the ring buffer is emptied, the requested count has been reached,
	// or a new-line code has been found. the copied characters should, of course,
	// be removed from the ring buffer. either input interrupts or all interrupts
	// should be disabled during the copying
	while (count < len && curr_dev->ring_buffer[buffer_index] != '\0') {
		char current_character = curr_dev->ring_buffer[buffer_index];
		
		// new-line code has been found
		if (current_character == '\n') {
			curr_dev->ring_buffer[buffer_index] = '\0';
			curr_dev->operation = IDLE;
			curr_dev->event_flag = 1;
			return 0;
		}

		buf[count++] = current_character;
		curr_dev->ring_buffer[buffer_index++] = '\0';
		// buffer_index++;
	}

	// if more charcters are needed, return. if the block is complete
	// continue with next step
	if (count < len) {
		return 0;
	}
	

	// reset the DCB status to idle, set the event flag and return the actual
	// count to the requestor's variable
	curr_dev->operation = IDLE;
	curr_dev->event_flag = 1;

	return 0;
}

int serial_write(device dev, char *buf, size_t len) {
	if (dev == COM1) {}

	dcb* curr_dev = serial_port;

	// ensure that the port is currently OPEN
	if (curr_dev->status == IN_USE) {
		return -404;
	}

	// ensure that the input parameters are valid
	if (buf == NULL) {
		return -402;
	}

	// ensure that the input parameters are valid
	if (len < 0) {
		return -403;
	}

	// ensure that the port is currently IDLE
	if (initialized[0] == 0) {
		return -401;
	}
	if (curr_dev->operation == IDLE) {
		curr_dev->operation = WRITE;
		curr_dev->event_flag = 1;
		curr_dev->counter = 0;
		curr_dev->count = len;

		//Takes current value of IER 
		int mask = inb(dev + IER);

		//Disables Serial Interrupts
		outb(dev + IER, 0x00);

		// or with bit one, 0b00000010
		mask = mask | 0x02;

		outb(dev + IER, mask);

		//Triggers output interrupt
		outb(dev, buf[0]);

		curr_dev->counter++;

		return 0;
	
	}
	else {
		return -404;
	}
}

void io_scheduler (device dev, char *buf, size_t len, op_code operation) {
	
	if (dev != COM1 || buf == NULL) {
		return;
	}
	iocb* temp = sys_alloc_mem(sizeof(iocb));
	temp->buffer = buf;
	temp->buf_size = len;
	temp->device = dev;
	temp->curr_op = operation;
	temp->curr_proc = global_pcb;
	temp->next = NULL;
	
	if (serial_port->io_head == NULL) {
		serial_port->io_head = temp;
	}
	else {
		iocb* iter = serial_port->io_head;
		if (temp != NULL) {
			while (iter->next != NULL) {
				iter = iter->next;
			}
		}
		iter->next = temp;
		temp->next = NULL;
	}

	if (serial_port->operation == IDLE) {
		if (operation == WRITE) {
			
			serial_write(dev, buf, len);
			
		}

		if (operation == READ) {
			serial_read(dev, buf, len);
			
		}
	}
	
}


// handles most of the work for the read and write registers

void serial_input_interrupt(dcb* dcb) {
	// read the character from the device

	// if the dcb state is reading, store the character in the 
	// appropriate iocb buffer
	if (dcb->operation == READ) {
		outb(COM1, 0x02);
	}

	// if the buffer is now full, or input was new-line, signal completion

	// attmept to store the character in the ring buffer
}

void serial_output_interrupt(dcb* dcb) {
	// if the dcb state is writing, check for additional characters in the 
	// appropriate iocb buffer
	if (dcb->operation != WRITE) {
		return;
	}
	// if there is additional data, write the next character to teh device
	outb(COM1, 0x02);

	//takes character at index of counter, that is index of counter of the buffer, which is in the iocb, which is head of the io queue of the dcb.
	//Now taking that character and storing it in the transmit holding register of the device 
	if (dcb->event_flag == 1 && dcb->counter < dcb->count) {
		outb(dcb->curr_dev + THR, dcb->io_head->buffer[dcb->counter]);
		dcb->counter++;
	}
	else {
		dcb->status = AVAILABLE;
		dcb->event_flag = 0;
		iocb* mark_head = dcb->io_head;
		dcb->io_head = dcb->io_head->next; 
		dcb->operation = IDLE;
		unblock_pcb(mark_head->curr_proc);
		sys_free_mem(mark_head);
		int mask = inb(COM1 + IER);
		//Disables Serial Interrupts
		outb(COM1 + IER, 0x00);
		//Disables output interrupts
		mask &= ~0x02;
		//Renables everything except output interrupts
		outb(COM1 + IER, mask);
	}

}


