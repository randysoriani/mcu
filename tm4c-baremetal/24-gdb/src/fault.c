/*
 * fault.c - HardFault_Handler que GUARDA o contexto da falha para o GDB.
 *
 * Quando ocorre uma exceção, o hardware empilha 8 words na pilha que estava
 * em uso (MSP ou PSP):  r0 r1 r2 r3 r12 lr pc xpsr
 * O bit 2 do LR (EXC_RETURN) diz qual pilha: 0 = MSP, 1 = PSP.
 *
 * Um handler "naked" (sem prólogo gerado pelo compilador) descobre a pilha,
 * passa o ponteiro para uma função C, que copia tudo para fault_info.
 * No GDB:   p/x fault_info      ou   fault   (comando de gdb/curso.gdb)
 */
#include <stdint.h>
#include "system.h"
#include "gpio.h"

typedef struct {
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;   /* empilhados pelo hardware */
    uint32_t cfsr, hfsr, mmfar, bfar;             /* registradores de falha do M4 */
    uint32_t exc_return;
} fault_info_t;

volatile fault_info_t fault_info;

void hardfault_c(uint32_t *frame, uint32_t exc_return) __attribute__((used, noreturn));

void hardfault_c(uint32_t *frame, uint32_t exc_return)
{
    fault_info.r0   = frame[0];
    fault_info.r1   = frame[1];
    fault_info.r2   = frame[2];
    fault_info.r3   = frame[3];
    fault_info.r12  = frame[4];
    fault_info.lr   = frame[5];
    fault_info.pc   = frame[6];        /* instrução que falhou (ou próxima) */
    fault_info.xpsr = frame[7];
    fault_info.exc_return = exc_return;
    fault_info.cfsr  = SCB->CFSR;
    fault_info.hfsr  = SCB->HFSR;
    fault_info.mmfar = SCB->MMFAR;
    fault_info.bfar  = SCB->BFAR;
    kprintf("\n*** HardFault! pc=0x%08X lr=0x%08X cfsr=0x%08X hfsr=0x%08X bfar=0x%08X\n",
            fault_info.pc, fault_info.lr, fault_info.cfsr, fault_info.hfsr, fault_info.bfar);
    kprintf("*** no GDB: Ctrl-C e depois 'fault'\n");

    for (;;) {                          /* pisca rápido = estado de falha */
        gpio_toggle(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++) { }
    }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "movs r0, #4          \n"   /* testa o bit 2 do EXC_RETURN (em LR)   */
        "mov  r1, lr          \n"
        "tst  r0, r1          \n"
        "beq  1f              \n"
        "mrs  r0, psp         \n"   /* bit 2 = 1: a pilha era a PSP           */
        "b    2f              \n"
        "1:                   \n"
        "mrs  r0, msp         \n"   /* bit 2 = 0: MSP (o caso deste curso)    */
        "2:                   \n"
        "mov  r1, lr          \n"   /* 2º argumento: EXC_RETURN               */
        "ldr  r2, =hardfault_c\n"
        "bx   r2              \n"   /* ldr+bx: funciona no M0 (sem b.w longo) */
        ".ltorg               \n"
    );
}
