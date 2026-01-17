/**
 * ===================================================================
 *  File Name: stm32_i2c.c
 *  Type     : C source file
 *  Purpose  : STM32F I2C driver
 *  Version  : 3.1
 * ===================================================================
 *  Description
 *		* STM32F I2C driver. See Revision History for functional scope
 * 		and currently supported STM32F microcontrollers.
 * 		* Requires external pull-up resistors for SCL and SDA lines.
 * 		* Strictly follows Controller Transmitter and Receiver as
 * 		defined in RM0368 (STM32F4), except for Receiver-2 bytes in
 * 		non-blocking mode.
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
 * ===================================================================
 */


#include "stm32_i2c.h"


#if defined( STM32F103xB )
#include "stm32f1xx.h"
#elif defined( STM32F401xC )
#include "stm32f4xx.h"
#endif



/**
 * Definitions for speed mode timings
 */
#define FM_TRE_NS		( 300U			)
#define FM_PERIOD_NS	( 2500U			)
#define SM_TRE_NS		( 1000U			)
#define SM_PERIOD_NS	( 10000U		)



/**
 * @brief	Select the I2C base address
 */
#if defined( STM32F103xB )
#define SELECT_I2C_CHAN( ch )			\
	do									\
	{									\
		switch( ch )					\
		{								\
			case 0x01:					\
				_channel = I2C1; break;	\
			case 0x02:					\
				_channel = I2C2; break;	\
			default:					\
				_channel = I2C1;		\
		};								\
	} while(0)

#elif defined( STM32F401xC )
#define SELECT_I2C_CHAN( ch )			\
	do									\
	{									\
		switch( ch )					\
		{								\
			case 0x01:					\
				_channel = I2C1; break;	\
			case 0x02:					\
				_channel = I2C2; break;	\
			case 0x03:					\
				_channel = I2C3; break;	\
			default:					\
				_channel = I2C1;		\
		};								\
	} while(0)
#endif


/**
 * ===================================================================
 * @brief	Function declarations - static functions
 * ===================================================================
 */

static ReturnType stm32_i2c_config_pins( I2C_TypeDef * i2c_channel );
static ReturnType stm32_i2c_config_rcc( I2C_TypeDef * i2c_channel );
static void stm32_i2c_config( I2C_TypeDef * i2c_channel, uint8_t mcu_freq_mhz, I2C_Speed speed_mode );

static IRQn_Type get_i2c_event_irq( I2C_Channel channel );
static IRQn_Type get_i2c_error_irq( I2C_Channel channel );



/**
 * ===================================================================
 * @brief	Function definitions - public functions
 * ===================================================================
 */

/**
 * @brief	Initialize the selected I2C channel.
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @param	mcu_freq_mhz - MCU system clock frequency, in MHz
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
ReturnType stm32_i2c_initialize( G_HAL_I2C_Handle * i2c, uint8_t mcu_freq_mhz )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	if( stm32_i2c_config_rcc(_channel ) == FAIL ) return FAIL;
	if( stm32_i2c_config_pins(_channel ) == FAIL ) return FAIL;
	stm32_i2c_config(_channel, mcu_freq_mhz, i2c->setup.i2c_speed );

	i2c->setup.state = I2C_STATE_IDLE;

	return PASS;
}


/**
 * @brief	Blocking function for I2C start (or re-start)
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_i2c.h
 *
 */
void stm32_i2c_start( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	SET_BIT(_channel->CR1, I2C_CR1_START );
	while( !READ_BIT(_channel->SR1, I2C_SR1_SB )){}
}


/**
 * @brief	Blocking function for I2C stop
 * @param	i2c - G_HAL_I2C_Handle defined in g_hal_i2c.h
 *
 */
void stm32_i2c_stop( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	SET_BIT(_channel->CR1, I2C_CR1_STOP );
	i2c->setup.state = I2C_STATE_IDLE;
}


/**
 * @brief	Send a byte array, no start/stop sequences
 * @param	i2c	G_HAL_I2C_Handle (defined in g_hal_i2c.h)
 * 
 */
void stm32_i2c_send_bytes( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	i2c->setup.state = I2C_STATE_TX_BUSY;

	for( uint16_t i = i2c->txSize; i > 0; i-- )
	{
		WRITE_REG(_channel->DR, *i2c->txBuffer++ );
		while( !READ_BIT(_channel->SR1, I2C_SR1_TXE )){}		/* Wait for byte tx */
	}
}

