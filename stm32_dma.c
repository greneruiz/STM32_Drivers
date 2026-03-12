/**
 * ===================================================================
 *  File Name: stm32_dma.c
 *  Type     : C source file
 *  Purpose  : STM32F DMA driver
 *  Version  : 3.2
 * ===================================================================
 *  Description
 *		* STM32F non-blocking DMA driver.
 *
 * ===================================================================
 *  Revision History
 * Version/Date : v1.0 / 2025-Feb-14 / G.RUIZ
 *		* Initial release
 * ===================================================================
 */

#include "stm32_dma.h"

/// TEMPORARY
//#define STM32F401xC

#if defined( STM32F103xB )
#include "stm32f1xx.h"
#elif defined( STM32F401xC )
#include "stm32f4xx.h"
#endif


#if defined( STM32F401xC )

/**
 * @brief	STM32F4 DMA Interrupts
 */
typedef enum
{
	DMA_FEIF,
	DMA_Invalid,
	DMA_DMEIF,
	DMA_TEIF,
	DMA_HTIF,
	DMA_TCIF
} STM32F4_DMA_InterruptType;


/**
 * @brief	STM32F4 DMA interrupt mask
 */
#define DMA_INTERRUPTS_MASK	0x3DUL


/**
 * @brief	Number of channels per DMA stream, for STM32F401xC
 */
#define DMA_STREAM_CHANNELS 8U


/**
 * @brief	Address spacing for DMA streams
 */
#define DMA_STREAM_ADDR_STRIDE	((uint32_t)DMA1_Stream1_BASE - (uint32_t)DMA1_Stream0_BASE )

/**
 * @brief	Enables the DMA clock.
 * @param	dma	DMA_TypeDef pointer to the selected DMA engine
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 *
 */
static ReturnType stm32_dma_enable_rcc( DMA_TypeDef * dma )
{
	if( dma == DMA1 )
	{
		MODIFY_REG( RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN_Msk, RCC_AHB1ENR_DMA1EN );
	}
	else if( dma == DMA2 )
	{
		MODIFY_REG( RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN_Msk, RCC_AHB1ENR_DMA2EN );			
	}
	else return FAIL;

	return PASS;
}


/**
 * @brief	Switch for the DMA stream to check for a channel's interrupt status.
 * @param	stream	(DMA_Stream_TypeDef *) pointer to the STM32 DMA stream
 * @param	dmaInt	STM32F4_DMA_InterruptType refering to the interrupt bit in question
 * @return	0x00 (FAIL)
 * @return	Interrupt bit status
 * 
 */
