/**
 * ===================================================================
 *  File Name: stm32_i2c.h
 *  Type     : C header file
 *  Purpose  : STM32F I2C driver
 *  Version  : 3.1
 * ===================================================================
 *  Description
 *		* STM32F I2C driver. See Revision History for functional scope
 * 		and currently supported STM32F microcontrollers.
 * 		* Requires external pull-up resistors for SCL and SDA lines.
 * ===================================================================
 *  Revision History
 * Version/Date : v1.0 / 2025-Aug-29 / G.RUIZ
 *		* Initial release
 * Version/Date : v1.1 / 2025-Sep-03 / G.RUIZ
 *		* Added STM32F1 prefix
 *		* Replaced argument datatypes with stdint.h datatypes
 *		* Added option to change MCU frequency
 *		* Renamed to stm32f1_i2c
 * Version/Date : v2.0 / 2025-Oct-12 / G.RUIZ
 *		* Renamed to stm32_i2c
 *		* Added STM32F4xx compile-time polymorphism
 * Version/Date : v2.1 / 2025-Oct-18 / G.RUIZ
 *		* Fixed bug at stm32_i2c_read() on block-reads: stop bit is 
 * 	now set thru SET_BIT (previously was thru WRITE_REG which 
 *		turns off the PE bit)
 * Version/Date : v2.2 / 2025-Oct-24 / G.RUIZ
 *		* mcu_period_ns now integer-calculated
 *		* changed uint32_t mcu_period_hz to uint8_t mcu_period_mhz;
 * Version/Date : v3.0 / 2025-Dec-20 / G.RUIZ
 * 		* Overhauled for G_HAL compatibility
 * 		* Features included or limited to:
 * 			- master mode
 * 			- speed modes: standard, fast
 * 		* Features NOT included:
 * 			- target mode
 * 			- 10-bit addressing
 * 		* Features to be implemented on the next version:
 * 			- non-blocking send and receive
 * 			- DMA communication
 * Version/Date : v3.1 / 2025-Dec-25 / G.RUIZ
 * 		* Merry christmas 25-25 !!
 * 		* Implemented interrupt-based functions
 * 		* Functions now requre G_HAL_I2C_Handle as argument
 * 		(previously used I2C_Object)
 * ===================================================================
 */


#ifndef __STM32F_I2C_H__
#define __STM32F_I2C_H__


#include <stdint.h>

/* Header file containing the necessary enums and structures */
#include "g_hal_i2c.h"



/**
 * ===================================================================
 * @brief	Function declarations - blocking
 * ===================================================================
 */

 /**
 * @brief	Initialize the selected I2C channel.
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_def.h
 * @param	mcu_freq_mhz - MCU system clock frequency, in MHz
 * @return	0x00 = FAIL
 * @return	0x01 = PASS
 * 
 */
extern ReturnType stm32_i2c_initialize( G_HAL_I2C_Handle * i2c, uint8_t mcu_freq_mhz );

/**
 * @brief	Blocking function for I2C start (or re-start)
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_def.h
 *
 */
extern void stm32_i2c_start( G_HAL_I2C_Handle * i2c );

/**
 * @brief	Blocking function for I2C stop
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_def.h
 *
 */
extern void stm32_i2c_stop( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Blocking function for I2C write sequence.
 * 			Starts, sends device address, transmits bytes, and optionally stops I2C.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_def.h
 * 
 */
extern void stm32_i2c_send( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Blocking function for I2C read sequence.
 * 			Starts, sends device address and read bit, receives bytes, and stops I2C.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_def.h
 * 
 */
extern void stm32_i2c_receive( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Send a byte array, no start/stop sequences
 * @param	i2c	G_HAL_I2C_Handle (defined in g_hal_def.h)
 * 
 */
extern void stm32_i2c_send_bytes( G_HAL_I2C_Handle * i2c );


/**
 * ===================================================================
 * @brief	Function declarations - non-blockinng
 * ===================================================================
 */

/**
 * @brief	Non-blocking function for I2C write sequence.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	PASS	- successfully started the I2C transaction
 * @return	FAIU	- failed to initiate the I2C transaction
 * 
 */
extern ReturnType stm32_i2c_send_nb( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Non-blocking function for I2C read sequence.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	PASS	- successfully started the I2C transaction
 * @return	FAIL	- failed to initiate the I2C transaction
 * 
 */
extern ReturnType stm32_i2c_receive_nb( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Checks for the current I2C state.
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	0x01 = PASS (state = I2C_STATE_IDLE)
 * @return	0x00 = FAIL (state != I2C_STATE_IDLE)
 */
ReturnType stm32_i2c_irq_status( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Enables the Cortex-M Nested Vector Interrupt Control (NVIC) for I2C events & errors
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
extern ReturnType stm32_i2c_enable_nvic( G_HAL_I2C_Handle * i2c );


/**
 * @brief	Sets the Cortex-M Nested Vector Interrupt Control (NVIC) priorities for I2C events.
 * 			Non-zero value for the interrupt priority (0 = highest priority, 15 = lowest priority).
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @param	event_priority event interrupt priority (0-15)
 * @param	error_priority error interrupt priority (0-15)
 * 
 */
extern void stm32_i2c_set_nvic_priority( G_HAL_I2C_Handle * i2c, uint32_t event_priority, uint32_t error_priority );


/**
 * @brief	Disables the Cortex Nested Vector Interrupt Control (NVIC) for I2C events & errors
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void stm32_i2c_disable_nvic( G_HAL_I2C_Handle * i2c );


/**
 * ===================================================================
 * @brief	Function declarations - interrupt routines
 * ===================================================================
 */

/**
 * @brief	State machine for the I2C interrupt event request (IRQ)
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
extern void STM32_I2C_EventIRQ_FSM( G_HAL_I2C_Handle * i2c );

/**
 * @brief	State machine for the I2C interrupt event request (IRQ)
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
extern void STM32_I2C_ErrorIRQ_FSM( G_HAL_I2C_Handle * i2c );



/**
 * ===================================================================
 * @brief	Function declarations - DMA
 * ===================================================================
 */

/// TODO



#endif /*__STM32F_I2C_H__*/