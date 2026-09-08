#include "Modbus.h"
#include "ModbusPort.h"

/***********************Input/Output Coils and holdingRegisters***********************/
#if ((MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0) || (MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0) || (MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED > 0))
RegStructure holdingRegisters[NUMBER_OF_OUTPUT_REGISTERS];
#endif
