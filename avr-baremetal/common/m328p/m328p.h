/*
 * m328p.h - Registradores do ATmega328P (Arduino Uno / Nano)
 *
 * Escrito à mão para o curso a partir do datasheet "ATmega48A/PA/88A/PA/
 * 168A/PA/328/P" (Microchip DS40002061). Não usa <avr/io.h> da avr-libc.
 *
 * Todos os endereços abaixo são do ESPAÇO DE DADOS (o que o C enxerga).
 * Os 64 primeiros registradores de E/S também têm um endereço no "espaço de
 * E/S" (endereço de dados - 0x20), usado pelas instruções IN/OUT/SBI/CBI;
 * o compilador escolhe essas instruções sozinho quando o endereço é constante.
 */
#ifndef M328P_H
#define M328P_H

#include <stdint.h>

#define REG8(addr)  (*(volatile uint8_t  *)(addr))
#define REG16(addr) (*(volatile uint16_t *)(addr))   /* acesso de 16 bits (byte baixo primeiro) */

/* ---------------- Memórias ---------------- */
#define FLASH_SIZE   32768U      /* 32 KB, endereços 0x0000-0x7FFF (em BYTES) */
#define SRAM_START   0x0100U     /* RAM: 0x0100-0x08FF (2 KB) */
#define RAMEND       0x08FFU
#define EEPROM_SIZE  1024U

/* ---------------- GPIO ---------------- */
#define PINB    REG8(0x23)   /* leitura dos pinos (escrever 1 INVERTE o pino) */
#define DDRB    REG8(0x24)   /* direção: 1 = saída */
#define PORTB   REG8(0x25)   /* saída / pull-up na entrada */
#define PINC    REG8(0x26)
#define DDRC    REG8(0x27)
#define PORTC   REG8(0x28)
#define PIND    REG8(0x29)
#define DDRD    REG8(0x2A)
#define PORTD   REG8(0x2B)

/* ---------------- Flags de interrupção / registradores de uso geral ---------------- */
#define TIFR0   REG8(0x35)
#define TIFR1   REG8(0x36)
#define TIFR2   REG8(0x37)
#define PCIFR   REG8(0x3B)
#define EIFR    REG8(0x3C)
#define EIMSK   REG8(0x3D)
#define GPIOR0  REG8(0x3E)

/* ---------------- EEPROM ---------------- */
#define EECR    REG8(0x3F)
#define EEDR    REG8(0x40)
#define EEAR    REG16(0x41)
#define   EERE   0
#define   EEPE   1
#define   EEMPE  2
#define   EERIE  3

/* ---------------- Timer0 (8 bits) ---------------- */
#define GTCCR   REG8(0x43)
#define TCCR0A  REG8(0x44)
#define TCCR0B  REG8(0x45)
#define TCNT0   REG8(0x46)
#define OCR0A   REG8(0x47)
#define OCR0B   REG8(0x48)
#define   WGM00  0
#define   WGM01  1
#define   COM0B0 4
#define   COM0B1 5
#define   COM0A0 6
#define   COM0A1 7
#define   CS00   0
#define   CS01   1
#define   CS02   2
#define   WGM02  3

/* ---------------- SPI ---------------- */
#define SPCR    REG8(0x4C)
#define SPSR    REG8(0x4D)
#define SPDR    REG8(0x4E)
#define   SPR0   0
#define   SPR1   1
#define   CPHA   2
#define   CPOL   3
#define   MSTR   4
#define   DORD   5
#define   SPE    6
#define   SPIE   7
#define   SPI2X  0
#define   WCOL   6
#define   SPIF   7

/* ---------------- Comparador analógico ---------------- */
#define ACSR    REG8(0x50)
#define   ACIS0  0
#define   ACIS1  1
#define   ACIC   2
#define   ACIE   3
#define   ACI    4
#define   ACO    5
#define   ACBG   6
#define   ACD    7

/* ---------------- Controle do MCU ---------------- */
#define SMCR    REG8(0x53)   /* sleep */
#define   SE     0
#define   SM0    1
#define   SM1    2
#define   SM2    3
#define MCUSR   REG8(0x54)   /* causa do reset */
#define   PORF   0
#define   EXTRF  1
#define   BORF   2
#define   WDRF   3
#define MCUCR   REG8(0x55)
#define   IVCE   0
#define   IVSEL  1
#define   PUD    4
#define   BODSE  5
#define   BODS   6
#define SPMCSR  REG8(0x57)   /* autoprogramação da Flash */
#define   SPMEN  0
#define   PGERS  1
#define   PGWRT  2
#define   BLBSET 3
#define   RWWSRE 4
#define   SIGRD  5
#define   RWWSB  6
#define SPL     REG8(0x5D)   /* stack pointer */
#define SPH     REG8(0x5E)
#define SREG    REG8(0x5F)   /* status: bit 7 = I (interrupções globais) */

/* ---------------- Memória estendida de E/S (só LDS/STS) ---------------- */
#define WDTCSR  REG8(0x60)
#define   WDP0   0
#define   WDP1   1
#define   WDP2   2
#define   WDE    3
#define   WDCE   4
#define   WDP3   5
#define   WDIE   6
#define   WDIF   7
#define CLKPR   REG8(0x61)
#define   CLKPCE 7
#define PRR     REG8(0x64)   /* power reduction: 1 = DESLIGA o periférico */
#define   PRADC     0
#define   PRUSART0  1
#define   PRSPI     2
#define   PRTIM1    3
#define   PRTIM0    5
#define   PRTIM2    6
#define   PRTWI     7
#define OSCCAL  REG8(0x66)
#define PCICR   REG8(0x68)
#define EICRA   REG8(0x69)
#define PCMSK0  REG8(0x6B)
#define PCMSK1  REG8(0x6C)
#define PCMSK2  REG8(0x6D)
#define TIMSK0  REG8(0x6E)
#define TIMSK1  REG8(0x6F)
#define TIMSK2  REG8(0x70)
#define   TOIE   0           /* mesmos bits nos três TIMSKn */
#define   OCIEA  1
#define   OCIEB  2
#define   ICIE1  5
#define   TOV    0           /* mesmos bits nos três TIFRn */
#define   OCFA   1
#define   OCFB   2
#define   ICF1   5