/**
 * @brief	Blocking function for I2C write sequence.
 * 			Starts, sends device address, transmits bytes, and optionally stops I2C.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void stm32_i2c_send( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	i2c->setup.state = I2C_STATE_TX_BUSY;

	/* Start sequence */
	SET_BIT(_channel->CR1, I2C_CR1_START );
	while( !READ_BIT(_channel->SR1, I2C_SR1_SB )){}
	
	/* Send I2C device address */
	WRITE_REG(_channel->DR, i2c->tgtDevAddr );							/* Write device id to DR buffer */

	if( i2c->txSize == 0U )
	{
		i2c->setup.state = I2C_STATE_IDLE;
		return;
	}

	while( !READ_BIT(_channel->SR1, I2C_SR1_ADDR )){}					/* Wait until AddrStatus is asserted */
	READ_REG(_channel->SR2 );											/* Read SR2 to clear SR1[ADDR] bit */
	while( !READ_BIT(_channel->SR1, I2C_SR1_TXE )){}					/* Wait for ADDR tx ---> could this be removed? */

	/* Start sending bytes */
	for( i2c->setup.xCounter = i2c->txSize; i2c->setup.xCounter > 0; i2c->setup.xCounter-- )
	{
		WRITE_REG(_channel->DR, *i2c->txBuffer++ );
		while( !READ_BIT(_channel->SR1, I2C_SR1_TXE )){}				/* Wait for byte tx */
	}
	while( !READ_BIT(_channel->SR1, I2C_SR1_BTF )){}					/* Wait for last byte tx */

	/* If not planning to restart, do the stop sequence */
	if( i2c->restartMode == I2C_RESTART_DISABLED )
	{
		SET_BIT(_channel->CR1, I2C_CR1_STOP );
		i2c->setup.state = I2C_STATE_IDLE;
	}
}


/**
 * @brief	Blocking function for I2C read sequence.
 * 			Starts, sends device address and read bit, receives bytes, and stops I2C.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void stm32_i2c_receive( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	i2c->setup.state = I2C_STATE_RX_BUSY;
	i2c->setup.xCounter = i2c->rxSize;

	/* Start sequence */
	SET_BIT(_channel->CR1, I2C_CR1_START );
	while( !READ_BIT(_channel->SR1, I2C_SR1_SB )){}

	/* Send I2C device address */
	WRITE_REG(_channel->DR, ( i2c->tgtDevAddr | 0x01U ));

	/* Begin reading the byte/s */
	if( i2c->rxSize == 1U )
	{
		CLEAR_BIT(_channel->CR1, I2C_CR1_ACK );
		while( !READ_BIT(_channel->SR1, I2C_SR1_ADDR )){}				/* Wait until AddrStatus is asserted */
		READ_REG(_channel->SR2 );										/* Read SR2 to clear SR1[ADDR] bit */
		SET_BIT(_channel->CR1, I2C_CR1_STOP );
		while( !READ_BIT(_channel->SR1, I2C_SR1_RXNE )){}				/* Wait until byte rx is done */
		*i2c->rxBuffer = READ_REG(_channel->DR );
	}
	else
	{
		while( !READ_BIT(_channel->SR1, I2C_SR1_ADDR )){}				/* Wait until AddrStatus is asserted */
		if( i2c->rxSize == 2U )
		{
			CLEAR_BIT(_channel->CR1, I2C_CR1_ACK );
			SET_BIT(_channel->CR1, I2C_CR1_POS );
		}
		READ_REG(_channel->SR2 );										/* Read SR2 to clear SR1[ADDR] bit */
	
		if( i2c->rxSize > 2U )
			SET_BIT(_channel->CR1, I2C_CR1_ACK );

		while( i2c->setup.xCounter > 0U )
		{
			if( i2c->setup.xCounter == 3U )
			{
				while( !READ_BIT(_channel->SR1, I2C_SR1_BTF )){}		/* Wait until data[2] is received */
				CLEAR_BIT(_channel->CR1, I2C_CR1_ACK );
				*i2c->rxBuffer++ = READ_REG(_channel->DR );				/* Read data[2] */
				i2c->setup.xCounter--;
			}
			else if( i2c->setup.xCounter == 2U )
			{
				while( !READ_BIT(_channel->SR1, I2C_SR1_BTF )){}		/* Wait data[1] and data[0] are received */
				SET_BIT(_channel->CR1, I2C_CR1_STOP );					/* Generate Stop */
				*i2c->rxBuffer++ = READ_REG(_channel->DR );				/* Read data[1] */
				*i2c->rxBuffer = READ_REG(_channel->DR );				/* Read data[0] */
				i2c->setup.xCounter = 0U;
			}
			else
			{
				while( !READ_BIT(_channel->SR1, I2C_SR1_RXNE )){}		/* Wait until byte rx is done */
				*i2c->rxBuffer++ = READ_REG(_channel->DR );
				i2c->setup.xCounter--;
			}
		}
		
		CLEAR_BIT(_channel->CR1, I2C_CR1_POS );							/* Cleanup: Turn off POS bit */
	}

	i2c->setup.state = I2C_STATE_IDLE;
}


