
#include <mpx/pcb.h>
#include <mpx/device.h>
#include <sys_req.h>
#include <memory.h>
#include <string.h>
#include <sys_call.h>

#define MAX_NAME_LEN 50
#define MIN_NAME_LEN 2

// Prompt user for 1 or 0 to initialize process class
#define USER_PROC 1
#define SYSTEM_PROC 0

// Prompt user for 1 or 0 to initialize state
#define READY_STATE 1
#define BLOCKED_STATE 0
#define SUSPENDED_STATE -1
#define RESUME_STATE 3
#define RUNNING_STATE 4

#define PCB_SUCCESS 0
#define PCB_ERROR 1

struct pcb* ready_head;
struct pcb* blocked_head;
struct pcb* suspended_head;
struct pcb* resume_head;

// Function to allocate memory for a new PCB
struct pcb* pcb_allocate(void) {
    // Allocate memory for a new PCB
    struct pcb* curr_pcb = sys_alloc_mem(sizeof(struct pcb));

    // Check if allocation was successful
    if (curr_pcb == NULL) {
        sys_req(WRITE, COM1, "PCB allocation failed.", sizeof("PCB allocation failed."));
        return NULL;
    }

    // Initialize PCB fields
    curr_pcb->class = -1;      // Initialize class (or other fields) as needed
    curr_pcb->priority = -1;
    curr_pcb->state = -1;
    curr_pcb->next_pcb = NULL;
    curr_pcb->prev_pcb = NULL;

    return curr_pcb;
}

// Function to find a PCB by name
struct pcb* pcb_find(char* found_name) {
    struct pcb* curr_pcb = ready_head; // Start at the head of the ready queue

    // Loop through the ready queue
    while (curr_pcb != NULL) {
        // Check if the requested name is equivalent to the name of the current PCB
        if (strcmp(found_name, curr_pcb->name) == 0) {
            return curr_pcb; // Return the found PCB
        }
        // Move to the next PCB in the queue
        curr_pcb = curr_pcb->next_pcb;
    }

    curr_pcb = suspended_head; // Search the suspended queue if not found in ready queue

    // Loop through the suspended queue
    while (curr_pcb != NULL) {
        // Check if the requested name is equivalent to the name of the current PCB
        if (strcmp(found_name, curr_pcb->name) == 0)
        {
            return curr_pcb; // Return the found PCB
        }
        // Move to the next PCB in the queue
        curr_pcb = curr_pcb->next_pcb;

        if (curr_pcb == NULL) {
            // PCB with the given name was not found
            sys_req(WRITE, COM1, "A PCB with the given name cannot be found. Please try again.", sizeof("A PCB with the given name cannot be found. Please try again."));
            return NULL;
        }
    }

    return curr_pcb;
}

int pcb_free(struct pcb* old_pcb) {
    if (old_pcb != NULL) {
        // Free the PCB name memory, if it's not empty
        if (old_pcb->name[0] != '\0') {
            sys_free_mem(old_pcb->name);
        }
        // Free the PCB structure itself
        sys_free_mem(old_pcb);
        return 0;
    } 
    else {
        // PCB is NULL, display an error message
        sys_req(WRITE, COM1, "The PCB to be removed could not be found. Please try again.", sizeof("The PCB to be removed could not be found. Please try again."));
        return 1;
    }
}

void pcb_insert(struct pcb* curr_pcb) {
    struct pcb** head_ptr = NULL;

    if (curr_pcb == NULL)
        return;

    switch (curr_pcb->state) {
        case READY_STATE:
        case RESUME_STATE:
            head_ptr = &ready_head;
            break;
        case SUSPENDED_STATE:
            head_ptr = &suspended_head;
            break;
        case BLOCKED_STATE:
            head_ptr = &blocked_head;
            break;
        default:
            // Handle invalid state or other cases as needed
            return;
    }

    // Special case for an empty queue or higher priority
    if (*head_ptr == NULL || curr_pcb->priority < (*head_ptr)->priority) {
        curr_pcb->next_pcb = *head_ptr;
        *head_ptr = curr_pcb;
    } else {
        struct pcb* prev_pcb = *head_ptr;

        // Traverse the queue to find the appropriate position
        while (prev_pcb->next_pcb != NULL && curr_pcb->priority >= prev_pcb->next_pcb->priority) {
            prev_pcb = prev_pcb->next_pcb;
        }

        // Insert the new PCB after prev_pcb
        curr_pcb->next_pcb = prev_pcb->next_pcb;
        prev_pcb->next_pcb = curr_pcb;
    }
}

int pcb_remove(struct pcb* curr_pcb) {
    if (curr_pcb == NULL) {
        return PCB_ERROR;
    }

    if (curr_pcb->state == READY_STATE || curr_pcb->state == RESUME_STATE) {
        struct pcb** head_ptr = &ready_head;
        struct pcb* current = *head_ptr;

        // Special case for removing the head of the ready queue
        if (current == curr_pcb) {
            *head_ptr = current->next_pcb;
            return PCB_SUCCESS;
        }

        // Search for the PCB to be removed in the ready queue
        while (current != NULL && current->next_pcb != curr_pcb) {
            current = current->next_pcb;
        }

        if (current == NULL) {
            // The PCB to be removed was not found in the ready queue
            return PCB_ERROR;
        }

        // Remove the PCB from the ready queue
        current->next_pcb = curr_pcb->next_pcb;
        return PCB_SUCCESS;
    } 
    else if (curr_pcb->state == SUSPENDED_STATE) {
        struct pcb** head_ptr = &suspended_head;
        struct pcb* current = *head_ptr;

        // Special case for removing the head of the suspended queue
        if (current == curr_pcb) {
            *head_ptr = current->next_pcb;
            return PCB_SUCCESS;
        }

        // Search for the PCB to be removed in the suspended queue
        while (current != NULL && current->next_pcb != curr_pcb) {
            current = current->next_pcb;
        }

        if (current == NULL) {
            // The PCB to be removed was not found in the suspended queue
            return PCB_ERROR;
        }

        // Remove the PCB from the suspended queue
        current->next_pcb = curr_pcb->next_pcb;
        return PCB_SUCCESS;
    }

    return PCB_ERROR; // Invalid state
}

// Function to create and set up a new PCB
struct pcb* pcb_setup(const char* name, int class, int priority) {
    struct pcb* pcbPtr = pcb_allocate();
    // Check if memory was allocated successfully
    if (pcbPtr == NULL) {
        return NULL;
    }

    // Check for valid priority and class values
    if (priority < 0 || priority > 9) {
        pcb_free(pcbPtr);
        return NULL;
    }
    if (class != USER_PROC && class != SYSTEM_PROC) {
        pcb_free(pcbPtr);
        return NULL;
    }

    // Check if the PCB name is valid
    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN) {
        pcb_free(pcbPtr);
        return NULL;
    }

    // Allocate memory for the PCB name
    size_t name_length = strlen(name);
   
    // Manually copy the characters
    for (size_t i = 0; i <= name_length; i++) {
        pcbPtr->name[i] = name[i];
    }

    pcbPtr->priority = priority;
    pcbPtr->class = class;
    pcbPtr->state = READY_STATE; // or SUSPENDED_STATE, depending on the case
    pcbPtr->stack_top = (unsigned char *)(pcbPtr->stack + 1024 - sizeof(struct context));
    // Check if the PCB name is already in use
    if (pcb_find(pcbPtr->name) != NULL) {
        sys_req(WRITE, COM1, "Name already in use.\n", sizeof("Name already in use.\n"));
        pcb_free(pcbPtr);
        return NULL;
    }
    return pcbPtr;
}
