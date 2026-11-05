/*
 * Módulo 18 - CAN (bxCAN)
 *
 * DEMO 1 (padrão): modo LOOPBACK + SILENT. O controlador recebe o que ele
 *     mesmo transmite, sem tocar no barramento -> não precisa de transceiver.
 * DEMO 2: modo NORMAL a 500 kbit/s. Precisa de um transceiver (SN65HVD230,
 *     TJA1050/MCP2551 com cuidado com 5 V) e de outro nó no barramento,
 *     com terminação de 120 Ω nas duas pontas.
 *
 * Pinos: PB8 = CAN_RX (D15), PB9 = CAN_TX (D14)
 *        F072: AF4    |   F446: CAN1, AF9
 *
 *   make clean && make MCU=f4 EXTRA=-DDEMO=2
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#if defined(STM32F0)
#  define CANx       CAN
#  define CAN_AF     4U
#else
#  define CANx       CAN1
#  define CAN_AF     9U
#endif

typedef struct {
    uint32_t id;        /* 11 bits (std) ou 29 bits (ext) */
    uint8_t  ext;       /* 1 = identificador estendido     */
    uint8_t  rtr;       /* 1 = remote frame                */
    uint8_t  dlc;       /* 0..8                            */
    uint8_t  data[8];
} can_msg_t;

static int can_init(int loopback)
{
#if defined(STM32F0)
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_CANEN;
#else
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
#endif
    gpio_af(GPIOB, 8, CAN_AF);
    gpio_af(GPIOB, 9, CAN_AF);
    gpio_pull(GPIOB, 8, GPIO_PULLUP);           /* RX recessivo se o fio soltar */

    /* 1) Sai do Sleep e entra em Initialization mode */
    CANx->MCR &= ~(1U << 1);                    /* SLEEP = 0 */
    CANx->MCR |=  (1U << 0);                    /* INRQ  = 1 */
    uint32_t t0 = millis();
    while (!(CANx->MSR & (1U << 0))) {          /* espera INAK */
        if (millis() - t0 > 100) return -1;
    }

    /* 2) Opções: ABOM (sai sozinho do bus-off), TXFP (FIFO de TX por ordem) */
    CANx->MCR |= (1U << 6) | (1U << 2);

    /* 3) Bit timing: 500 kbit/s, ponto de amostragem ~87 %
     *    bit = (1 + TS1 + TS2) tq,  tq = BRP / PCLK
     *    F0: 48 MHz / 6  = 8 MHz  -> 16 tq: 1 + 13 + 2   (87,5 %)
     *    F4: 45 MHz / 5  = 9 MHz  -> 18 tq: 1 + 15 + 2   (88,9 %) */
#if defined(STM32F0)
    const uint32_t brp = 6, ts1 = 13, ts2 = 2;
#else
    const uint32_t brp = 5, ts1 = 15, ts2 = 2;
#endif
    const uint32_t sjw = 1;
    CANx->BTR = ((sjw - 1U) << 24) | ((ts2 - 1U) << 20) | ((ts1 - 1U) << 16) | (brp - 1U)
              | (loopback ? ((1U << 30) | (1U << 31)) : 0U);   /* LBKM | SILM */

    /* 4) Filtro 0: máscara 32 bits com máscara 0 -> aceita TUDO, vai p/ FIFO0 */
    CANx->FMR  |=  1U;                          /* FINIT: filtros em modo init */
    CANx->FA1R &= ~1U;                          /* desativa o filtro 0         */
    CANx->FS1R |=  1U;                          /* escala 32 bits              */
    CANx->FM1R &= ~1U;                          /* modo máscara (não lista)    */
    CANx->sFilterRegister[0].FR1 = 0;           /* ID                          */
    CANx->sFilterRegister[0].FR2 = 0;           /* máscara: 0 = "não importa"  */
    CANx->FFA1R &= ~1U;                         /* -> FIFO 0                   */
    CANx->FA1R |=  1U;                          /* ativa                       */
    CANx->FMR  &= ~1U;                          /* sai do modo init de filtros */

    /* 5) Normal mode: o controlador espera 11 bits recessivos no barramento */
    CANx->MCR &= ~(1U << 0);
    t0 = millis();
    while (CANx->MSR & (1U << 0)) {
        if (millis() - t0 > 100) return -2;     /* barramento preso em dominante? */
    }
    return 0;
}

