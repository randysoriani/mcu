/* device.h - seleciona o header conforme o MCU (-DTM4C123 / -DTM4C129) */
#ifndef DEVICE_H
#define DEVICE_H
#if defined(TM4C123)
#  include "tm4c123.h"
#elif defined(TM4C129)
#  include "tm4c129.h"
#else
#  error "Defina TM4C123 ou TM4C129 (o Makefile faz isso com MCU=123/129)"
#endif
#endif