/**
 * @brief	Enables the Cortex-M Nested Vector Interrupt Control (NVIC) for I2C events & errors
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	0x00 (FAIL)
 * @return	0x01 (PASS)
 * 
 */
ReturnType stm32_i2c_enable_nvic( G_HAL_I2C_Handle * i2c )
{
	if( i2c->setup.state != I2C_STATE_IDLE )
		return FAIL;

	NVIC_EnableIRQ( get_i2c_event_irq( i2c->setup.channel ));
//	NVIC_EnableIRQ( get_i2c_error_irq( i2c->setup.channel ));
	
	return PASS;
}


/**
 * @brief	Sets the Cortex-M Nested Vector Interrupt Control (NVIC) priorities for I2C events.
 * 			Non-zero value for the interrupt priority (0 = highest priority, 15 = lowest priority).
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @param	event_priority event interrupt priority (0-15)
 * @param	error_priority error interrupt priority (0-15)
 * 
 */
void stm32_i2c_set_nvic_priority( G_HAL_I2C_Handle * i2c, uint32_t event_priority, uint32_t error_priority )
{
	NVIC_SetPriority( get_i2c_event_irq( i2c->setup.channel ), event_priority );
	NVIC_SetPriority( get_i2c_error_irq( i2c->setup.channel ), error_priority );
}


/**
 * @brief	Disables the Cortex Nested Vector Interrupt Control (NVIC) for I2C events & errors
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void stm32_i2c_disable_nvic( G_HAL_I2C_Handle * i2c )
{
	NVIC_DisableIRQ( get_i2c_event_irq( i2c->setup.channel ));
	NVIC_DisableIRQ( get_i2c_error_irq( i2c->setup.channel ));
}


/**
 * @brief	Enables the I2C interrupt events
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
#define STM32_I2C_ENABLE_IRQ( chan )	\
	SET_BIT( chan->CR2, ( I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN ))		

/**
 * @brief	Disables the I2C interrupt events
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
#define STM32_I2C_DISABLE_IRQ( chan )			\
	CLEAR_BIT(chan->CR2, ( I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN ))


/**
 * @brief	Non-blocking function for I2C write sequence.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	PASS	- successfully started the I2C transaction
 * @return	FAIU	- failed to initiate the I2C transaction
 * 
 */
ReturnType stm32_i2c_send_nb( G_HAL_I2C_Handle * i2c )
{
	if( i2c->setup.state != I2C_STATE_IDLE )
		return FAIL;

	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );
	
	i2c->setup.state = I2C_STATE_TX_BUSY;
	i2c->setup.xCounter = i2c->txSize;
	STM32_I2C_ENABLE_IRQ(_channel );
	SET_BIT(_channel->CR1, I2C_CR1_START );
	return PASS;
}


/**
 * @brief	Non-blocking function for I2C read sequence.
 * @param	i2c	G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	PASS	- successfully started the I2C transaction
 * @return	FAIL	- failed to initiate the I2C transaction
 * 
 */
ReturnType stm32_i2c_receive_nb( G_HAL_I2C_Handle * i2c )
{
	if( i2c->setup.state != I2C_STATE_IDLE )
		return FAIL;

	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );


	i2c->setup.state = I2C_STATE_RX_BUSY;
	i2c->setup.xCounter = i2c->rxSize;

	SET_BIT(_channel->CR1, I2C_CR1_START );
	STM32_I2C_ENABLE_IRQ(_channel );		/* Send the Start before enabling IRQ, in case BTF+TXE is previously asserted */

	return PASS;
}