static int can_send(const can_msg_t *m)
{
    uint32_t tsr = CANx->TSR;
    int mb;
    if      (tsr & (1U << 26)) mb = 0;          /* TME0 */
    else if (tsr & (1U << 27)) mb = 1;          /* TME1 */
    else if (tsr & (1U << 28)) mb = 2;          /* TME2 */
    else return -1;                             /* 3 mailboxes ocupadas */

    CAN_TxMailBox_TypeDef *tx = &CANx->sTxMailBox[mb];
    tx->TIR  = m->ext ? ((m->id << 3) | (1U << 2))   /* EXID[28:0] em [31:3], IDE */
                      : (m->id << 21);               /* STID[10:0] em [31:21]     */
    if (m->rtr) tx->TIR |= (1U << 1);
    tx->TDTR = m->dlc & 0xFU;
    tx->TDLR = (uint32_t)m->data[0] | ((uint32_t)m->data[1] << 8) |
               ((uint32_t)m->data[2] << 16) | ((uint32_t)m->data[3] << 24);
    tx->TDHR = (uint32_t)m->data[4] | ((uint32_t)m->data[5] << 8) |
               ((uint32_t)m->data[6] << 16) | ((uint32_t)m->data[7] << 24);
    tx->TIR |= 1U;                              /* TXRQ: pede a transmissão */
    return mb;
}

static int can_recv(can_msg_t *m)
{
    if ((CANx->RF0R & 3U) == 0) return 0;       /* FMP0: mensagens pendentes no FIFO0 */
    CAN_FIFOMailBox_TypeDef *rx = &CANx->sFIFOMailBox[0];
    uint32_t rir = rx->RIR;
    m->ext = (rir >> 2) & 1U;
    m->rtr = (rir >> 1) & 1U;
    m->id  = m->ext ? (rir >> 3) : (rir >> 21);
    m->dlc = rx->RDTR & 0xFU;
    uint32_t lo = rx->RDLR, hi = rx->RDHR;
    for (int i = 0; i < 4; i++) { m->data[i] = (uint8_t)(lo >> (8 * i)); m->data[4 + i] = (uint8_t)(hi >> (8 * i)); }
    CANx->RF0R |= (1U << 5);                    /* RFOM0: libera a posição do FIFO */
    return 1;
}

static void print_msg(const char *tag, const can_msg_t *m)
{
    kprintf(m->ext ? "%s id=0x%08X(ext) dlc=%u :" : "%s id=0x%03X dlc=%u :", tag, m->id, m->dlc);
    for (int i = 0; i < m->dlc && i < 8; i++) kprintf(" %02X", m->data[i]);
    kprintf("\n");
}

int main(void)
{
    system_init();
    uart_init(115200);
    kprintf("\n=== Modulo 18: CAN (%s) ===\n", DEMO == 1 ? "loopback+silent" : "normal 500k");

    int r = can_init(DEMO == 1);
    if (r) {
        kprintf("can_init falhou (%d). MSR=0x%08X\n", r, CANx->MSR);
        for (;;) { }
    }

    uint32_t cont = 0, t_tx = 0;
    can_msg_t m;
    for (;;) {
        if (millis() - t_tx >= 1000) {
            t_tx = millis();
            can_msg_t tx = { .id = 0x123, .dlc = 8 };
            for (int i = 0; i < 4; i++) tx.data[i] = (uint8_t)(cont >> (8 * i));
            tx.data[4] = 0xCA; tx.data[5] = 0xFE; tx.data[6] = 0xBA; tx.data[7] = 0xBE;
            if ((cont & 1U) != 0) { tx.ext = 1; tx.id = 0x18DAF110; }   /* alterna std/ext */
            int mb = can_send(&tx);
            print_msg(mb >= 0 ? "TX" : "TX FALHOU", &tx);
            cont++;

            uint32_t esr = CANx->ESR;
            if (esr & 7U)
                kprintf("   ESR: TEC=%u REC=%u LEC=%u %s%s%s\n", (esr >> 16) & 0xFFU, esr >> 24,
                        (esr >> 4) & 7U, (esr & 4U) ? "BUS-OFF " : "", (esr & 2U) ? "PASSIVE " : "",
                        (esr & 1U) ? "WARNING" : "");
        }
        while (can_recv(&m)) print_msg("RX", &m);
    }
}
