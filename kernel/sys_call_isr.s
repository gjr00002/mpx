bits 32
global sys_call_isr

;;; System call interrupt handler. To be implemented in Module R3.
extern sys_call			; The C function that sys_call_isr will call
sys_call_isr:
    ; Save registers
    

    push edi;
    push esi;
    push ebp;
    push esp;
    push edx;
    push ecx;
    push ebx;
    push eax;

    push ss;
    push gs;
    push fs;
    push es;
    push ds;
    
    ;PUSH ESP FOR sys_call argmuent
    push esp;

    ; Call C function
    call sys_call

    mov esp, eax

    ; Restore registers
    pop ds;
    pop es;
    pop fs;
    pop gs;
    pop ss;
 
    pop eax;
    pop ebx;
    pop ecx;
    pop edx;
    add esp, 4;
    pop ebp;
    pop esi;
    pop edi;


    ; Return from ISR
    iret
