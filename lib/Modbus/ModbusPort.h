#ifndef __MODBUSPORT__H
#define __MODBUSPORT__H

#include "Modbus.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

extern volatile unsigned char ReceiveBuffer[MODBUS_RECEIVE_BUFFER_SIZE];
extern volatile unsigned char ReceiveCounter;

extern void ModBus_UART_Initialise(void);
extern void Timer_Initialise(void);
extern void ModBus_UART_Put(unsigned char c);
extern unsigned char ModBus_UART_String(unsigned char *s, unsigned int Length);

extern void ModBus_TimerValues(void);

// Serial port transceiver enable
#define RS485_DE_GPIO_PORT (GPIOD)
#define RS485_DE_GPIO_PIN (GPIO_Pin_4)

void set_rs485_de_enable(void);
void set_rs485_de_disable(void);

#endif
