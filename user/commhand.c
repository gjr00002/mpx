#include <mpx/serial.h>
#include <mpx/io.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys_req.h>
#include <mpx/interrupts.h>
#include <memory.h>
#include <stdlib.h>
#include <mpx/pcb.h>
#include <processes.h>
#include <sys_call.h>

#define RTC_ADDR_PORT 0x70
#define RTC_DATA_PORT 0x71
#define BUFFER_OVERFLOW -1
#define SECONDS 0x00
#define ALARM_SEC 0x01
#define MINUTES 0x02
#define ALARM_MIN 0x03
#define HOURS 0x04
#define ALARM_HR 0x05
#define DAY 0x07
#define MONTH 0x08
#define YEAR 0x09
void show_blocked(void);
void show_ready(void);
char *get_time(void);


void alarm(void)
{
    char *input_time;
    char *alarm_name;
    char *alarm_msg = "ALARM";

    //Ask user to set alarm time
    char buf[20];
    char input_time_prompt[] = "\nPlease enter the time of the alarm (HH:MM:SS): ";

    sys_req(WRITE, COM1, input_time_prompt, sizeof(input_time_prompt));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    input_time = strtok(buf, "\n\r\t ");

    //Ask user to set unique time
    char name_prompt[] = "\nPlease enter the name of your alarm: ";
    sys_req(WRITE, COM1, name_prompt, sizeof(name_prompt));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    alarm_name = strtok(buf, "\n\r\t ");

    if (pcb_find(alarm_name)) {
        char error[] = "\nName already in use:";
        sys_req(WRITE, COM1, error, sizeof(error));
    }

    int get_time_int;
    get_time_int = atoi(get_time());
    int alarm_time_int;
    alarm_time_int = atoi(input_time);
 

    char hour[3], minutes[3], seconds[3];
    hour[0] = buf[0];
    hour[1] = buf[1];
    hour[2] = '\0';
    minutes[0] = buf[3];
    minutes[1] = buf[4];
    minutes[2] = '\0';
    seconds[0] = buf[6];
    minutes[1] = buf[7];
    minutes[2] = '\0';

    // Convert parsed strings to integers
    int alarm_hour = atoi(hour);
    int alarm_minutes = atoi(minutes);
    int alarm_seconds = atoi(seconds);

    // Validate the input (assuming a 24-hour clock)
    if (alarm_hour < 0 || alarm_hour > 23 || alarm_minutes < 0 || alarm_minutes > 59 || alarm_seconds < 0 || alarm_seconds > 59) {
        sys_req(WRITE, COM1, "\nInvalid time input. \n", strlen("\nInvalid time input. \n"));
        return;
    }

    // Set the alarm time
    //cli(); // Stop interrupts
    outb(RTC_ADDR_PORT, ALARM_HR);
    outb(RTC_DATA_PORT, alarm_hour);
    outb(RTC_ADDR_PORT, ALARM_MIN);
    outb(RTC_DATA_PORT, alarm_minutes);
    outb(RTC_ADDR_PORT, ALARM_SEC);
    outb(RTC_DATA_PORT, alarm_seconds);
    //sti(); // Start interrupts

    sys_req(WRITE, COM1, "\nTime set successfully. \n", strlen("\nTime set successfully. \n"));


    struct pcb *alarm_proc = pcb_setup(alarm_name, 1, 1);
    struct context *new_context1 = (struct context *)alarm_proc->stack_top;
    memset(new_context1, 0, sizeof(struct context));

    new_context1->cs = 0x08;
    new_context1->ds = 0x10;
    new_context1->es = 0x10;
    new_context1->fs = 0x10;
    new_context1->gs = 0x10;
    new_context1->ss = 0x10;
    new_context1->ebp = (uint32_t)(alarm_proc->stack);
    new_context1->esp = (uint32_t)(alarm_proc->stack_top);
    new_context1->eip = (uint32_t)alarm;
    new_context1->flags = 0x0202;

    pcb_insert(alarm_proc);
    sys_req(IDLE);

    if (get_time_int == alarm_time_int) {
        sys_req(WRITE, COM1, alarm_msg, sizeof(alarm_msg));
        sys_req(EXIT);
    }
}


/*
* Puts a process in the blocked state and moves it to the appropiate queue.
@params Process Name
@error Checks if name is valid
*/
/*void block_pcb(char* name) {

    char buf[20];
    char suspendPCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, suspendPCBName, sizeof(suspendPCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    name = strtok(buf, "\n\r\t ");

    // Check if the provided name is valid
    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN)
    {
        // Invalid name, send an error message to COM1
        const char *error_msg = "Invalid process name. \n";
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Attempt to find the PCB with the provided name
    struct pcb *found_pcb = pcb_find(name);

    // Check if the PCB with the provided name exists
    if (found_pcb == NULL)
    {
        // PCB not found, send an error message to COM1
        const char *error_msg = "Process not found. \n";
        // comment
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Check if it's a system process
    if (found_pcb->class == SYSTEM_PROC)
    {
        char system_process_msg[] = "\nSystem processes cannot be blocked. \n";
        sys_req(WRITE, COM1, system_process_msg, strlen(system_process_msg));
        return; // Return early if it's a system process
    }

    // Set the state to BLOCKED_STATE and move to blocked queue
    found_pcb->state = BLOCKED_STATE;
    pcb_insert(found_pcb); // Move to the blocked queue

    // Optionally, you can provide feedback that the process has been blocked
    const char *success_msg = "Process blocked successfully. \n";
    sys_req(WRITE, COM1, success_msg, strlen(success_msg));
}*/

