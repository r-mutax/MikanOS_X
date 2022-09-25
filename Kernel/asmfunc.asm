; asmfunc.asm
; 
; System V AMD64 Calling Convention

bits 64
section .text
global IoOut32 ; void InOut32(int16_t addr, uint32_t data);
IoOut32:
    mov dx, di
    mov eax, esi
    out dx, eax
    ret

global IoIn32 ; uint32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di
    in eax, dx
    ret
