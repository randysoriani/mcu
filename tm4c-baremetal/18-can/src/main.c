/*
 * Módulo 18 - CAN (controlador Bosch C_CAN)
 *
 *   DEMO 1 (padrão): LOOPBACK + SILENT interno -> sem transceiver
 *   DEMO 2: modo normal a 500 kbit/s (transceiver SN65HVD230 + outro nó)
 *
 *   123: CAN0, PE4 = CAN0RX, PE5 = CAN0TX (PCTL 8)
 *   129: CAN1, PB0 = CAN1RX, PB1 = CAN1TX (PCTL 7)  (CAN0 do 129 ficaria em
 *        PA0/PA1, que são da UART0 do console)
 *
 * O C_CAN não tem "mailboxes" fixas: tem 32 MESSAGE OBJECTS na RAM interna.
 * A CPU nunca os acessa direto: preenche os registradores de interface
 * (IF1/IF2) e manda o hardware copiar de/para o objeto n (IFnCRQ).
 *   objeto 1 = transmissão, objeto 2 = recepção (aceita qualquer ID)
 */
#include "system.h"
#include "gpio.h"

#ifndef DEMO
#define DEMO 1
#endif

#if defined(TM4C123)
#  define CANx CAN0
#else
#  define CANx CAN1
#endif
#define OBJ_TX 1U
#define OBJ_RX 2U

typedef struct { uint32_t id; uint8_t ext, dlc, data[8]; } can_msg_t;

static void if_wait(volatile CAN_IF_TypeDef *ifr)
{
    while (ifr->CRQ & CAN_IFCRQ_BUSY) { }
}

static void can_init(int loopback)
{
#if defined(TM4C123)
    periph_enable(&SYSCTL_RCGCCAN, &SYSCTL_PRCAN, 0);
    gpio_port_enable(PORTE);
    gpio_af(GPIOE, 4, 8);
    gpio_af(GPIOE, 5, 8);
#else
    periph_enable(&SYSCTL_RCGCCAN, &SYSCTL_PRCAN, 1);
    gpio_port_enable(PORTB);
    gpio_af(GPIOB, 0, 7);
    gpio_af(GPIOB, 1, 7);
#endif

    CANx->CTL = CAN_CTL_INIT | CAN_CTL_CCE;     /* init + libera escrita no BIT */

    /* 500 kbit/s: bit = (1 + TSEG1 + TSEG2) tq = 16 tq, sample point 87,5%
     *   123: 80 MHz / 10 = 8 MHz (tq 125 ns)   129: 120 MHz / 15 = 8 MHz */
    uint32_t brp = SystemCoreClock / 8000000U, tseg1 = 13, tseg2 = 2, sjw = 1;
    CANx->BIT  = ((tseg2 - 1U) << 12) | ((tseg1 - 1U) << 8) | ((sjw - 1U) << 6) | (brp - 1U);
    CANx->BRPE = 0;

    /* Objeto de recepção: máscara 0 = aceita qualquer ID (std ou ext) */
    if_wait(&CANx->IF2);
    CANx->IF2.CMSK = CAN_IFCMSK_WRNRD | CAN_IFCMSK_MASK | CAN_IFCMSK_ARB | CAN_IFCMSK_CONTROL;
    CANx->IF2.MSK1 = 0;
    CANx->IF2.MSK2 = 0;                          /* MXTD = 0: não filtra pelo tipo */
    CANx->IF2.ARB1 = 0;
    CANx->IF2.ARB2 = CAN_IFARB2_MSGVAL;          /* DIR = 0: recepção */
    CANx->IF2.MCTL = CAN_IFMCTL_UMASK | CAN_IFMCTL_EOB | 8U;
    CANx->IF2.CRQ  = OBJ_RX;
    if_wait(&CANx->IF2);

    if (loopback) {
        CANx->CTL |= CAN_CTL_TEST;
        CANx->TST  = CAN_TST_LBACK | CAN_TST_SILENT;   /* "hot selftest" */
    }
    CANx->CTL &= ~(CAN_CTL_INIT | CAN_CTL_CCE);      /* sai do init (mantém TEST) */
}