static uint32_t stm32_dma_check_interrupts( DMA_Stream_TypeDef * stream, STM32F4_DMA_InterruptType dmaIntType )
{
	uint32_t _dmastatus;
	DMA_TypeDef * thisDMA;
	uintptr_t baseAddr;

	if( (uintptr_t)stream >= (uintptr_t)DMA2_Stream0 )
	{
		thisDMA = DMA2;
		baseAddr = (uintptr_t)DMA2_Stream0;
	}
	else
	{
		thisDMA = DMA1;
		baseAddr = (uintptr_t)DMA1_Stream0;
	}

	uint32_t stream_index = ((uintptr_t)stream - baseAddr ) / DMA_STREAM_ADDR_STRIDE;

	/// TODO: SIMPLIFY. Made like this for debugging
	/* Read the status register, then place the chosen channel's bits to the LSB */
	switch( stream_index )
	{
		case 0:	_dmastatus = READ_BIT( thisDMA->LISR, DMA_INTERRUPTS_MASK << DMA_LISR_FEIF0_Pos ) >> DMA_LISR_FEIF0_Pos;	break;
		case 1:	_dmastatus = READ_BIT( thisDMA->LISR, DMA_INTERRUPTS_MASK << DMA_LISR_FEIF1_Pos ) >> DMA_LISR_FEIF1_Pos;	break;
		case 2:	_dmastatus = READ_BIT( thisDMA->LISR, DMA_INTERRUPTS_MASK << DMA_LISR_FEIF2_Pos ) >> DMA_LISR_FEIF2_Pos;	break;
		case 3:	_dmastatus = READ_BIT( thisDMA->LISR, DMA_INTERRUPTS_MASK << DMA_LISR_FEIF3_Pos ) >> DMA_LISR_FEIF3_Pos;	break;
		case 4:	_dmastatus = READ_BIT( thisDMA->HISR, DMA_INTERRUPTS_MASK << DMA_HISR_FEIF4_Pos ) >> DMA_HISR_FEIF4_Pos;	break;
		case 5:	_dmastatus = READ_BIT( thisDMA->HISR, DMA_INTERRUPTS_MASK << DMA_HISR_FEIF5_Pos ) >> DMA_HISR_FEIF5_Pos;	break;
		case 6:	_dmastatus = READ_BIT( thisDMA->HISR, DMA_INTERRUPTS_MASK << DMA_HISR_FEIF6_Pos ) >> DMA_HISR_FEIF6_Pos;	break;
		case 7:	_dmastatus = READ_BIT( thisDMA->HISR, DMA_INTERRUPTS_MASK << DMA_HISR_FEIF7_Pos ) >> DMA_HISR_FEIF7_Pos;	break;
		default:	return 0x00;
	};

	/* Mask out the other bits except for dmaIntType */
	switch( dmaIntType )
	{
		case DMA_FEIF:	return ((_dmastatus & DMA_LISR_FEIF0_Msk ) >> DMA_LISR_FEIF0_Pos );
		case DMA_DMEIF:	return ((_dmastatus & DMA_LISR_DMEIF0_Msk ) >> DMA_LISR_DMEIF0_Pos );
		case DMA_TEIF:	return ((_dmastatus & DMA_LISR_TEIF0_Msk ) >> DMA_LISR_TEIF0_Pos );
		case DMA_HTIF:	return ((_dmastatus & DMA_LISR_HTIF0_Msk ) >> DMA_LISR_HTIF0_Pos );
		case DMA_TCIF:	return ((_dmastatus & DMA_LISR_TCIF0_Msk ) >> DMA_LISR_TCIF0_Pos );
		case DMA_Invalid:
		default:	return 0x00;
	}
}


