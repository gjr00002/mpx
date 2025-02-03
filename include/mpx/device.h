#ifndef MPX_DEVICES_H
#define MPX_DEVICES_H

#include <sys_req.h>
#include <mpx/io.h>

typedef enum {
	COM1 = 0x3f8,
	COM2 = 0x2f8,
	COM3 = 0x3e8,
	COM4 = 0x2e8,
} device;

typedef enum
{
   AVAILABLE,
   IN_USE
} alloc_status;

#include <sys_req.h>

typedef struct io_control_block
{
	pcb* curr_proc;
	device device;
	op_code curr_op;
	char* buffer;
	int buf_size;
	int* buf_index;
	struct io_control_block* next;
} iocb;

typedef struct device_control_block
{
	device curr_dev;
   alloc_status status;
   op_code operation;
   int event_flag;
   int counter;
   int count;
   char ring_buffer[16];
	int ring_head;
	int ring_tail;
	iocb* io_head;
} dcb;

#endif