/*
@params Process Name, Process Class, Process Priority
@checks Name must be unique and valid, Class must be valid, Priority must be valid.
*/
/*void create_pcb(void)
{
    char buf[20];
    char createpcbName[] = "Pick a name for the PCB: ";
    char createpcbClass[] = "Pick a Class for the PCB (0 for SYSTEM, 1 for USER): ";
    char createpcbPriority[] = "Pick a priority for the PCB (0-9): ";
    char *name = NULL;

    struct pcb *curr_pcb;

    // Ask the user to define PCB name
    sys_req(WRITE, COM1, createpcbName, sizeof(createpcbName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    strtok(buf, "\n\r\t ");
    // Allocate memory for name
    name = (char *)sys_alloc_mem(strlen(buf) + 1); // +1 for the null-terminator
    strcpy(name, buf);

    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) >= MAX_NAME_LEN)
    {
        char *invalid_name_msg = "Invalid PCB name. Name must be between 8 and 50 characters.\n";
        sys_req(WRITE, COM1, invalid_name_msg, strlen(invalid_name_msg));
        return;
    }

    // Ask the user to define PCB class
    sys_req(WRITE, COM1, createpcbClass, sizeof(createpcbClass) - 1);
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    strtok(buf, "\n\r\t ");
    int class = atoi(buf);

    if (class != USER_PROC && class != SYSTEM_PROC)
    {
        char *invalid_class_msg = "Invalid PCB class. Use 0 for SYSTEM or 1 for USER.\n";
        sys_req(WRITE, COM1, invalid_class_msg, strlen(invalid_class_msg));
        return;
    }

    // Ask the user to define PCB priority
    sys_req(WRITE, COM1, createpcbPriority, sizeof(createpcbPriority) - 1);
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    strtok(buf, "\n\r\t ");
    int priority = atoi(buf);

    if (priority < 0 || priority > 9)
    {
        char *invalid_priority_msg = "Invalid PCB priority. Priority must be between 0 and 9.\n";
        sys_req(WRITE, COM1, invalid_priority_msg, strlen(invalid_priority_msg));
        return;
    }
    else
    {
        // All checks passed, create the PCB
        curr_pcb = pcb_setup(name, class, priority);

        if (curr_pcb != NULL)
        {
            pcb_insert(curr_pcb);
            sys_req(WRITE, COM1, "PCB created successfully.\n", strlen("PCB created successfully.\n"));
        }
        else
        {
            sys_req(WRITE, COM1, "Failed to create PCB. Check your inputs and try again.\n", strlen("Failed to create PCB. Check your inputs and try again.\n"));
        }
    }
}*/

/*
@params Process Name
@checks Name must be valid, must not be a system process
*/
void delete_pcb(char *name)
{

    char buf[20];
    char deletePCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, deletePCBName, sizeof(deletePCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    name = strtok(buf, "\n\r\t ");

    // Check if the name is valid (you can add your own validation logic)
    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN)
    {
        char invalid_name_msg[] = "\nInvalid PCB name. \n";
        sys_req(WRITE, COM1, invalid_name_msg, strlen(invalid_name_msg));
        return; // Return early if the name is invalid
    }

    // Find the PCB with the given name
    struct pcb *found_pcb = pcb_find(name);

    // Check if the PCB was found
    if (found_pcb == NULL)
    {
        char not_found_msg[] = "\nPCB not found. \n";
        sys_req(WRITE, COM1, not_found_msg, strlen(not_found_msg));
        return; // Return early if the PCB wasn't found
    }

    // Check if it's a system process
    if (found_pcb->class == SYSTEM_PROC)
    {
        char system_process_msg[] = "\nSystem processes cannot be deleted. \n";
        sys_req(WRITE, COM1, system_process_msg, strlen(system_process_msg));
        return; // Return early if it's a system process
    }

    // Free the memory associated with the PCB
    int free_result = pcb_remove(found_pcb);

    // Check if freeing was successful
    if (free_result != 0)
    {
        char free_failed_msg[] = "\nFailed to free PCB memory. \n";
        sys_req(WRITE, COM1, free_failed_msg, strlen(free_failed_msg));
    }
    else
    {
        char success_msg[] = "\nPCB deleted successfully. \n";
        sys_req(WRITE, COM1, success_msg, strlen(success_msg));
    }
}

// get date command
void get_date(void)
{

    uint8_t day, month, year;

    // Read day, month, and year from RTC registers
    outb(RTC_ADDR_PORT, DAY);
    day = inb(RTC_DATA_PORT);

    outb(RTC_ADDR_PORT, MONTH);
    month = inb(RTC_DATA_PORT);

    outb(RTC_ADDR_PORT, YEAR);
    year = inb(RTC_DATA_PORT);

    // Format the date into a string like "DD/MM/YY"
    char dateStr[12]; // "\nDD/MM/YY\n\0"
    dateStr[0] = '\n';
    dateStr[1] = ((day >> 4) & 0x0F) + '0'; // Convert BCD to binary and then to ASCII
    dateStr[2] = (day & 0x0F) + '0';
    dateStr[3] = '/';
    dateStr[4] = ((month >> 4) & 0x0F) + '0';
    dateStr[5] = (month & 0x0F) + '0';
    dateStr[6] = '/';
    dateStr[7] = ((year >> 4) & 0x0F) + '0';
    dateStr[8] = (year & 0x0F) + '0';
    dateStr[9] = '\n'; // Add a newline character
    dateStr[10] = '\0';

    // Write the formatted date to the screen
    sys_req(WRITE, COM1, dateStr, strlen(dateStr));
}