/**
 * @brief	Switch for the DMA stream to clear a specific interrupt
 * @param	stream	(DMA_Stream_TypeDef *) pointer to the STM32 DMA stream
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
static ReturnType stm32_dma_chan_clear_this_interrupt( DMA_Stream_TypeDef * stream, STM32F4_DMA_InterruptType dmaIntType )
{
	uint32_t _intMask;
	DMA_TypeDef * thisDMA;
	uintptr_t baseAddr;

	switch( dmaIntType )
	{
		case DMA_FEIF:	_intMask = 0x01UL << DMA_LIFCR_CFEIF0_Pos;	break;
		case DMA_DMEIF:	_intMask = 0x01UL << DMA_LIFCR_CDMEIF0_Pos;	break;
		case DMA_TEIF:	_intMask = 0x01UL << DMA_LIFCR_CTEIF0_Pos;	break;
		case DMA_HTIF:	_intMask = 0x01UL << DMA_LIFCR_CHTIF0_Pos;	break;
		case DMA_TCIF:	_intMask = 0x01UL << DMA_LIFCR_CTCIF0_Pos;	break;
		case DMA_Invalid:
		default:	return FAIL;
	};

	if( (uintptr_t)stream >= (uintptr_t)DMA2_Stream0 )
	{
		thisDMA = DMA2;
		baseAddr = (uintptr_t)DMA2_Stream0;
	}
	else
	{
		thisDMA = DMA1;
		baseAddr = (uintptr_t)DMA1_Stream0;
	}

	uint32_t stream_index = ((uintptr_t)stream - baseAddr ) / DMA_STREAM_ADDR_STRIDE;

	switch( stream_index )
	{
		case 0:	CLEAR_BIT( thisDMA->LIFCR, _intMask << DMA_LIFCR_CFEIF0_Pos );	break;	
		case 1:	CLEAR_BIT( thisDMA->LIFCR, _intMask << DMA_LIFCR_CFEIF1_Pos );	break;	
		case 2:	CLEAR_BIT( thisDMA->LIFCR, _intMask << DMA_LIFCR_CFEIF2_Pos );	break;	
		case 3:	CLEAR_BIT( thisDMA->LIFCR, _intMask << DMA_LIFCR_CFEIF3_Pos );	break;	
		case 4:	CLEAR_BIT( thisDMA->HIFCR, _intMask << DMA_HIFCR_CFEIF4_Pos );	break;	
		case 5:	CLEAR_BIT( thisDMA->HIFCR, _intMask << DMA_HIFCR_CFEIF5_Pos );	break;	
		case 6:	CLEAR_BIT( thisDMA->HIFCR, _intMask << DMA_HIFCR_CFEIF6_Pos );	break;	
		case 7:	CLEAR_BIT( thisDMA->HIFCR, _intMask << DMA_HIFCR_CFEIF7_Pos );	break;	
		default:	return FAIL;
	};

	return PASS;
}


/**
 * @brief	Switch for the DMA stream to clear all interrupts
 * @param	stream	(DMA_Stream_TypeDef *) pointer to the STM32 DMA stream
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
static ReturnType stm32_dma_chan_clear_all_interrupts( DMA_Stream_TypeDef * stream )
{
	DMA_TypeDef * thisDMA;
	uintptr_t baseAddr;

	if( (uintptr_t)stream >= (uintptr_t)DMA2_Stream0 )
	{
		thisDMA = DMA2;
		baseAddr = (uintptr_t)DMA2_Stream0;
	}
	else
	{
		thisDMA = DMA1;
		baseAddr = (uintptr_t)DMA1_Stream0;
	}

	uint32_t stream_index = ((uintptr_t)stream - baseAddr ) / DMA_STREAM_ADDR_STRIDE;

	switch( stream_index )
	{
		case 0:	CLEAR_BIT( thisDMA->LIFCR, DMA_INTERRUPTS_MASK << DMA_LIFCR_CFEIF0_Pos );	break;	
		case 1:	CLEAR_BIT( thisDMA->LIFCR, DMA_INTERRUPTS_MASK << DMA_LIFCR_CFEIF1_Pos );	break;	
		case 2:	CLEAR_BIT( thisDMA->LIFCR, DMA_INTERRUPTS_MASK << DMA_LIFCR_CFEIF2_Pos );	break;	
		case 3:	CLEAR_BIT( thisDMA->LIFCR, DMA_INTERRUPTS_MASK << DMA_LIFCR_CFEIF3_Pos );	break;	
		case 4:	CLEAR_BIT( thisDMA->HIFCR, DMA_INTERRUPTS_MASK << DMA_HIFCR_CFEIF4_Pos );	break;	
		case 5:	CLEAR_BIT( thisDMA->HIFCR, DMA_INTERRUPTS_MASK << DMA_HIFCR_CFEIF5_Pos );	break;	
		case 6:	CLEAR_BIT( thisDMA->HIFCR, DMA_INTERRUPTS_MASK << DMA_HIFCR_CFEIF6_Pos );	break;	
		case 7:	CLEAR_BIT( thisDMA->HIFCR, DMA_INTERRUPTS_MASK << DMA_HIFCR_CFEIF7_Pos );	break;	
		default:	return FAIL;
	};

	return PASS;
}


/**
 * @brief	Configures the Selected DMA stream and channel before a transaction.
 * 			Note: STM32 requires the DMA to be setup before every transaction!
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 *
 */