static inline void STM32_I2C_State_Start_Detected( G_HAL_I2C_Handle * i2c, I2C_TypeDef * channel )
{
	if( i2c->setup.state == I2C_STATE_TX_BUSY )
	{
		WRITE_REG( channel->DR, i2c->tgtDevAddr );
	}
	else if( i2c->setup.state == I2C_STATE_RX_BUSY )
	{
		WRITE_REG( channel->DR, i2c->tgtDevAddr | I2C_READ );

		/* For single-byte reads, clear the ACK bit */
		if( i2c->rxSize == 1U )
			CLEAR_BIT( channel->CR1, I2C_CR1_ACK );
	}
}

static inline void STM32_I2C_State_Address_Sent( G_HAL_I2C_Handle * i2c, I2C_TypeDef * channel )
{
	/* clear the address status bit */
	READ_REG( channel->SR2 );		

	/* If read mode and receive count is 1 byte, initiate stop */
	if( i2c->setup.state == I2C_STATE_RX_BUSY )
	{
		if( i2c->rxSize == 1U )
			SET_BIT( channel->CR1, I2C_CR1_STOP );
		else
			SET_BIT( channel->CR1, I2C_CR1_ACK );
	}
}

static inline void STM32_I2C_State_Byte_Transfer_Finished( G_HAL_I2C_Handle * i2c, I2C_TypeDef * channel )
{
	if(( i2c->setup.state == I2C_STATE_TX_BUSY ) && ( READ_BIT( channel->SR1, I2C_SR1_TXE )))
	{
		if( i2c->setup.xCounter > 0 )
		{
			WRITE_REG( channel->DR, i2c->txBuffer[i2c->txSize - i2c->setup.xCounter] );
			i2c->setup.xCounter--;
		}

		if( i2c->setup.xCounter == 0U )
		{
			if( i2c->restartMode == I2C_RESTART_DISABLED )
				SET_BIT( channel->CR1, I2C_CR1_STOP );
			
			STM32_I2C_DISABLE_IRQ( channel );
			i2c->setup.state = I2C_STATE_IDLE;
		}
	}
	else if( i2c->setup.state == I2C_STATE_RX_BUSY )
	{
		if( i2c->setup.xCounter == 3U )
		{
			CLEAR_BIT( channel->CR1, I2C_CR1_ACK );
			i2c->rxBuffer[i2c->rxSize - i2c->setup.xCounter] = READ_REG( channel->DR );	/* Read data[2] */
			i2c->setup.xCounter--;
		}
	}
}

static inline void STM32_I2C_State_Transmit_Buffer_Empty( G_HAL_I2C_Handle * i2c, I2C_TypeDef * channel )
{
	if(( i2c->setup.state == I2C_STATE_TX_BUSY ) && ( i2c->setup.xCounter > 0 ))
	{
		WRITE_REG( channel->DR, i2c->txBuffer[i2c->txSize - i2c->setup.xCounter] );
		i2c->setup.xCounter--;
	}
}

static inline void STM32_I2C_State_Receive_Buffer_Not_Empty( G_HAL_I2C_Handle * i2c, I2C_TypeDef * channel )
{
	if( i2c->setup.state == I2C_STATE_RX_BUSY )
	{
		if(( i2c->setup.xCounter != 3U ) && ( i2c->setup.xCounter > 0U ))
		{
			i2c->rxBuffer[i2c->rxSize - i2c->setup.xCounter] = READ_REG( channel->DR );
			i2c->setup.xCounter--;

			if( i2c->setup.xCounter == 1U )
				CLEAR_BIT( channel->CR1, I2C_CR1_ACK );
		}

		if( i2c->setup.xCounter == 0U )
		{
			if( i2c->rxSize > 1U )
				SET_BIT( channel->CR1, I2C_CR1_STOP );

			STM32_I2C_DISABLE_IRQ( channel );
			CLEAR_BIT( channel->CR1, I2C_CR1_POS );
			i2c->setup.state = I2C_STATE_IDLE;
		}
	}
}

