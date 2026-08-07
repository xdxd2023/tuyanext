#pragma once

#include "../include/linux_serial.h"

#define MCU_PORT "/dev/ttyS3"

extern Serial serial;

extern int ArmUartInit(void);
extern void *ArmTask(void *argv);