#ifndef _PCB_H
#define _PCB_H

#define MAX_NAME_LEN 50
#define MIN_NAME_LEN 2

// Prompt user for 1 or 0 to initialize process class
#define USER_PROC 1
#define SYSTEM_PROC 0

// Prompt user for 1 or 0 to initialize state
#define READY_STATE 1
#define BLOCKED_STATE 0
#define EMPTY_STATE -1
#define SUSPEND_STATE 2
#define RESUME_STATE 3
#define PCB_SUCCESS 0
#define PCB_ERROR 1

typedef struct pcb {
    char name[MAX_NAME_LEN];
    int class;
    int priority;
    int state;
    unsigned char stack[1024];
    unsigned char* stack_top;
    struct pcb* next_pcb;
    struct pcb* prev_pcb;
} pcb;



//Define PCB head pointers
extern struct pcb* ready_head;
extern struct pcb* blocked_head;
extern struct pcb* suspended_head;
extern struct pcb* resume_head;


struct pcb* pcb_allocate(void);

int pcb_free(struct pcb* old_pcb);

struct pcb* pcb_setup(const char* name, int priority, int class);

struct pcb* pcb_find(char* found_name);

void pcb_insert(struct pcb* curr_pcb);

int pcb_remove(struct pcb* curr_pcb);

#endif
