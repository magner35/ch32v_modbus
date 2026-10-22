#ifndef _MODBUS_CONTROL_H_
#define _MODBUS_CONTROL_H_

/* Диспетчер управляющих команд Modbus (HREG_CONTROL + пароль).
 * Вызывать из главного цикла — при пустой команде сразу возвращается. */
void modbusControl(void);

#endif // !_MODBUS_CONTROL_H_
