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

extern void ModBus_TimerValues(void);

#define RS485_DE_GPIO_PORT (GPIOC)
#define RS485_DE_GPIO_PIN (GPIO_Pin_3)

void set_rs485_de_enable(void);
void set_rs485_de_disable(void);

#endif