static int can_send(const can_msg_t *m)
{
    if (CANx->TXRQ1 & (1U << (OBJ_TX - 1U))) return -1;   /* anterior ainda pendente */
    if_wait(&CANx->IF1);
    CANx->IF1.CMSK = CAN_IFCMSK_WRNRD | CAN_IFCMSK_ARB | CAN_IFCMSK_CONTROL
                   | CAN_IFCMSK_DATAA | CAN_IFCMSK_DATAB;
    if (m->ext) {
        CANx->IF1.ARB1 = m->id & 0xFFFFU;
        CANx->IF1.ARB2 = CAN_IFARB2_MSGVAL | CAN_IFARB2_XTD | CAN_IFARB2_DIR | ((m->id >> 16) & 0x1FFFU);
    } else {
        CANx->IF1.ARB1 = 0;
        CANx->IF1.ARB2 = CAN_IFARB2_MSGVAL | CAN_IFARB2_DIR | ((m->id & 0x7FFU) << 2);  /* ID[28:18] */
    }
    CANx->IF1.MCTL = CAN_IFMCTL_TXRQST | CAN_IFMCTL_EOB | (m->dlc & 0xFU);
    CANx->IF1.DA1  = m->data[0] | ((uint32_t)m->data[1] << 8);
    CANx->IF1.DA2  = m->data[2] | ((uint32_t)m->data[3] << 8);
    CANx->IF1.DB1  = m->data[4] | ((uint32_t)m->data[5] << 8);
    CANx->IF1.DB2  = m->data[6] | ((uint32_t)m->data[7] << 8);
    CANx->IF1.CRQ  = OBJ_TX;                         /* copia para o objeto e transmite */
    return 0;
}

static int can_recv(can_msg_t *m)
{
    if (!(CANx->NWDA1 & (1U << (OBJ_RX - 1U)))) return 0;   /* sem dado novo */
    if_wait(&CANx->IF2);
    CANx->IF2.CMSK = CAN_IFCMSK_ARB | CAN_IFCMSK_CONTROL | CAN_IFCMSK_DATAA | CAN_IFCMSK_DATAB
                   | CAN_IFCMSK_CLRINTPND | CAN_IFCMSK_TXRQST;  /* TXRQST aqui = limpa NEWDAT */
    CANx->IF2.CRQ  = OBJ_RX;
    if_wait(&CANx->IF2);
    uint32_t a2 = CANx->IF2.ARB2;
    m->ext = (a2 & CAN_IFARB2_XTD) ? 1U : 0U;
    m->id  = m->ext ? (((a2 & 0x1FFFU) << 16) | CANx->IF2.ARB1) : ((a2 >> 2) & 0x7FFU);
    m->dlc = CANx->IF2.MCTL & 0xFU;
    uint32_t d[4] = { CANx->IF2.DA1, CANx->IF2.DA2, CANx->IF2.DB1, CANx->IF2.DB2 };
    for (int i = 0; i < 4; i++) { m->data[2 * i] = (uint8_t)d[i]; m->data[2 * i + 1] = (uint8_t)(d[i] >> 8); }
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
    can_init(DEMO == 1);

    uint32_t cont = 0, t_tx = 0;
    can_msg_t m;
    for (;;) {
        if (millis() - t_tx >= 1000) {
            t_tx = millis();
            can_msg_t tx = { .id = 0x123, .dlc = 8 };
            for (int i = 0; i < 4; i++) tx.data[i] = (uint8_t)(cont >> (8 * i));
            tx.data[4] = 0xCA; tx.data[5] = 0xFE; tx.data[6] = 0xBA; tx.data[7] = 0xBE;
            if (cont & 1U) { tx.ext = 1; tx.id = 0x18DAF110; }
            print_msg(can_send(&tx) == 0 ? "TX" : "TX OCUPADO", &tx);
            cont++;
            uint32_t sts = CANx->STS, err = CANx->ERR;
            if (sts & (CAN_STS_BOFF | CAN_STS_EPASS | CAN_STS_EWARN) || (sts & 7U))
                kprintf("   STS=0x%02X (LEC=%u) TEC=%u REC=%u\n", sts, sts & 7U, err & 0xFFU, (err >> 8) & 0x7FU);
        }
        while (can_recv(&m)) print_msg("RX", &m);
    }
}
