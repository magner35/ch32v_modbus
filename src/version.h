#ifndef _VERSION_H_
#define _VERSION_H_

/* Идентификация прибора для Modbus FC 0x11 (Report Slave ID).
 * Строки лежат во flash (.rodata), длины известны на компиляции.
 * PRODUCT — заменить на реальное обозначение изделия. */
#define DEVICE_PRODUCT_CODE "SKE03"
#define DEVICE_HW_VERSION   "10"
#define DEVICE_FW_VERSION   "3026"

#endif // !_VERSION_H_