char *get_time(void)
{
    uint8_t hours, minutes, seconds;

    // Read hours, minutes, and seconds from RTC registers
    outb(RTC_ADDR_PORT, 0x04); // Hours register (24-hour format)
    hours = inb(RTC_DATA_PORT);

    outb(RTC_ADDR_PORT, 0x02); // Minutes register
    minutes = inb(RTC_DATA_PORT);

    outb(RTC_ADDR_PORT, 0x00); // Seconds register
    seconds = inb(RTC_DATA_PORT);

    // Convert BCD values to binary
    hours = (hours & 0x0F) + ((hours >> 4) * 10);
    minutes = (minutes & 0x0F) + ((minutes >> 4) * 10);
    seconds = (seconds & 0x0F) + ((seconds >> 4) * 10);

    // Apply UTC offset for EST (Eastern Standard Time)
    hours -= 4; // Subtract 4 hours to convert from UTC to EST
    if (hours < 0)
    {
        hours += 24; // Ensure hours are in the range 0-23
    }

    // Allocate memory for the time string using sys_alloc_mem
    char *timeStr = (char *)sys_alloc_mem(12); // "\nHH:MM:SS\n\0"

    if (timeStr == NULL)
    {
        // Handle allocation failure if necessary
        return NULL;
    }

    // Format the time into the string
    timeStr[0] = '\n';
    timeStr[1] = '0' + (hours / 10);
    timeStr[2] = '0' + (hours % 10);
    timeStr[3] = ':';
    timeStr[4] = '0' + (minutes / 10);
    timeStr[5] = '0' + (minutes % 10);
    timeStr[6] = ':';
    timeStr[7] = '0' + (seconds / 10);
    timeStr[8] = '0' + (seconds % 10);
    timeStr[9] = '\n';
    timeStr[10] = '\0';

    // Print it using sys_req
    sys_req(WRITE, COM1, timeStr, strlen(timeStr));

    // Return the dynamically allocated string
    return timeStr;
}

// help command
void help(void)
{
    char *alarm_info = "\nThe 'alarm' command puts a process in the blocked state and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, alarm_info, strlen(alarm_info));

    /*char *block_pcb_info = "\nThe 'block pcb' command puts a process in the blocked state and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, block_pcb_info, strlen(block_pcb_info));*/

    /*char *create_pcb_info = "\nThe 'create pcb' command creates a PCB and inserts it into the appropriate queue.";
    sys_req(WRITE, COM1, create_pcb_info, strlen(create_pcb_info));*/

    char *delete_pcb_info = "\nThe 'delete pcb' command removes the requested process from the queue and frees the associate memory.";
    sys_req(WRITE, COM1, delete_pcb_info, strlen(delete_pcb_info));

    char *get_date_info = "\nThe 'getdate' command retrieves the date.";
    sys_req(WRITE, COM1, get_date_info, strlen(get_date_info));

    char *get_time_info = "\nThe 'gettime' command retrieves the time.";
    sys_req(WRITE, COM1, get_time_info, strlen(get_time_info));

    char *load_r3_info = "\nThe 'loadR3' command loads and queues predetermined processes.";
    sys_req(WRITE, COM1, load_r3_info, strlen(load_r3_info));

    /*char *unblock_pcb_info = "\n'unblockPCB' command puts the process in the ready state and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, unblock_pcb_info, strlen(unblock_pcb_info));*/

    char *resume_pcb_info = "\nThe 'resumePCB' command unsuspends the process and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, resume_pcb_info, strlen(resume_pcb_info));

    char *set_date_info = "\nThe 'setdate' command allows user to set the date.";
    sys_req(WRITE, COM1, set_date_info, strlen(set_date_info));

    char *set_time_info = "\nThe 'settime' command allows the user to set the time.";
    sys_req(WRITE, COM1, set_time_info, strlen(set_time_info));

    char *set_pcb_priority_info = "\nThe 'setPCBpriority' command changes the priority of the process and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, set_pcb_priority_info, strlen(set_pcb_priority_info));

    char *show_all_info = "\nThe 'show all' command shows all of the following information for all processes in any state: name, class, state, suspended status and priority.";
    sys_req(WRITE, COM1, show_all_info, strlen(show_all_info));

    /*char *show_blocked_info = "\nThe 'show blocked' command shows all of the following information for all processes in the blocked state: name, class, state, suspended status and priority.";
    sys_req(WRITE, COM1, show_blocked_info, strlen(show_blocked_info));*/

    char *show_pcb_info = "\nThe 'showPCB' command displays the process's name, class, state, suspend status and priority.";
    sys_req(WRITE, COM1, show_pcb_info, strlen(show_pcb_info));

    char *show_ready_info = "\nThe 'show ready' command shows all of the following information for all processes in the ready state: name, class, state, suspended status and priority.";
    sys_req(WRITE, COM1, show_ready_info, strlen(show_ready_info));

    char *shutdown_info = "\nThe 'shutdown' command exits to the main function and requires confirmation.";
    sys_req(WRITE, COM1, shutdown_info, strlen(shutdown_info));

    char *suspend_pcb_info = "\nThe 'suspendPCB' command suspends the process and moves it to the appropriate queue.";
    sys_req(WRITE, COM1, suspend_pcb_info, strlen(suspend_pcb_info));

    char *version_info = "\nThe 'version' command prints the current version of MPX and the compilation date.";
    sys_req(WRITE, COM1, version_info, strlen(version_info));
}

