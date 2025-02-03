#include <sys_call.h>
#include <mpx/pcb.h>
#include <mpx/device.h>
#include <mpx/serial.h>

// Global PCB pointer representing the currently executing process
struct pcb *global_pcb = NULL;
// Global or static context pointer representing the initial context
struct context *initial_context = NULL;

struct context *sys_call(struct context *current_context)
{

    // Extract the system call number from the current context
    int syscall_number = current_context->eax;
    // int a = current_context->ebx;
    // int c = current_context->ecx;
    // int d = current_context->edx;

    struct pcb *next_pcb = ready_head;
    device dev;
    char *buf;
    int length;

    // Perform actions based on the system call number
    switch (syscall_number)
    {

    case WRITE:
        global_pcb->stack_top = (unsigned char *)current_context;
        global_pcb->state = BLOCKED_STATE;
        dev = current_context->ebx;
        buf = (char *)current_context->ecx;
        length = current_context->edx;
        io_scheduler(dev, buf, length, syscall_number);

        pcb_insert(global_pcb);

    case READ:

        global_pcb->stack_top = (unsigned char *)current_context;
        global_pcb->state = BLOCKED_STATE;
        dev = current_context->ebx;
        buf = (char *)current_context->ecx;
        length = current_context->edx;
        io_scheduler(dev, buf, length, syscall_number);

        pcb_insert(global_pcb);

    case IDLE:

        if (initial_context == NULL)
        {
            initial_context = current_context;
        }

        if (next_pcb == NULL)
        {
            return current_context;
        }

        if (global_pcb != NULL)
        {
            global_pcb->stack_top = (unsigned char *)current_context;
            // make the running process ready
            global_pcb->state = READY_STATE;
            // insert into the ready queue
            pcb_insert(global_pcb);
        }

        else if (global_pcb == NULL)
        {
            break;
        }
        break;

    case EXIT:
        if (global_pcb != NULL)
        {
            // Handle process termination and resource cleanup
            pcb_remove(global_pcb);
            next_pcb->next_pcb = NULL;
            pcb_free(global_pcb);
            global_pcb = NULL; // No active process

            // Switch to the exit context
            return initial_context;
        }
        break;

    // means for now its read or write
    default:
        current_context->eax = -1;
        return current_context;
    }

    if (next_pcb == NULL)
    {
        return initial_context;
    }

    else
    {
        global_pcb = next_pcb;
        pcb_remove(next_pcb);
        next_pcb->next_pcb = NULL;
        global_pcb->state = READY_STATE;
        return (struct context *)global_pcb->stack_top;
    }

    return 0;
}
