#ifndef _PIT_H
#define _PIT_H

#include "lib.h"
#include "i8259.h"
#include "int_handlers.h"

#ifndef ASM

void pit_init();

#endif
#endif
