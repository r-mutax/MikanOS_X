; asmfunc.asm
; 
; System V AMD64 Calling Convention
; Registers: RDI, RSI, RDX, RCX, R8, R9

bits 64
section .text

global IoOut32 ; void InOut32(int16_t addr, uint32_t data);
IoOut32:
    mov dx, di    ; dx = addr
    mov eax, esi  ; eax = data
    out dx, eax
    ret

global IoIn32 ; uint32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di    ; dx = addr
    in eax, dx
    ret

global GetCS    ; uint16_t GetCS(void);
GetCS:
    xor eax, eax  ; also clears upper 32 bits of rax
    mov ax, cs
    ret

global LoadIDT  ;void LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di   ; limit
    mov [rsp + 2], rsi  ; offset
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global LoadGDT  ; void LoadGDT(uint16_t limit, uint64_t offset);
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di  ; limit
    mov [rsp + 2], rsi  ; offset
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global SetCSSS  ; void SetCSSS(uint16_t cs, uint16_t ss);
SetCSSS:
    push rbp
    mov rbp, rsp
    mov ss, si
    mov rax, .next
    push rdi    ; CS
    push rax    ; RIP
    o64 retf
.next:
    mov rsp, rbp
    pop rbp
    ret

global SetDSAll  ; void SetDSAll(uint16_t value);
SetDSAll:
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global GetCR0   ; uint64_t GetCR0();
GetCR0:
    mov rax, cr0
    ret

global SetCR0   ; void SetCR0(uint64_t value);
SetCR0:
    mov cr0, rdi
    ret

global GetCR2   ; uint64_t GetCR2();
GetCR2:
    mov rax, cr2
    ret

global SetCR3   ; void SetCR3(uint64_t value);
SetCR3:
    mov cr3, rdi
    ret

global GetCR3   ; uint64_t GetCR3();
GetCR3:
    mov rax, cr3
    ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
    mov rsp, kernel_main_stack + 1024 * 1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin
    
global SwitchContext
SwitchContext:    ; void SwitchContext(void* next_ctx, void* current_ctx);
    mov [rsi + 0x40], rax
    mov [rsi + 0x48], rbx
    mov [rsi + 0x50], rcx
    mov [rsi + 0x58], rdx
    mov [rsi + 0x60], rdi
    mov [rsi + 0x68], rsi

    lea rax, [rsp + 8]
    mov [rsi + 0x70], rax   ; RSP
    mov [rsi + 0x78], rbp

    mov [rsi + 0x80], r8
    mov [rsi + 0x88], r9
    mov [rsi + 0x90], r10
    mov [rsi + 0x98], r11
    mov [rsi + 0xa0], r12
    mov [rsi + 0xa8], r13
    mov [rsi + 0xb0], r14
    mov [rsi + 0xb8], r15

    mov rax, cr3
    mov [rsi + 0x00], rax   ; CR3
    mov rax, [rsp]
    mov [rsi + 0x08], rax   ; RIP
    pushfq
    pop qword [rsi + 0x10]  ; RFLAGS

    mov ax, cs
    mov [rsi + 0x20], rax
    mov bx, ss
    mov [rsi + 0x28], rbx
    mov cx, fs
    mov [rsi + 0x30], rcx
    mov dx, gs
    mov [rsi + 0x38], rdx

    fxsave [rsi + 0xc0]
	; fall through to RestoreContext

global RestoreContext
RestoreContext:  ; void RestoreContext(void* task_context);
    ; iret用のスタックフレーム
    push qword [rdi + 0x28] ; SS
    push qword [rdi + 0x70] ; RSP
    push qword [rdi + 0x10] ; RFLAGS
    push qword [rdi + 0x20] ; CS
    push qword [rdi + 0x08] ; RIP

    ; コンテキストの復帰
    fxrstor [rdi + 0xc0]

    mov rax, [rdi + 0x00]
    mov cr3, rax
    mov rax, [rdi + 0x30]
    mov fs, ax
    mov rax, [rdi + 0x38]
    mov gs, ax

    mov rax, [rdi + 0x40]
    mov rbx, [rdi + 0x48]
    mov rcx, [rdi + 0x50]
    mov rdx, [rdi + 0x58]
    mov rsi, [rdi + 0x68]
    mov rbp, [rdi + 0x78]
    mov r8, [rdi + 0x80]
    mov r9, [rdi + 0x88]
    mov r10, [rdi + 0x90]
    mov r11, [rdi + 0x98]
    mov r12, [rdi + 0xa0]
    mov r13, [rdi + 0xa8]
    mov r14, [rdi + 0xb0]
    mov r15, [rdi + 0xb8]

    mov rdi, [rdi + 0x60]

    o64 iret

