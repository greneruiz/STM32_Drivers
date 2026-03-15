/**
 * ===================================================================
 *  File Name: stm32_dma.h
 *  Type     : C header file
 *  Purpose  : STM32F DMA driver
 *  Version  : 1.1
 * ===================================================================
 *  Description
 *		* STM32F non-blocking DMA driver.
 *
 * ===================================================================
 *  Revision History
 * Version/Date : v1.0 / 2026-Feb-14 / G.RUIZ
 *		* Initial release
 * Version/Date : v1.1 / 2026-Mar-15 / G.RUIZ
 * 		* Deprecated NVIC functions:
 * 			stm32_dma_enable_nvic()
 * 			stm32_dma_disable_nvic()
 * ===================================================================
 */

 #ifndef __STM32F_DMA_H__
#define __STM32F_DMA_H__


#include <stdint.h>

/* Header file containing the necessary enums and structures */
#include "g_hal_dma.h"


/**
 * ===================================================================
 * @brief	Function declarations - non-blockinng
 * ===================================================================
 */

/**
 * @brief	Initialize the DMA instance.
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @param	dmaChannel	DMA channel index.
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
ReturnType stm32_dma_initialize( G_HAL_DMA_Handle * dmaHandle, uint8_t dmaChannel );


/**
 * @brief	Initiates a DMA transaction.
 * @param	dmaHandle		G_HAL_DMA_Handle defined in g_hal_dma.h
 * @param	sourceAddr		The data source address
 * @param	destinationAddr	The data destination address
 * @param	bufferAddr		The data buffer's address for double-buffer mode.
 * 							Can be used for both source and destination, depending on the transferDirection.
 * @param	length			Number of data to transmit.
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
ReturnType stm32_start_dma_transaction( G_HAL_DMA_Handle * dmaHandle, uintptr_t sourceAddr, uintptr_t destinationAddr, uintptr_t bufferAddr, uint32_t length );


/** TO-DO 
 * 
*/
// void stm32_abort_dma_transaction();


/**
 * @brief	Checks for the current DMA state.
 * @param	dma	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @return	0x01 = PASS (state = DMA_STATE_IDLE)
 * @return	0x00 = FAIL (state != DMA_STATE_IDLE)
 */
ReturnType stm32_dma_irq_status( G_HAL_DMA_Handle * dma );


/**
 * @brief	Disarms the DMA engine, then the DMA interrupts
 * 			The transferDirection on the G_HAL_DMA_Handle must be
 * 			set up before calling this function.
 * @param	dmaHandle G_HAL_DMA_Handle defined in g_hal_dma.h
 * 
 */

void stm32_dma_disarm( G_HAL_DMA_Handle * dmaHandle );



/**
 * ===================================================================
 * @brief	Function declarations - interrupt routines
 * ===================================================================
 */

/**
 * @brief	State machine for the DMA event request (IRQ)
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * 
 */
void STM32_DMA_EventIRQ_FSM( G_HAL_DMA_Handle * dmaHandle );





#endif /*__STM32F_DMA_H__*/