static ReturnType stm32_dma_config( G_HAL_DMA_Handle * dmaHandle )
{
	uint32_t _sxcr = 0x00UL;
	DMA_Stream_TypeDef * _stream = ( DMA_Stream_TypeDef * )dmaHandle->setup.stream;

	/* Disable the DMA: */
	CLEAR_BIT(_stream->CR, DMA_SxCR_EN );

	/* Clear interrupt flags */
	if( stm32_dma_chan_clear_all_interrupts(_stream ) == FAIL )
		return FAIL;

	/* Select Stream Channel */
	_sxcr = ( dmaHandle->setup.channel << DMA_SxCR_CHSEL_Pos ) & DMA_SxCR_CHSEL_Msk;

	/* Current buffer target (for double buffer mode) */
	_sxcr |= ( dmaHandle->buffer_select << DMA_SxCR_CT_Pos ) & DMA_SxCR_CT_Msk;

	/* Enable/disable double buffer mode */
	_sxcr |= ( dmaHandle->setup.bufferMode << DMA_SxCR_DBM_Pos ) & DMA_SxCR_DBM_Msk;

	/* Set the priority */
	_sxcr |= ( dmaHandle->setup.priority << DMA_SxCR_PL_Pos ) & DMA_SxCR_PL_Msk;

	/* Memory Increment */
	_sxcr |= ( dmaHandle->setup.memory_autoinc << DMA_SxCR_MINC_Pos ) & DMA_SxCR_MINC_Msk;

	/* Peripheral Increment (usually used when interacting with memory-to-memory DMA) */
	_sxcr |= ( dmaHandle->setup.periph_autoinc << DMA_SxCR_PINC_Pos ) & DMA_SxCR_PINC_Msk;

	/* Enable circular mode */
	_sxcr |= ( dmaHandle->setup.circularMode << DMA_SxCR_CIRC_Pos ) & DMA_SxCR_CIRC_Msk;
	
	/* Set transfer direction */
	_sxcr |= ( dmaHandle->setup.transferDirection << DMA_SxCR_DIR_Pos ) & DMA_SxCR_DIR_Msk;

	/* Select peripheral flow controller. If transferDirection is Mem-to-Mem, hardware overrides this to 0x00 */
	_sxcr |= ( dmaHandle->setup.flowController << DMA_SxCR_PFCTRL_Pos ) & DMA_SxCR_PFCTRL_Msk;

	WRITE_REG(_stream->CR, _sxcr );

	/* If Memory-to-Memory, use FIFO mode */
	if( dmaHandle->setup.transferDirection == DMA_MEM_TO_MEM )
	{
		/* Set to 1 to disable Direct Mode */
		SET_BIT(_stream->FCR, DMA_SxFCR_DMDIS );	
		SET_BIT(_stream->FCR, DMA_SxFCR_FTH );
	}
	else
	{
		/* Enable Direct Mode */
		CLEAR_BIT(_stream->FCR, DMA_SxFCR_DMDIS );
	}

	return PASS;
}