global CallApp
CallApp:    ;int CallApp(int argc, char** argv, uint16_t ss, uint64_t rip, uint64_t rsp, uint64_t* os_stack_ptr);
    
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [r9], rsp
    
    push rdx
    push r8
    add rdx, 8
    push rdx
    push rcx
    o64 retf


extern LAPICTimerOnInterrupt
; void LAPICTimerOnInterrupt(const TaskContext& ctx_stack);

global IntHandlerLAPICTimer
IntHandlerLAPICTimer:   ;void IntHandlerLAPICTimer();
    push rbp
    mov rbp, rsp

    sub rsp, 512
    fxsave [rsp]
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push qword [rbp]
    push qword [rbp + 0x20]
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax
    
    mov ax, fs
    mov bx, gs
    mov rcx, cr3

    push rbx
    push rax
    push qword [ rbp + 0x28 ]
    push qword [ rbp + 0x10 ]
    push rbp
    push qword [ rbp + 0x18 ]
    push qword [ rbp + 0x08 ]
    push rcx

    mov rdi, rsp
    call LAPICTimerOnInterrupt

    add rsp, 8 * 8
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    add rsp, 16
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    fxrstor [rsp]

    mov rsp, rbp
    pop rbp
    iretq

global LoadTR
LoadTR:     ;void LoadTR(uint16_t sel);
    ltr di
    ret

global WriteMSR
WriteMSR:   ; void WriteMSR(uint32_t msr, uint64_t value);
    mov rdx, rsi
    shr rdx, 32
    mov eax, esi
    mov ecx, edi
    wrmsr
    ret

extern GetCurrentTaskOSStackPointer
extern syscall_table
global SyscallEntry
SyscallEntry:   ; void SyscallEntry(void);

    ; syscall命令はRIPとRFLAGSをrcx, r11に保存してからここに飛んでくる
    ; 最後、sysret命令で帰っていくときにrcx, r11をRIP,RFLAGSに復帰させるので、
    ; ここで覚えておく
    push rbp
    push rcx    ; original RIP
    push r11    ; original RFLAGS

    push rax

    ; 前述したようにsyscall命令はrcxをRIPの保存に使うので、引数の受け渡しのためにr10にコピーしていた。
    ; のでもとに戻しておく。
    mov rcx, r10

    ; eaxがシステムコール番号。0x80000000始まりとして使おうとしているので、MASKしている。
    and eax, 0x7fffffff

    ; CALL前のRSPの調整、16byteアライメント？させる。
    mov rbp, rsp
    and rsp, 0xfffffffffffffff0
    push rax
    push rdx
    cli
    call GetCurrentTaskOSStackPointer
    sti
    mov rdx, [rsp + 0]
    mov [rax - 16], rdx
    mov rdx, [rsp + 8]
    mov [rax - 8], rdx
    
    lea rsp, [rax - 16]
    pop rdx
    pop rax
    and rsp, 0xfffffffffffffff0

    call [syscall_table + 8 * eax]

    mov rsp, rbp
    
    pop rsi
    cmp esi, 0x80000002
    je .exit

    pop r11
    pop rcx
    pop rbp
    o64 sysret
.exit:
    mov rsp, rax
    mov eax, edx

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret

global ExitApp ; void ExitApp(uint64_t rsp, int32_t ret_val);
ExitApp
    mov rsp, rdi
    mov eax, esi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret ; CallApp の次の行に飛ぶ

global InvalidateTLB    ; void InvalidateTLB(uint64_t addr);
InvalidateTLB:
    invlpg [rdi]
    ret
