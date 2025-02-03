#ifndef _COMMHAND_H
#define _COMMHAND_H

#include <mpx/pcb.h>


void commhand(void);

void getdate(void);
void setdate(void);


char* get_time(void);
void settime(void);

void help(void);
void shutdown(void);
void version(void);
void create_pcb(void);
void delete_pcb(const char* name);
void block_pcb(const char* name);
void unblock_pcb(pcb* pcb);
void suspend_pcb(const char* name);
void resume_pcb(const char* name);
void set_pcb_priority(const char *name, int priority);
void show_pcb(char *name);
void show_ready(void);
void show_blocked(void);
void alarm(void);






#endif
