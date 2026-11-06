/* device.h - seleciona o header de registradores (-DAVR8 / -DAVR32) */
#ifndef DEVICE_H
#define DEVICE_H
#if defined(ATMEGA328P)
#  include "m328p.h"
#elif defined(UC3A0512)
#  include "uc3a.h"
#else
#  error "Defina ATMEGA328P ou UC3A0512 (o Makefile faz isso com MCU=m328p/uc3a)"
#endif
#endif