void load_r3(void)
{
    // struct pcb* newer_pcb= NULL;
    for (int i = 0; i <= 5; i++)
    {
        if (i == 1)
        {
            struct pcb *new_pcb1 = pcb_setup("process1", 1, 1);

            struct context *new_context1 = (struct context *)new_pcb1->stack_top;
            memset(new_context1, 0, sizeof(struct context));
            new_context1->cs = 0x08;
            new_context1->ds = 0x10;
            new_context1->es = 0x10;
            new_context1->fs = 0x10;
            new_context1->gs = 0x10;
            new_context1->ss = 0x10;
            new_context1->ebp = (uint32_t)(new_pcb1->stack);
            new_context1->esp = (uint32_t)(new_pcb1->stack_top);
            new_context1->eip = (uint32_t)proc1;
            new_context1->flags = 0x0202;
            pcb_insert(new_pcb1);
        }
        else if (i == 2)
        {
            struct pcb *new_pcb2 = pcb_setup("process2", 1, 2);
            struct context *new_context2 = (struct context *)new_pcb2->stack_top;
            memset(new_context2, 0, sizeof(&new_context2));
            new_context2->cs = 0x08;
            new_context2->ds = 0x10;
            new_context2->es = 0x10;
            new_context2->fs = 0x10;
            new_context2->gs = 0x10;
            new_context2->ss = 0x10;
            new_context2->ebp = (uint32_t)(new_pcb2->stack);
            new_context2->esp = (uint32_t)(new_pcb2->stack_top);
            new_context2->eip = (uint32_t)proc2;
            new_context2->flags = 0x0202;
            pcb_insert(new_pcb2);
        }
        else if (i == 3)
        {
            struct pcb *new_pcb3 = pcb_setup("process3", 1, 3);
            struct context *new_context3 = (struct context *)new_pcb3->stack_top;
            memset(new_context3, 0, sizeof(&new_context3));
            new_context3->cs = 0x08;
            new_context3->ds = 0x10;
            new_context3->es = 0x10;
            new_context3->fs = 0x10;
            new_context3->gs = 0x10;
            new_context3->ss = 0x10;
            new_context3->ebp = (uint32_t)(new_pcb3->stack);
            new_context3->esp = (uint32_t)(new_pcb3->stack_top);
            new_context3->eip = (uint32_t)proc3;
            new_context3->flags = 0x0202;
            pcb_insert(new_pcb3);
        }
        else if (i == 4)
        {
            struct pcb *new_pcb4 = pcb_setup("process4", 1, 4);
            struct context *new_context4 = (struct context *)new_pcb4->stack_top;
            memset(new_context4, 0, sizeof(&new_context4));
            new_context4->cs = 0x08;
            new_context4->ds = 0x10;
            new_context4->es = 0x10;
            new_context4->fs = 0x10;
            new_context4->gs = 0x10;
            new_context4->ss = 0x10;
            new_context4->ebp = (uint32_t)(new_pcb4->stack);
            new_context4->esp = (uint32_t)(new_pcb4->stack_top);
            new_context4->eip = (uint32_t)proc4;
            new_context4->flags = 0x0202;
            pcb_insert(new_pcb4);
        }
        else if (i == 5)
        {
            struct pcb *new_pcb5 = pcb_setup("process5", 1, 5);
            struct context *new_context5 = (struct context *)new_pcb5->stack_top;
            memset(new_context5, 0, sizeof(&new_context5));
            new_context5->cs = 0x08;
            new_context5->ds = 0x10;
            new_context5->es = 0x10;
            new_context5->fs = 0x10;
            new_context5->gs = 0x10;
            new_context5->ss = 0x10;
            new_context5->ebp = (uint32_t)(new_pcb5->stack);
            new_context5->esp = (uint32_t)(new_pcb5->stack_top);
            new_context5->eip = (uint32_t)proc5;
            new_context5->flags = 0x0202;
            pcb_insert(new_pcb5);
        }
    }
}

// menu for user to choose options
void menu(void) {

    // printed menu
    char *introduction = "\n\nHello! Please type one of the following from the menu.";
    sys_req(WRITE, COM1, introduction, strlen(introduction));

    char *alarm_prompt = "\nType 'alarm'";
    sys_req(WRITE, COM1, alarm_prompt, strlen(alarm_prompt));

    /*char *blockPCB_prompt = "\nType 'blockPCB'";
    sys_req(WRITE, COM1, blockPCB_prompt, strlen(blockPCB_prompt));*/

    /*char *createPCB_prompt = "\nType 'createPCB'";
    sys_req(WRITE, COM1, createPCB_prompt, strlen(createPCB_prompt));*/

    char *deletePCB_prompt = "\nType 'deletePCB'";
    sys_req(WRITE, COM1, deletePCB_prompt, strlen(deletePCB_prompt));

    char *help_prompt = "\nType 'help'";
    sys_req(WRITE, COM1, help_prompt, strlen(help_prompt));

    char *getdate_prompt = "\nType 'getdate'";
    sys_req(WRITE, COM1, getdate_prompt, strlen(getdate_prompt));

    char *gettime_prompt = "\nType 'gettime'";
    sys_req(WRITE, COM1, gettime_prompt, strlen(getdate_prompt));

    char *load_r3_prompt = "\nType 'loadR3'";
    sys_req(WRITE, COM1, load_r3_prompt, strlen(load_r3_prompt));

    char *resumePCB_prompt = "\nType 'resumePCB'";
    sys_req(WRITE, COM1, resumePCB_prompt, strlen(resumePCB_prompt));

    char *setdate_prompt = "\nType 'setdate'";
    sys_req(WRITE, COM1, setdate_prompt, strlen(setdate_prompt));

    char *setPCBpriority_prompt = "\nType 'setPCBpriority'";
    sys_req(WRITE, COM1, setPCBpriority_prompt, strlen(setPCBpriority_prompt));

    char *settime_prompt = "\nType 'settime'";
    sys_req(WRITE, COM1, settime_prompt, strlen(settime_prompt));

    char *showall_prompt = "\nType 'showall'";
    sys_req(WRITE, COM1, showall_prompt, strlen(showall_prompt));

    /*char *showblocked_prompt = "\nType 'showblocked'";
    sys_req(WRITE, COM1, showblocked_prompt, strlen(showblocked_prompt));*/

    char *showPCB_prompt = "\nType 'showPCB'";
    sys_req(WRITE, COM1, showPCB_prompt, strlen(showPCB_prompt));

    char *showready_prompt = "\nType 'showready'";
    sys_req(WRITE, COM1, showready_prompt, strlen(showready_prompt));

    char *shutdown_prompt = "\nType 'shutdown'";
    sys_req(WRITE, COM1, shutdown_prompt, strlen(shutdown_prompt));

    char *suspendPCB_prompt = "\nType 'suspendPCB'";
    sys_req(WRITE, COM1, suspendPCB_prompt, strlen(suspendPCB_prompt));

    /*char *unblockPCB_prompt = "\nType 'unblockPCB'";
    sys_req(WRITE, COM1, unblockPCB_prompt, strlen(unblockPCB_prompt));*/

    char *version_prompt = "\nType 'version'";
    sys_req(WRITE, COM1, version_prompt, strlen(version_prompt));

    char *prompt = "\nPlease enter a command here : ";
    sys_req(WRITE, COM1, prompt, strlen(prompt));
}