/* ---------------- ADC ---------------- */
#define ADC     REG16(0x78)  /* ADCL + ADCH (ler ADCL primeiro: o acesso de 16 bits já faz isso) */
#define ADCL    REG8(0x78)
#define ADCH    REG8(0x79)
#define ADCSRA  REG8(0x7A)
#define ADCSRB  REG8(0x7B)
#define ADMUX   REG8(0x7C)
#define DIDR0   REG8(0x7E)
#define DIDR1   REG8(0x7F)
#define   ADPS0  0
#define   ADIE   3
#define   ADIF   4
#define   ADATE  5
#define   ADSC   6
#define   ADEN   7
#define   ADLAR  5
#define   REFS0  6
#define   REFS1  7
#define   ACME   6

/* ---------------- Timer1 (16 bits) ---------------- */
#define TCCR1A  REG8(0x80)
#define TCCR1B  REG8(0x81)
#define TCCR1C  REG8(0x82)
#define TCNT1   REG16(0x84)
#define ICR1    REG16(0x86)
#define OCR1A   REG16(0x88)
#define OCR1B   REG16(0x8A)
#define   WGM10  0
#define   WGM11  1
#define   COM1B0 4
#define   COM1B1 5
#define   COM1A0 6
#define   COM1A1 7
#define   CS10   0
#define   CS11   1
#define   CS12   2
#define   WGM12  3
#define   WGM13  4
#define   ICES1  6
#define   ICNC1  7

/* ---------------- Timer2 (8 bits, assíncrono) ---------------- */
#define TCCR2A  REG8(0xB0)
#define TCCR2B  REG8(0xB1)
#define TCNT2   REG8(0xB2)
#define OCR2A   REG8(0xB3)
#define OCR2B   REG8(0xB4)
#define ASSR    REG8(0xB6)
#define   WGM20  0
#define   WGM21  1
#define   COM2B0 4
#define   COM2B1 5
#define   COM2A0 6
#define   COM2A1 7
#define   CS20   0
#define   CS21   1
#define   CS22   2
#define   WGM22  3

/* ---------------- TWI (I2C) ---------------- */
#define TWBR    REG8(0xB8)
#define TWSR    REG8(0xB9)
#define TWAR    REG8(0xBA)
#define TWDR    REG8(0xBB)
#define TWCR    REG8(0xBC)
#define   TWIE   0
#define   TWEN   2
#define   TWWC   3
#define   TWSTO  4
#define   TWSTA  5
#define   TWEA   6
#define   TWINT  7

/* ---------------- USART0 ---------------- */
#define UCSR0A  REG8(0xC0)
#define UCSR0B  REG8(0xC1)
#define UCSR0C  REG8(0xC2)
#define UBRR0   REG16(0xC4)
#define UDR0    REG8(0xC6)
#define   MPCM0  0
#define   U2X0   1
#define   UPE0   2
#define   DOR0   3
#define   FE0    4
#define   UDRE0  5
#define   TXC0   6
#define   RXC0   7
#define   TXB80  0
#define   RXB80  1
#define   UCSZ02 2
#define   TXEN0  3
#define   RXEN0  4
#define   UDRIE0 5
#define   TXCIE0 6
#define   RXCIE0 7
#define   UCSZ00 1
#define   UCSZ01 2
#define   USBS0  3
#define   UPM00  4
#define   UPM01  5

/* ---------------- Vetores de interrupção (datasheet, tabela 12-6) ---------------- */
#define INT0_vect          1
#define INT1_vect          2
#define PCINT0_vect        3
#define PCINT1_vect        4
#define PCINT2_vect        5
#define WDT_vect           6
#define TIMER2_COMPA_vect  7
#define TIMER2_COMPB_vect  8
#define TIMER2_OVF_vect    9
#define TIMER1_CAPT_vect   10
#define TIMER1_COMPA_vect  11
#define TIMER1_COMPB_vect  12
#define TIMER1_OVF_vect    13
#define TIMER0_COMPA_vect  14
#define TIMER0_COMPB_vect  15
#define TIMER0_OVF_vect    16
#define SPI_STC_vect       17
#define USART_RX_vect      18
#define USART_UDRE_vect    19
#define USART_TX_vect      20
#define ADC_vect           21
#define EE_READY_vect      22
#define ANALOG_COMP_vect   23
#define TWI_vect           24
#define SPM_READY_vect     25

/*
 * ISR(vetor): define a rotina de interrupção. O startup.S declara a tabela
 * com "jmp __vector_N"; uma função com esse nome substitui a padrão.
 * O atributo "signal" faz o GCC salvar os registradores e terminar com RETI.
 */
#define ISR(v)          ISR_IMPL(v)
#define ISR_IMPL(n) \
    void __vector_##n(void) __attribute__((signal, used, externally_visible)); \
    void __vector_##n(void)

static inline void sei(void) { __asm__ __volatile__ ("sei" ::: "memory"); }
static inline void cli(void) { __asm__ __volatile__ ("cli" ::: "memory"); }
static inline void nop(void) { __asm__ __volatile__ ("nop"); }
static inline void sleep_cpu(void) { __asm__ __volatile__ ("sleep"); }
static inline void wdt_reset(void) { __asm__ __volatile__ ("wdr"); }

#endif