/**
 * @brief	Initialize the DMA instance.
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @param	dmaChannel	DMA channel index
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
ReturnType stm32_dma_initialize( G_HAL_DMA_Handle * dmaHandle, uint8_t dmaChannel )
{
	DMA_TypeDef * thisDMA;

	/* We're wasting calculations here because STM32's stream table is dogshit. */
	uint8_t _stream_sel = dmaChannel / DMA_STREAM_CHANNELS;
	switch(_stream_sel )
	{
		case 0:		dmaHandle->setup.stream = (void *)DMA1_Stream0; break;
		case 1:		dmaHandle->setup.stream = (void *)DMA1_Stream1; break;
		case 2:		dmaHandle->setup.stream = (void *)DMA1_Stream2; break;
		case 3:		dmaHandle->setup.stream = (void *)DMA1_Stream3; break;
		case 4:		dmaHandle->setup.stream = (void *)DMA1_Stream4; break;
		case 5:		dmaHandle->setup.stream = (void *)DMA1_Stream5; break;
		case 6:		dmaHandle->setup.stream = (void *)DMA1_Stream6; break;
		case 7:		dmaHandle->setup.stream = (void *)DMA1_Stream7; break;
		case 8:		dmaHandle->setup.stream = (void *)DMA2_Stream0; break;
		case 9:		dmaHandle->setup.stream = (void *)DMA2_Stream1; break;
		case 10:	dmaHandle->setup.stream = (void *)DMA2_Stream2; break;
		case 11:	dmaHandle->setup.stream = (void *)DMA2_Stream3; break;
		case 12:	dmaHandle->setup.stream = (void *)DMA2_Stream4; break;
		case 13:	dmaHandle->setup.stream = (void *)DMA2_Stream5; break;
		case 14:	dmaHandle->setup.stream = (void *)DMA2_Stream6; break;
		case 15:	dmaHandle->setup.stream = (void *)DMA2_Stream7; break;
		default:	dmaHandle->setup.stream = NULL; return FAIL;
	};

	if( (uintptr_t)( dmaHandle->setup.stream ) >= (uintptr_t)DMA2_Stream0 )
		thisDMA = DMA2;
	else
		thisDMA = DMA1;


	if( stm32_dma_enable_rcc( thisDMA ) == FAIL )
	{
		dmaHandle->setup.state = DMA_STATE_ERROR;
		return FAIL;
	}

	dmaHandle->setup.channel = dmaChannel % DMA_STREAM_CHANNELS;
	dmaHandle->setup.state = DMA_STATE_IDLE;

	return PASS;
}


/**
 * @brief	Enables the DMA interrupt events
 * 			Does not enable HTIE interrupt for simplicity.
 * @param	dmaStream	DMA_Stream_TypeDef pointer (DMAx_Streamy)
 * 
 */
#define STM32_DMA_ENABLE_IRQ( dmaStream )			\
	SET_BIT( dmaStream->CR, ( DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE ))


/**
 * @brief	Disables the DMA interrupt events
 * 			Does not enable HTIE interrupt for simplicity.
 * @param	dmaStream	DMA_Stream_TypeDef pointer (DMAx_Streamy)
 * 
 */
#define STM32_DMA_DISABLE_IRQ( dmaStream )			\
	CLEAR_BIT( dmaStream->CR, ( DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE ))

/**
 * @brief	Enables the DMA FIFO interrupt
 * @param	dmaStream	DMA_Stream_TypeDef pointer (DMAx_Streamy)
 * 
 */
#define STM32_DMA_FIFO_ENABLE_IRQ( dmaStream )		\
	SET_BIT( dmaStream->FCR, DMA_SxFCR_FEIE )


/**
 * @brief	Disables the DMA FIFO interrupt
 * @param	dmaStream	DMA_Stream_TypeDef pointer (DMAx_Streamy)
 * 
 */
#define STM32_DMA_FIFO_DISABLE_IRQ( dmaStream )		\
	CLEAR_BIT( dmaStream->FCR, DMA_SxFCR_FEIE )


/**
 * @brief	Arms the DMA interrupts, and enables the DMA engine
 * 			The transferDirection on the G_HAL_DMA_Handle must be
 * 			set up before calling this function.
 * 			Sets the DMA state to BUSY.
 * @param	dmaHandle G_HAL_DMA_Handle pointer defined in g_hal_dma.h
 * 
 */
static void stm32_dma_arm( G_HAL_DMA_Handle * dmaHandle )
{
	/* Set DMA state to Busy */
	/** TODO: 20260309 Check if this works! */
	dmaHandle->setup.state = DMA_STATE_BUSY;

	/* Enable IRQs */
	STM32_DMA_ENABLE_IRQ((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream )) );

	/* Enable FIFO IRQ if transferDirection is Mem-to-Mem */
	if( dmaHandle->setup.transferDirection == DMA_MEM_TO_MEM )
		STM32_DMA_FIFO_ENABLE_IRQ( (( DMA_Stream_TypeDef * )( dmaHandle->setup.stream )) );
		
	/* Enable DMA */
	SET_BIT((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->CR, DMA_SxCR_EN );
}