/*
* Puts a process in the not suspended state, and moves it the appropriate queue
@params Name
@error Name Must be valid
*/
void resume_pcb(char *name)
{

    char buf[20];
    char suspendPCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, suspendPCBName, sizeof(suspendPCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    name = strtok(buf, "\n\r\t ");
    // Check if the provided name is valid
    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN)
    {
        // Invalid name, send an error message to COM1
        const char *error_msg = "Invalid process name. \n";
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Attempt to find the PCB with the provided name
    struct pcb *found_pcb = pcb_find(name);

    // Check if the PCB with the provided name exists
    if (found_pcb == NULL)
    {
        // PCB not found, send an error message to COM1
        const char *error_msg = "Process not found. \n";
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Check if it's a system process
    if (found_pcb->class == SYSTEM_PROC)
    {
        char system_process_msg[] = "\nSystem processes cannot be deleted. \n";
        sys_req(WRITE, COM1, system_process_msg, strlen(system_process_msg));
        return; // Return early if it's a system process
    }

    // Set the state to RESUME_STATE and move to resume queue
    found_pcb->state = RESUME_STATE;
    pcb_insert(found_pcb); // Move to the resume queue

    // Optionally, you can provide feedback that the process has been resumed
    const char *success_msg = "Process resumed successfully. \n";
    sys_req(WRITE, COM1, success_msg, strlen(success_msg));
}

// set date command
void set_date(void)
{
    // Stop interrupts
    cli();

    // Prompt the user to enter a date in the format "DD/MM/YY: "
    char prompt[] = "\nEnter date (DD/MM/YY): ";
    sys_req(WRITE, COM1, prompt, strlen(prompt));

    // Read user input for the date
    char buf[12];
    int nread = serial_poll(COM1, buf, sizeof(buf));

    if (nread == BUFFER_OVERFLOW)
    {
        char overflow_msg[] = "\nInput buffer overflow. \n";
        sys_req(WRITE, COM1, overflow_msg, strlen(overflow_msg));
        sti(); // Start interrupts before returning
        return;
    }

    // Null-terminate the user input
    buf[nread] = '\0';

    // Remove leading and trailing whitespace characters (including '\n')
    char *trimmed_input = strtok(buf, "\n\r\t "); // Tokenize by whitespace and newline

    // Check if the input has the correct format (DD/MM/YY)
    if (strlen(trimmed_input) != 8 || trimmed_input[2] != '/' || trimmed_input[5] != '/')
    {
        char invalid_msg[] = "\nInvalid date format. Please use DD/MM/YY. \n";
        sys_req(WRITE, COM1, invalid_msg, strlen(invalid_msg));
        sti(); // Start interrupts before returning
        return;
    }

    // Extract the day, month, and year components manually
    char day_str[3];
    char month_str[3];
    char year_str[3];

    day_str[0] = trimmed_input[0];
    day_str[1] = trimmed_input[1];
    day_str[2] = '\0';

    month_str[0] = trimmed_input[3];
    month_str[1] = trimmed_input[4];
    month_str[2] = '\0';

    year_str[0] = trimmed_input[6];
    year_str[1] = trimmed_input[7];
    year_str[2] = '\0';

    // Convert the extracted strings to integers
    int day = atoi(day_str);
    int month = atoi(month_str);
    int year = atoi(year_str);

    // Validate the date (basic checks)
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99)
    {
        char invalid_date_msg[] = "\nInvalid date. Please check the values. \n";
        sys_req(WRITE, COM1, invalid_date_msg, strlen(invalid_date_msg));
        sti(); // Start interrupts before returning
        return;
    }

    // Update the RTC date
    outb(RTC_ADDR_PORT, DAY);
    outb(RTC_DATA_PORT, (char)((day / 10) << 4 | (day % 10))); // Convert to BCD

    outb(RTC_ADDR_PORT, MONTH);
    outb(RTC_DATA_PORT, (char)((month / 10) << 4 | (month % 10))); // Convert to BCD

    outb(RTC_ADDR_PORT, YEAR);
    outb(RTC_DATA_PORT, (char)((year / 10) << 4 | (year % 10))); // Convert to BCD

    // Restart interrupts
    sti();

    char success_msg[] = "\nDate set successfully. \n";
    sys_req(WRITE, COM1, success_msg, strlen(success_msg));
}

void set_pcb_priority(char *name, int new_priority)
{

    char buf[20];
    char PCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, PCBName, sizeof(PCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    char *copy = strtok(buf, "\n\r\t ");

    // Allocate memory for name
    name = (char *)sys_alloc_mem(strlen(buf) + 1); // +1 for the null-terminator
    strcpy(name, copy);
    memset(buf, 0, sizeof(buf));

    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN)
    {
        char invalid_name_msg[] = "\nInvalid PCB name.";
        sys_req(WRITE, COM1, invalid_name_msg, strlen(invalid_name_msg));
        return; // Return early if the name is invalid
    }

    char PCBPriority[] = "Please enter the new priority of the PCB: ";

    sys_req(WRITE, COM1, PCBPriority, sizeof(PCBPriority));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    new_priority = atoi(buf);

    if (new_priority < 0 || new_priority > 9)
    {
        char invalid_pri_msg[] = "\nInvalid priority, please assign a priority in range 0-9.";
        sys_req(WRITE, COM1, invalid_pri_msg, strlen(invalid_pri_msg));
        return; // Return early if the name is invalid
    }

    struct pcb *curr_pcb = pcb_find(name);

    curr_pcb->priority = new_priority;
}

// set time command
void set_time(void)
{
    char buf[20];
    sys_req(WRITE, COM1, "\nEnter time (HH:MM:SS): ", strlen("\nEnter time (HH:MMSs): "));

    // Read user input for time
    int nread = serial_poll(COM1, buf, sizeof(buf));

    if (nread == BUFFER_OVERFLOW)
    {
        sys_req(WRITE, COM1, "\nInput buffer overflow. \n", strlen("\nInput buffer overflow. \n"));
        return;
    }

    if (nread < 5)
    {
        sys_req(WRITE, COM1, "\nInvalid input format. \n", strlen("\nInvalid input format. \n"));
        return;
    }

    // Parse hours and minutes from the input
    char hour[3], minutes[3], seconds[3];
    hour[0] = buf[0];
    hour[1] = buf[1];
    hour[2] = '\0';
    minutes[0] = buf[3];
    minutes[1] = buf[4];
    minutes[2] = '\0';
    seconds[0] = buf[6];
    minutes[1] = buf[7];
    minutes[2] = '\0';

    // Convert parsed strings to integers
    int input_hour = atoi(hour);
    int input_minutes = atoi(minutes);
    int input_seconds = atoi(seconds);

    // Validate the input (assuming a 24-hour clock)
    if (input_hour < 0 || input_hour > 23 || input_minutes < 0 || input_minutes > 59 || input_seconds < 0 || input_seconds > 59)
    {
        sys_req(WRITE, COM1, "\nInvalid time input. \n", strlen("\nInvalid time input. \n"));
        return;
    }

    // Set the RTC time
    cli(); // Stop interrupts
    outb(RTC_ADDR_PORT, HOURS);
    outb(RTC_DATA_PORT, input_hour);
    outb(RTC_ADDR_PORT, MINUTES);
    outb(RTC_DATA_PORT, input_minutes);
    outb(RTC_ADDR_PORT, SECONDS);
    outb(RTC_DATA_PORT, input_seconds);
    sti(); // Start interrupts

    sys_req(WRITE, COM1, "\nTime set successfully. \n", strlen("\nTime set successfully. \n"));
}

/*
 * For all processes (in any state), display the process's
 * Name, Class, State, Suspend Status, Priority
 * @param NONE
 * @checks None
 */
void show_all(void)
{
    struct pcb *current_pcb;

    // Iterate through each queue
    struct pcb *all_list[] = {ready_head, blocked_head, suspended_head};

    for (int index = 0; index < 3; index++)
    {
        current_pcb = all_list[index];

        while (current_pcb != NULL)
        {
            // Print process information
            sys_req(WRITE, COM1, "\nName: ", strlen("\nName: "));
            sys_req(WRITE, COM1, current_pcb->name, strlen(current_pcb->name));
            sys_req(WRITE, COM1, "\nClass: ", strlen("\nClass: "));
            sys_req(WRITE, COM1, (current_pcb->class == USER_PROC) ? "User" : "System", strlen("User"));
            sys_req(WRITE, COM1, "\nState: ", strlen("\nState: "));
            if (current_pcb->state == READY_STATE)
            {
                sys_req(WRITE, COM1, "READY", strlen("READY"));
            }
            else if (current_pcb->state == BLOCKED_STATE)
            {
                sys_req(WRITE, COM1, "BLOCKED", strlen("BLOCKED"));
            }
            else if (current_pcb->state == SUSPEND_STATE)
            {
                sys_req(WRITE, COM1, "SUSPENDED", strlen("SUSPENDED"));
            }
            else if (current_pcb->state == RESUME_STATE)
            {
                sys_req(WRITE, COM1, "RESUMED", strlen("RESUMED"));
            }
            sys_req(WRITE, COM1, "\nPriority: ", strlen("\nPriority: "));
            char priority_char = '0' + current_pcb->priority;
            sys_req(WRITE, COM1, &priority_char, 1); // Display priority as a single character
            sys_req(WRITE, COM1, "\n", strlen("\n"));

            // Move to the next PCB
            current_pcb = current_pcb->next_pcb;
        }
    }
}

/*void show_blocked(void)
{

    // Display information for blocked processes
    struct pcb *current_pcb = blocked_head;

    if (current_pcb == NULL)
    {
        sys_req(WRITE, COM1, "\nThere are no processes currently blocked.", strlen("\nThere are no processes currently blocked."));
    }

    while (current_pcb != NULL)
    {
        // diplay the name, class, state and priority to the user
        sys_req(WRITE, COM1, "\nName: ", strlen("\nName: "));
        sys_req(WRITE, COM1, current_pcb->name, strlen(current_pcb->name));
        sys_req(WRITE, COM1, "\nClass: ", strlen("\nClass: "));
        sys_req(WRITE, COM1, (current_pcb->class == USER_PROC) ? "User" : "System", strlen("User"));
        sys_req(WRITE, COM1, "\nState: ", strlen("\nState: "));
        if (current_pcb->state == READY_STATE)
        {
            sys_req(WRITE, COM1, "READY", strlen("READY"));
        }
        else if (current_pcb->state == BLOCKED_STATE)
        {
            sys_req(WRITE, COM1, "BLOCKED", strlen("BLOCKED"));
        }
        else if (current_pcb->state == SUSPEND_STATE)
        {
            sys_req(WRITE, COM1, "SUSPENDED", strlen("SUSPENDED"));
        }
        else if (current_pcb->state == RESUME_STATE)
        {
            sys_req(WRITE, COM1, "RESUMED", strlen("RESUMED"));
        }
        sys_req(WRITE, COM1, "\nPriority: ", strlen("\nPriority: "));
        char priority_char = '0' + current_pcb->priority;
        sys_req(WRITE, COM1, &priority_char, 1); // Display priority as a single character
        sys_req(WRITE, COM1, "\n", strlen("\n"));

        current_pcb = current_pcb->next_pcb;
    }
}*/

void show_pcb(char *name) {

    char buf[20];
    char class_str[20];
    char state_str[20];
    char priority_str[20];
    char showPCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, showPCBName, sizeof(showPCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    name = strtok(buf, "\n\r\t ");

    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN) {
        char invalid_name_msg[] = "\nInvalid PCB name. \n";
        sys_req(WRITE, COM1, invalid_name_msg, strlen(invalid_name_msg));
        return; // Return early if the name is invalid
    }

    struct pcb *found_pcb = pcb_find(name);

    itoa(found_pcb->class, class_str, 10);
    itoa(found_pcb->state, state_str, 10);
    itoa(found_pcb->priority, priority_str, 10);

    sys_req(WRITE, COM1, "\n", strlen("\n"));
    sys_req(WRITE, COM1, "Name: ", strlen("Name: "));
    sys_req(WRITE, COM1, found_pcb->name, sizeof(found_pcb->name));
    sys_req(WRITE, COM1, "\n", strlen("\n"));
    sys_req(WRITE, COM1, "Class: ", strlen("Class: "));
    sys_req(WRITE, COM1, class_str, strlen(class_str));
    sys_req(WRITE, COM1, "\n", strlen("\n"));
    sys_req(WRITE, COM1, "State: ", strlen("State: "));
    sys_req(WRITE, COM1, state_str, strlen(state_str));
    sys_req(WRITE, COM1, "\n", strlen("\n"));
    sys_req(WRITE, COM1, "Priority: ", strlen("Priority: "));
    sys_req(WRITE, COM1, priority_str, strlen(priority_str));
    sys_req(WRITE, COM1, "\n", strlen("\n"));
}

void show_ready(void) {

    // Display information for ready processes
    struct pcb *current_pcb = ready_head;

    if (current_pcb == NULL) {
        sys_req(WRITE, COM1, "\nThere are no processes currently ready.", strlen("\nThere are no processes currently ready."));
    }

    while (current_pcb != NULL) {
        sys_req(WRITE, COM1, "\nName: ", strlen("\nName: "));
        sys_req(WRITE, COM1, current_pcb->name, strlen(current_pcb->name));
        sys_req(WRITE, COM1, "\nClass: ", strlen("\nClass: "));
        sys_req(WRITE, COM1, (current_pcb->class == USER_PROC) ? "User" : "System", strlen("User"));
        sys_req(WRITE, COM1, "\nState: ", strlen("\nState: "));
        if (current_pcb->state == READY_STATE) {
            sys_req(WRITE, COM1, "READY", strlen("READY"));
        }
        else if (current_pcb->state == BLOCKED_STATE) {
            sys_req(WRITE, COM1, "BLOCKED", strlen("BLOCKED"));
        }
        else if (current_pcb->state == SUSPEND_STATE) {
            sys_req(WRITE, COM1, "SUSPENDED", strlen("SUSPENDED"));
        }
        else if (current_pcb->state == RESUME_STATE) {
            sys_req(WRITE, COM1, "RESUMED", strlen("RESUMED"));
        }
        sys_req(WRITE, COM1, "\nPriority: ", strlen("\nPriority: "));
        char priority_char = '0' + current_pcb->priority;
        sys_req(WRITE, COM1, &priority_char, 1); // Display priority as a single character
        sys_req(WRITE, COM1, "\n", strlen("\n"));

        current_pcb = current_pcb->next_pcb;
    }
}

void shutdown(void) {
    char shutdown_prompt[] = "\nWould you like to initiate system shutdown? (yes/no): ";
    sys_req(WRITE, COM1, shutdown_prompt, sizeof(shutdown_prompt) - 1); // Exclude the null terminator

    char buf[10];
    int shutdown_input = serial_poll(COM1, buf, sizeof(buf));

    // Null-terminate the user input
    buf[shutdown_input] = '\0';

    // Remove leading and trailing whitespace characters (including '\n')
    char *input = strtok(buf, "\n\r\t "); // Tokenize by whitespace and newline

    if (input != NULL) {
        // Check if the input is "yes" (case insensitive)
        if (strcmp(input, "yes") == 0) {
            // Perform system-wide cleanup here (e.g., release resources)
            // ...

            // Notify the user of the shutdown process
            char shutting_down_msg[] = "\nShutting down the system...\n";
            sys_req(WRITE, COM1, shutting_down_msg, sizeof(shutting_down_msg) - 1);

            sys_req(EXIT);
        }
        else {
            // Handle other cases (e.g., "no" or invalid input)
            char not_shutdown[] = "\nShutdown canceled or invalid input. \n";
            sys_req(WRITE, COM1, not_shutdown, sizeof(not_shutdown) - 1);
        }
    }
}

/*
*Puts a process in the suspended state, and moves it the appropriate queue
@params Name
@errors Name must be valid, must not be a system process
*/
void suspend_pcb(char *name) {

    char buf[20];
    char suspendPCBName[] = "Please enter the name of the PCB: ";

    sys_req(WRITE, COM1, suspendPCBName, sizeof(suspendPCBName));
    memset(buf, 0, sizeof(buf));
    serial_poll(COM1, buf, sizeof(buf));
    name = strtok(buf, "\n\r\t ");

    // Check if the provided name is valid
    if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN) {
        // Invalid name, send an error message to COM1
        const char *error_msg = "Invalid process name. \n";
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Attempt to find the PCB with the provided name
    struct pcb *found_pcb = pcb_find(name);

    // Check if the PCB with the provided name exists
    if (found_pcb == NULL) {
        // PCB not found, send an error message to COM1
        const char *error_msg = "Process not found. \n";
        sys_req(WRITE, COM1, error_msg, strlen(error_msg));
        return; // Exit the function due to the error
    }

    // Check if it's a system process
    if (found_pcb->class == SYSTEM_PROC) {
        char system_process_msg[] = "\nSystem processes cannot be deleted. \n";
        sys_req(WRITE, COM1, system_process_msg, strlen(system_process_msg));
        return; // Return early if it's a system process
    }

    // Set the state to SUSPEND_STATE and move to suspended queue
    found_pcb->state = SUSPEND_STATE;
    pcb_insert(found_pcb); // Move to the suspended queue

    // Optionally, you can provide feedback that the process has been blocked
    const char *success_msg = "Process suspended successfully. \n";
    sys_req(WRITE, COM1, success_msg, strlen(success_msg));
}

/*
* Puts a process in the unblocked (ready) state, and moves it to the approriate queue
@param Name
@error name must be valid
*/
void unblock_pcb(pcb* pcb) {

    //char buf[20];
    //char suspendPCBName[] = "Please enter the name of the PCB: ";

    //sys_req(WRITE, COM1, suspendPCBName, sizeof(suspendPCBName));
    //memset(buf, 0, sizeof(buf));
    //serial_poll(COM1, buf, sizeof(buf));
    //name = strtok(buf, "\n\r\t ");
    //// Check if the provided name is valid
    //if (name == NULL || strlen(name) < MIN_NAME_LEN || strlen(name) > MAX_NAME_LEN)
    //{
    //    // Invalid name, send an error message to COM1
    //    const char *error_msg = "Invalid process name. \n";
    //    sys_req(WRITE, COM1, error_msg, strlen(error_msg));
    //    return; // Exit the function due to the error
    //}

    //// Attempt to find the PCB with the provided name
    //struct pcb *found_pcb = pcb_find(name);

    //// Check if the PCB with the provided name exists
    //if (found_pcb == NULL)
    //{
    //    // PCB not found, send an error message to COM1
    //    const char *error_msg = "Process not found. \n";
    //    sys_req(WRITE, COM1, error_msg, strlen(error_msg));
    //    return; // Exit the function due to the error
    //}

    //// Check if it's a system process
    //if (found_pcb->class == SYSTEM_PROC)
    //{
    //    char system_process_msg[] = "\nSystem processes cannot be deleted. \n";
    //    sys_req(WRITE, COM1, system_process_msg, strlen(system_process_msg));
    //    return; // Return early if it's a system process
    //}

    // Set the state to READY_STATE and move to blocked queue
    pcb->state = READY_STATE;
    pcb_insert(pcb); // Move to the ready queue

    // Optionally, you can provide feedback that the process has been blocked
    //const char *success_msg = "Process UNBLOCKED (ready) successfully. \n";
    //sys_req(WRITE, COM1, success_msg, strlen(success_msg));
}

// version command
void version(void) {
    char *version_info = "\nThe current version of MPX is: R6.";
    sys_req(WRITE, COM1, version_info, strlen(version_info));
}

void commhand(void) {
    char buf[20];
    int nread;
    char *name = NULL;    // this needs to be changed too it is a place holder functions need params removed more than likely
    int new_priority = 5; // params need to be changed more than likely
    for (;;) {
        menu(); // Display the menu before waiting for a command

        memset(buf, 0, sizeof(buf)); // Clear the buffer

        // Read input using serial_poll
        nread = sys_req(READ, COM1, buf, 1);
        if (nread == BUFFER_OVERFLOW) {
            char *buffer_msg = "Buffer Overflow\n";
            sys_req(WRITE, COM1, buffer_msg, strlen(buffer_msg));
            continue; // Continue to read the next command
        }
        // Remove leading and trailing whitespace characters (including '\n')
        char *input = strtok(buf, "\n\r\t "); // Tokenize by whitespace and newline

        // Process the command
        if (input != NULL) {
            if (strcmp(input, "alarm") == 0) {
                alarm();
            }
            /*else if (strcmp(input, "blockPCB") == 0) {
                block_pcb(name);
            }*/
            /*else if (strcmp(input, "createPCB") == 0) {
                create_pcb();
            }*/
            else if (strcmp(input, "deletePCB") == 0) {
                delete_pcb(name);
            }
            else if (strcmp(input, "getdate") == 0) {
                get_date();
            }
            else if (strcmp(input, "gettime") == 0) {
                get_time();
            }
            else if (strcmp(input, "help") == 0) {
                help();
            }
            else if (strcmp(input, "loadR3") == 0) {
                load_r3();
            }
            /*else if (strcmp(input, "unblockPCB") == 0) {
                unblock_pcb(name);
            }*/
            else if (strcmp(input, "resumePCB") == 0) {
                resume_pcb(name);
            }
            else if (strcmp(input, "setdate") == 0) {
                set_date();
            }
            else if (strcmp(input, "setPCBpriority") == 0) {
                set_pcb_priority(name, new_priority);
            }
            else if (strcmp(input, "settime") == 0) {
                set_time();
            }
            else if (strcmp(input, "showall") == 0) {
                show_all();
            }
            /*else if (strcmp(input, "showblocked") == 0) {
                show_blocked();
            }*/
             else if (strcmp(input, "showPCB") == 0) {
                show_pcb(name);
            }
            else if (strcmp(input, "showready") == 0) {
                show_ready();
            }
            else if (strcmp(input, "shutdown") == 0) {
                shutdown();
            }
            else if (strcmp(input, "suspendPCB") == 0) {
                suspend_pcb(name);
            }
            else if (strcmp(input, "version") == 0) {
                version();
            }
            else
            {
                char *inv_msg = "\nInvalid input, please try again\n";
                sys_req(WRITE, COM1, inv_msg, strlen(inv_msg));
            }
        }
    }
}