/**
 * @brief	State machine for the I2C interrupt event request (IRQ)
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void STM32_I2C_EventIRQ_FSM( G_HAL_I2C_Handle * i2c )
{
	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );

	uint32_t _itevt, _itbuf, _cond;
	_itevt = READ_BIT(_channel->CR2, I2C_CR2_ITEVTEN );
	_itbuf = READ_BIT(_channel->CR2, I2C_CR2_ITBUFEN );
	_cond = READ_REG(_channel->SR1 );

 	/* state_check_start: check start bit, then send i2c address */
	if( (_cond & I2C_SR1_SB ) && _itevt )
	{
		STM32_I2C_State_Start_Detected( i2c, _channel );
	}

	/* state_addr_sent: clear address bit */
	else if((_cond & I2C_SR1_ADDR ) && _itevt )
	{
		STM32_I2C_State_Address_Sent( i2c, _channel );
	}

	/* state_post_btf: Byte transfer finished */
	else if((_cond & I2C_SR1_BTF ) && _itevt )
	{
		STM32_I2C_State_Byte_Transfer_Finished( i2c, _channel );
	}

	/* state_transmit_bytes */
	else if((_cond & I2C_SR1_TXE ) && _itevt && _itbuf )
	{
		STM32_I2C_State_Transmit_Buffer_Empty( i2c, _channel );
	}

	/* state_receive_bytes:  */
	else if((_cond & I2C_SR1_RXNE ) && (_itevt && _itbuf ))
	{
		STM32_I2C_State_Receive_Buffer_Not_Empty( i2c, _channel );
	}
}


/**
 * @brief	State machine for the I2C interrupt event request (IRQ)
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * 
 */
void STM32_I2C_ErrorIRQ_FSM( G_HAL_I2C_Handle * i2c )
{
	/// TODO: add a qualifier if USART is enabled
	// printf( " [ERROR] Encountered an I2C IRQ error!!\n\n" );
	//i2c->setup.state = I2C_STATE_ERROR;

	I2C_TypeDef * _channel;
	SELECT_I2C_CHAN( i2c->setup.channel );
	uint32_t _cond = READ_REG(_channel->SR1 );

	STM32_I2C_DISABLE_IRQ(_channel );
	if(_cond & I2C_SR1_BERR )
	{
		/* Abort the current transmission */
		CLEAR_BIT(_channel->SR1, I2C_SR1_BERR );
		i2c->setup.state = I2C_STATE_ERROR;
	}
	else if(_cond & I2C_SR1_AF )
	{
		CLEAR_BIT(_channel->SR1, I2C_SR1_AF );
		SET_BIT(_channel->CR1, I2C_CR1_STOP );
		i2c->setup.state = I2C_STATE_IDLE;
	}
	else if(_cond & I2C_SR1_ARLO )
	{
		CLEAR_BIT(_channel->SR1, I2C_SR1_ARLO );
		i2c->setup.state = I2C_STATE_ERROR;
	}
	else if(_cond & I2C_SR1_OVR )
	{
		CLEAR_BIT(_channel->SR1, I2C_SR1_OVR );
		i2c->setup.state = I2C_STATE_ERROR;
	}
}


/**
 * @brief	Checks for the current I2C state.
 * @param	i2c G_HAL_I2C_Handle defined in g_hal_i2c.h
 * @return	0x01 = PASS (state = I2C_STATE_IDLE)
 * @return	0x00 = FAIL (state != I2C_STATE_IDLE)
 */
ReturnType stm32_i2c_irq_status( G_HAL_I2C_Handle * i2c )
{
	return i2c->setup.state == I2C_STATE_IDLE ? PASS : FAIL;
}

/**
 * ===================================================================
 * @brief	Function definitions - static functions
 * ===================================================================
 */

/**
 * @brief	Set up I2C configuration registers.
 * 			Common to STM32F devices.
 * @param	i2c_channel Pointer to STM32 I2Cn base address
 * @param	mcu_freq_mhz - MCU system clock frequency, in MHz
 * @param	speed_mode - I2C speed: Standard/Fast/Fast-Plus
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
static void stm32_i2c_config( I2C_TypeDef * i2c_channel, uint8_t mcu_freq_mhz, I2C_Speed speed_mode )
{
	uint8_t mcu_period_ns = 1000U / mcu_freq_mhz;

	/* Toggle I2Cx Reset */
	SET_BIT( i2c_channel->CR1, I2C_CR1_SWRST );
	CLEAR_BIT( i2c_channel->CR1, I2C_CR1_SWRST );				

	uint32_t mode_period_ns = speed_mode == I2C_SM_100KHZ ? SM_PERIOD_NS : FM_PERIOD_NS;
	uint32_t tre_ns = speed_mode == I2C_SM_100KHZ ? SM_TRE_NS : FM_TRE_NS; 
	uint32_t ccr_freq = (uint16_t)( ( mode_period_ns / 2U ) / (uint16_t)mcu_period_ns );
	uint32_t trise_val = (uint16_t)( ( tre_ns / (uint16_t)mcu_period_ns ) + 1U );

	CLEAR_BIT( i2c_channel->CR1, I2C_CR1_PE );							/* Disable I2C */
	MODIFY_REG( i2c_channel->CR2, I2C_CR2_FREQ, mcu_freq_mhz );			/* Set system FREQ = 8MHz */
	MODIFY_REG( i2c_channel->CCR, I2C_CCR_CCR, ccr_freq );				/* Standard mode, set frequency */
	MODIFY_REG( i2c_channel->TRISE, I2C_TRISE_TRISE, trise_val );		/* Set SCL rising edge time */
	SET_BIT( i2c_channel->CR1, I2C_CR1_PE );							/* Enable I2C */
}