/**
 * @brief	Disarms the DMA engine, then the DMA interrupts.
 * 			Sets the DMA state to IDLE.
 * 			The transferDirection on the G_HAL_DMA_Handle must be
 * 			set up before calling this function.
 * @param	dmaHandle G_HAL_DMA_Handle defined in g_hal_dma.h
 * 
 */

void stm32_dma_disarm( G_HAL_DMA_Handle * dmaHandle )
{
	/* Disable DMA */
	CLEAR_BIT((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->CR, DMA_SxCR_EN );
	
	/* Disable IRQs */
	STM32_DMA_DISABLE_IRQ((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream )));

	/* Disable FIFO IRQ */
	if( dmaHandle->setup.transferDirection == DMA_MEM_TO_MEM )
		STM32_DMA_FIFO_DISABLE_IRQ((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream )));

	/* Set to Idle state */
	/** TODO: 20260309 Check if this works! */
	dmaHandle->setup.state = DMA_STATE_IDLE;
}


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
ReturnType stm32_start_dma_transaction( G_HAL_DMA_Handle * dmaHandle, uintptr_t sourceAddr, uintptr_t destinationAddr, uintptr_t bufferAddr, uint32_t length )
{
	/* Write the DMA configuration to register. The configuration must be declared at the MCU peripheral level. */
	stm32_dma_config( dmaHandle );

	/* Set number of data */
	WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->NDTR, ( length & DMA_SxNDT ));
	
	/* Assign the source and destination */
	switch( dmaHandle->setup.transferDirection )
	{
		case DMA_PER_TO_MEM:
		case DMA_MEM_TO_MEM:
			WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->PAR, (uint32_t)( sourceAddr ));		/* Peripheral address register */
			WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->M0AR, (uint32_t)( destinationAddr ));	/* Memory 0 address register */
			break;

		case DMA_MEM_TO_PER:
			WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->PAR, (uint32_t)( destinationAddr ));	/* Peripheral address register */
			WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->M0AR, (uint32_t)( sourceAddr ) );		/* Memory 0 address register */
			break;
		
		default: return FAIL;	/// The rest are unsupported for STM32
	};

	/* Memory 1 address register (for double-buffer mode) */
	if( dmaHandle->setup.bufferMode == DMA_DOUBLE_BUFFER )
		WRITE_REG((( DMA_Stream_TypeDef * )( dmaHandle->setup.stream ))->M1AR, (uint32_t)( bufferAddr ));

	stm32_dma_arm( dmaHandle );

	return PASS;
}


/**
 * @brief	Static function that returns the IRQn_Type of the DMA Stream
 * 			and channel, as declared in the STM32 device header file.
 * @param	stream	Void pointer to the DMA_Stream_TypeDef address
 * @return	IRQn_Type name of the DMA IRQ
 * 
 */
static IRQn_Type get_dma_event_irq( void * stream )
{
	uintptr_t baseAddr;
	uint32_t stream_index;

	if( (uintptr_t)stream >= (uintptr_t)DMA2_Stream0 )
	{
		baseAddr = (uintptr_t)DMA2_Stream0;
		stream_index = ((uintptr_t)stream - baseAddr ) / DMA_STREAM_ADDR_STRIDE;	

		switch( stream_index )
		{
			case 0:	return DMA2_Stream0_IRQn;	break;
			case 1:	return DMA2_Stream1_IRQn;	break;
			case 2:	return DMA2_Stream2_IRQn;	break;
			case 3:	return DMA2_Stream3_IRQn;	break;
			case 4:	return DMA2_Stream4_IRQn;	break;
			case 5:	return DMA2_Stream5_IRQn;	break;
			case 6:	return DMA2_Stream6_IRQn;	break;
			case 7:	return DMA2_Stream7_IRQn;	break;
			default:	return DMA2_Stream0_IRQn;
		};
	}
	else
	{
		baseAddr = (uintptr_t)DMA1_Stream0;
		stream_index = ((uintptr_t)stream - baseAddr ) / DMA_STREAM_ADDR_STRIDE;	
		
		switch( stream_index )
		{
			case 0:	return DMA1_Stream0_IRQn;	break;
			case 1:	return DMA1_Stream1_IRQn;	break;
			case 2:	return DMA1_Stream2_IRQn;	break;
			case 3:	return DMA1_Stream3_IRQn;	break;
			case 4:	return DMA1_Stream4_IRQn;	break;
			case 5:	return DMA1_Stream5_IRQn;	break;
			case 6:	return DMA1_Stream6_IRQn;	break;
			case 7:	return DMA1_Stream7_IRQn;	break;
			default:	return DMA1_Stream0_IRQn;
		};
	}
}


/**
 * @brief	Enables the Cortex-M Nested Vector Interrupt Control (NVIC) for DMA events
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @param	event_priority event interrupt priority (0 = highest priority, 15 = lowest priority)
 * 
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
ReturnType stm32_dma_enable_nvic( G_HAL_DMA_Handle * dmaHandle, uint32_t event_priority )
{
	if( dmaHandle->setup.state != DMA_STATE_IDLE )
		return FAIL;

	NVIC_EnableIRQ( get_dma_event_irq( dmaHandle->setup.stream ));
	NVIC_SetPriority( get_dma_event_irq( dmaHandle->setup.stream ), event_priority );

	return PASS;
}


/**
 * @brief	Disables the Cortex Nested Vector Interrupt Control (NVIC) for DMA events
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * 
 */
void stm32_dma_disable_nvic( G_HAL_DMA_Handle * dmaHandle )
{
	NVIC_DisableIRQ( get_dma_event_irq( dmaHandle->setup.stream ));
}


/**
 * @brief	Checks for the current DMA state.
 * @param	dma	G_HAL_DMA_Handle defined in g_hal_dma.h
 * @return	0x01 = PASS (state = DMA_STATE_IDLE)
 * @return	0x00 = FAIL (state != DMA_STATE_IDLE)
 */
ReturnType stm32_dma_irq_status( G_HAL_DMA_Handle * dma )
{
	return dma->setup.state == DMA_STATE_IDLE ? PASS : FAIL;
}


/**
 * ===================================================================
 * @brief	Function definitions - interrupt state machine
 * ===================================================================
 */

/**
 * @brief	State machine for the DMA event request (IRQ)
 * @param	dmaHandle	G_HAL_DMA_Handle defined in g_hal_dma.h
 * 
 */
void STM32_DMA_EventIRQ_FSM( G_HAL_DMA_Handle * dmaHandle )
{
	uint32_t _tcie, _teie, _dmeie, _feie;
	_tcie = stm32_dma_check_interrupts( dmaHandle->setup.stream, DMA_TCIF );
	_teie = stm32_dma_check_interrupts( dmaHandle->setup.stream, DMA_TEIF );
	_dmeie = stm32_dma_check_interrupts( dmaHandle->setup.stream, DMA_DMEIF );
	_feie = stm32_dma_check_interrupts( dmaHandle->setup.stream, DMA_FEIF );

	if(_tcie )
	{
		stm32_dma_chan_clear_this_interrupt( dmaHandle->setup.stream, DMA_TCIF );
		dmaHandle->transfer_done_callback( dmaHandle );
	}
	else if(_dmeie )
	{
		stm32_dma_chan_clear_this_interrupt( dmaHandle->setup.stream, DMA_DMEIF );
		dmaHandle->error_callback( dmaHandle );
	}
	else if(_teie )
	{
		stm32_dma_chan_clear_this_interrupt( dmaHandle->setup.stream, DMA_TEIF );
		dmaHandle->error_callback( dmaHandle );
	}
	else if(_feie )
	{
		stm32_dma_chan_clear_this_interrupt( dmaHandle->setup.stream, DMA_FEIF );
		dmaHandle->error_callback( dmaHandle );
	}
}




#endif /* STM32 selection */