#if defined( STM32F103xB )

/**
 * @brief	Configure RCC for GPIOs, and I2C registers
 * @param	i2c_channel Pointer to STM32 I2Cn base address
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
static ReturnType stm32_i2c_config_rcc( I2C_TypeDef * i2c_channel )
{
	/* GPIOB & Alt Function clock enable */
	MODIFY_REG( RCC->APB2ENR, ( RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN ), ( RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN ));

	/* I2C Clock enable */
	if( i2c_channel == I2C1 )
		MODIFY_REG( RCC->APB1ENR, (RCC_APB1ENR_I2C1EN_Msk ), RCC_APB1ENR_I2C1EN );
	else if( i2c_channel == I2C2 )
		MODIFY_REG( RCC->APB1ENR, (RCC_APB1ENR_I2C1EN_Msk ), RCC_APB1ENR_I2C1EN );
	else
		return FAIL;
	
	return PASS;
}


/**
 * @brief	I2C SCL and SDA configurations
 * @param	i2c_channel Pointer to STM32 I2Cn base address
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
static ReturnType stm32_i2c_config_pins( I2C_TypeDef * i2c_channel )
{
	if( i2c_channel == I2C1 )
	{
		/* Configure GPIO: Output 10MHz, Open-drain */
		MODIFY_REG( GPIOB->CRL, ( GPIO_CRL_CNF6_Msk | GPIO_CRL_MODE6_Msk ), ( GPIO_CRL_CNF6 | GPIO_CRL_MODE6_0 ));
		MODIFY_REG( GPIOB->CRL, ( GPIO_CRL_CNF7_Msk | GPIO_CRL_MODE7_Msk ), ( GPIO_CRL_CNF7 | GPIO_CRL_MODE7_0 ));
	}
	else if( i2c_channel == I2C2 )
	{
		/* Configure GPIO: Output 10MHz, Open-drain */
		MODIFY_REG( GPIOB->CRH, ( GPIO_CRH_CNF10_Msk | GPIO_CRH_MODE10_Msk ), ( GPIO_CRH_CNF10 | GPIO_CRH_MODE10_0 ));
		MODIFY_REG( GPIOB->CRH, ( GPIO_CRH_CNF11_Msk | GPIO_CRH_MODE11_Msk ), ( GPIO_CRH_CNF11 | GPIO_CRH_MODE11_0 ));
	}
	else
		return FAIL;

	return PASS;
}

#elif defined( STM32F401xC )

/**
 * @brief	Configure RCC for GPIOs, and I2C registers
 * @param	i2c_channel Pointer to STM32 I2Cn base address
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
static ReturnType stm32_i2c_config_rcc( I2C_TypeDef * i2c_channel )
{
	if( i2c_channel == I2C1 )
	{
		SET_BIT( RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN );
		SET_BIT( RCC->APB1ENR, RCC_APB1ENR_I2C1EN );
	}
	else if( i2c_channel == I2C2 )
	{
		SET_BIT( RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN );
		SET_BIT( RCC->APB1ENR, RCC_APB1ENR_I2C2EN );
	}
	else if( i2c_channel == I2C3 )
	{
		SET_BIT( RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN );
		SET_BIT( RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN );
		SET_BIT( RCC->APB1ENR, RCC_APB1ENR_I2C3EN );
	}
	else
		return FAIL;

	return PASS;
}


/**
 * @brief	I2C SCL and SDA configurations
 * @param	i2c_channel Pointer to STM32 I2Cn base address
 * @return	0x01 = Success; 0x00 = Failed
 * 
 */
static ReturnType stm32_i2c_config_pins( I2C_TypeDef * i2c_channel )
{
	if( i2c_channel == I2C1 )
	{
		/* SCL = PB6; SDA = PB7 */
		MODIFY_REG( GPIOB->MODER, GPIO_MODER_MODER6_Msk, GPIO_MODER_MODER6_1 );		
		MODIFY_REG( GPIOB->MODER, GPIO_MODER_MODER7_Msk, GPIO_MODER_MODER7_1 );		
		SET_BIT( GPIOB->OTYPER, GPIO_OTYPER_OT6 );
		SET_BIT( GPIOB->OTYPER, GPIO_OTYPER_OT7 );
		MODIFY_REG( GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED6_Msk, GPIO_OSPEEDR_OSPEED6_0 );
		MODIFY_REG( GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED7_Msk, GPIO_OSPEEDR_OSPEED7_0 );

		/* Alt Function: I2C */
		MODIFY_REG( GPIOB->AFR[0], GPIO_AFRL_AFSEL6_Msk, GPIO_AFRL_AFSEL6_2 );
		MODIFY_REG( GPIOB->AFR[0], GPIO_AFRL_AFSEL7_Msk, GPIO_AFRL_AFSEL7_2 );
	}
	else if( i2c_channel == I2C2 )
	{
		/* SCL = PB10; SDA = PB11 */
		MODIFY_REG( GPIOB->MODER, GPIO_MODER_MODER10_Msk, GPIO_MODER_MODER10_1 );		
		MODIFY_REG( GPIOB->MODER, GPIO_MODER_MODER11_Msk, GPIO_MODER_MODER11_1 );		
		SET_BIT( GPIOB->OTYPER, GPIO_OTYPER_OT10 );
		SET_BIT( GPIOB->OTYPER, GPIO_OTYPER_OT11 );
		MODIFY_REG( GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED10_Msk, GPIO_OSPEEDR_OSPEED10_0 );
		MODIFY_REG( GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEED11_Msk, GPIO_OSPEEDR_OSPEED11_0 );

		/* Alt Function: I2C */
		MODIFY_REG( GPIOB->AFR[1], GPIO_AFRH_AFSEL10_Msk, GPIO_AFRH_AFSEL10_2 );
		MODIFY_REG( GPIOB->AFR[1], GPIO_AFRH_AFSEL11_Msk, GPIO_AFRH_AFSEL11_2 );
	}
	else if( i2c_channel == I2C3 )
	{
		/* SCL = PA8; SDA = PC9 */
		MODIFY_REG( GPIOA->MODER, GPIO_MODER_MODER8_Msk, GPIO_MODER_MODER8_1 );		
		MODIFY_REG( GPIOC->MODER, GPIO_MODER_MODER9_Msk, GPIO_MODER_MODER9_1 );		
		SET_BIT( GPIOA->OTYPER, GPIO_OTYPER_OT8 );
		SET_BIT( GPIOC->OTYPER, GPIO_OTYPER_OT9 );
		MODIFY_REG( GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED8_Msk, GPIO_OSPEEDR_OSPEED8_0 );
		MODIFY_REG( GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED9_Msk, GPIO_OSPEEDR_OSPEED9_0 );

		/* Alt Function: I2C */
		MODIFY_REG( GPIOA->AFR[1], GPIO_AFRH_AFSEL8_Msk, GPIO_AFRH_AFSEL8_2 );
		MODIFY_REG( GPIOC->AFR[1], GPIO_AFRH_AFSEL9_Msk, GPIO_AFRH_AFSEL9_2 );
	}
	else
		return FAIL;

	return PASS;
}


static IRQn_Type get_i2c_event_irq( I2C_Channel channel )
{
	switch( channel )
	{
		case I2C_CH1: return I2C1_EV_IRQn;
		case I2C_CH2: return I2C2_EV_IRQn;
		case I2C_CH3: return I2C3_EV_IRQn;
		default: return I2C1_EV_IRQn;
	};
}

static IRQn_Type get_i2c_error_irq( I2C_Channel channel )
{
	switch( channel )
	{
		case I2C_CH1: return I2C1_ER_IRQn;
		case I2C_CH2: return I2C2_ER_IRQn;
		case I2C_CH3: return I2C3_ER_IRQn;
		default: return I2C1_ER_IRQn;
	};
}





#endif /* STM32 selection */