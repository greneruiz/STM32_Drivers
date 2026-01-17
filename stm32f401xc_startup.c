/** 
 * ===================================================================
 *  File Name: stm32f401xc_startup.c
 *  Type     : STM32 C-header
 *  Purpose  : Startup File for STM32F401XC
 *  Version  : 1.0
 * ===================================================================
 *  Version/Date : v1.0 / 2025-Sep-10 / G.RUIZ
 * 		* Initial release
 *  Version/Date : v1.1 / 2025-Dec-13 / G.RUIZ
 * 		* Fixed bug- missing macro for RTC_WKUP_Handler IRQ function
 * 		* Fixed bug- fixed word organization for the vector table
 * ===================================================================
*/

#include <stdint.h>

/* Externs for variables from stm32_linker.ld linker script */
extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sidata;

/* Functions required by stm32_linker.ld */
void __attribute__((used)) Reset_Handler(void);
int main(void);


/* Macro for interrupt handler function */
#define DEFAULT_HANDLER_FX( int_name )	\
	void int_name(void)__attribute__(( weak, alias( "Default_Handler" )));


/* Interrupt handler functions (See Table 38 of RM0368) */
DEFAULT_HANDLER_FX( NMI_Handler )
DEFAULT_HANDLER_FX( HardFault_Handler )
DEFAULT_HANDLER_FX( MemManage_Handler )
DEFAULT_HANDLER_FX( BusFault_Handler )
DEFAULT_HANDLER_FX( UsageFault_Handler )
DEFAULT_HANDLER_FX( SVCall_Handler )
DEFAULT_HANDLER_FX( DebugMonitor_Handler )
DEFAULT_HANDLER_FX( PendSV_Handler )
DEFAULT_HANDLER_FX( Systick_Handler )
DEFAULT_HANDLER_FX( WWDG_Handler )
DEFAULT_HANDLER_FX( PVD_Handler )
DEFAULT_HANDLER_FX( TAMP_STAMP_Handler )
DEFAULT_HANDLER_FX( RTC_WKUP_Handler )
DEFAULT_HANDLER_FX( FLASH_Handler )
DEFAULT_HANDLER_FX( RCC_Handler )
DEFAULT_HANDLER_FX( EXTI0_Handler )
DEFAULT_HANDLER_FX( EXTI1_Handler )
DEFAULT_HANDLER_FX( EXTI2_Handler )
DEFAULT_HANDLER_FX( EXTI3_Handler )
DEFAULT_HANDLER_FX( EXTI4_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream0_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream1_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream2_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream3_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream4_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream5_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream6_Handler )
DEFAULT_HANDLER_FX( ADC_Handler )
DEFAULT_HANDLER_FX( EXTI9_5_Handler )
DEFAULT_HANDLER_FX( TIM1_BRK_TIM9_Handler )
DEFAULT_HANDLER_FX( TIM1_UP_TIM10_Handler )
DEFAULT_HANDLER_FX( TIM1_TRG_COM_TIM11_Handler )
DEFAULT_HANDLER_FX( TIM1_CC_Handler )
DEFAULT_HANDLER_FX( TIM2_Handler )
DEFAULT_HANDLER_FX( TIM3_Handler )
DEFAULT_HANDLER_FX( TIM4_Handler )
DEFAULT_HANDLER_FX( I2C1_EV_Handler )
DEFAULT_HANDLER_FX( I2C1_ER_Handler )
DEFAULT_HANDLER_FX( I2C2_EV_Handler )
DEFAULT_HANDLER_FX( I2C2_ER_Handler )
DEFAULT_HANDLER_FX( SPI1_Handler )
DEFAULT_HANDLER_FX( SPI2_Handler )
DEFAULT_HANDLER_FX( USART1_Handler )
DEFAULT_HANDLER_FX( USART2_Handler )
DEFAULT_HANDLER_FX( EXTI15_10_Handler )
DEFAULT_HANDLER_FX( RTC_Alarm_Handler )
DEFAULT_HANDLER_FX( OTG_FS_WKUP_Handler )
DEFAULT_HANDLER_FX( DMA1_Stream7_Handler )
DEFAULT_HANDLER_FX( SDIO_Handler )
DEFAULT_HANDLER_FX( TIM5_Handler )
DEFAULT_HANDLER_FX( SPI3_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream0_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream1_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream2_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream3_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream4_Handler )
DEFAULT_HANDLER_FX( OTG_FS_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream5_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream6_Handler )
DEFAULT_HANDLER_FX( DMA2_Stream7_Handler )
DEFAULT_HANDLER_FX( USART6_Handler )
DEFAULT_HANDLER_FX( I2C3_EV_Handler )
DEFAULT_HANDLER_FX( I2C3_ER_Handler )
DEFAULT_HANDLER_FX( FPU_Handler )
DEFAULT_HANDLER_FX( SPI4_Handler )

/* Vector Table */
uint32_t vector_tbl[] __attribute__(( section(".isr_vector_tbl"))) =
{
	(uint32_t)&_estack,
	(uint32_t)&Reset_Handler,
	(uint32_t)&NMI_Handler,
	(uint32_t)&HardFault_Handler,
	(uint32_t)&MemManage_Handler,
	(uint32_t)&BusFault_Handler,
	(uint32_t)&UsageFault_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&SVCall_Handler,
	(uint32_t)&DebugMonitor_Handler,
	(uint32_t)0,
	(uint32_t)&PendSV_Handler,
	(uint32_t)&Systick_Handler,
	(uint32_t)&WWDG_Handler,
	(uint32_t)&PVD_Handler,
	(uint32_t)&TAMP_STAMP_Handler,
	(uint32_t)&RTC_WKUP_Handler,
	(uint32_t)&FLASH_Handler,
	(uint32_t)&RCC_Handler,
	(uint32_t)&EXTI0_Handler,
	(uint32_t)&EXTI1_Handler,
	(uint32_t)&EXTI2_Handler,
	(uint32_t)&EXTI3_Handler,
	(uint32_t)&EXTI4_Handler,
	(uint32_t)&DMA1_Stream0_Handler,
	(uint32_t)&DMA1_Stream1_Handler,
	(uint32_t)&DMA1_Stream2_Handler,
	(uint32_t)&DMA1_Stream3_Handler,
	(uint32_t)&DMA1_Stream4_Handler,
	(uint32_t)&DMA1_Stream5_Handler,
	(uint32_t)&DMA1_Stream6_Handler,
	(uint32_t)&ADC_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&EXTI9_5_Handler,
	(uint32_t)&TIM1_BRK_TIM9_Handler,
	(uint32_t)&TIM1_UP_TIM10_Handler,
	(uint32_t)&TIM1_TRG_COM_TIM11_Handler,
	(uint32_t)&TIM1_CC_Handler,
	(uint32_t)&TIM2_Handler,
	(uint32_t)&TIM3_Handler,
	(uint32_t)&TIM4_Handler,
	(uint32_t)&I2C1_EV_Handler,
	(uint32_t)&I2C1_ER_Handler,
	(uint32_t)&I2C2_EV_Handler,
	(uint32_t)&I2C2_ER_Handler,
	(uint32_t)&SPI1_Handler,
	(uint32_t)&SPI2_Handler,
	(uint32_t)&USART1_Handler,
	(uint32_t)&USART2_Handler,
	(uint32_t)0,
	(uint32_t)&EXTI15_10_Handler,
	(uint32_t)&RTC_Alarm_Handler,
	(uint32_t)&OTG_FS_WKUP_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&DMA1_Stream7_Handler,
	(uint32_t)0,
	(uint32_t)&SDIO_Handler,
	(uint32_t)&TIM5_Handler,
	(uint32_t)&SPI3_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&DMA2_Stream0_Handler,
	(uint32_t)&DMA2_Stream1_Handler,
	(uint32_t)&DMA2_Stream2_Handler,
	(uint32_t)&DMA2_Stream3_Handler,
	(uint32_t)&DMA2_Stream4_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&OTG_FS_Handler,
	(uint32_t)&DMA2_Stream5_Handler,
	(uint32_t)&DMA2_Stream6_Handler,
	(uint32_t)&DMA2_Stream7_Handler,
	(uint32_t)&USART6_Handler,
	(uint32_t)&I2C3_EV_Handler,
	(uint32_t)&I2C3_ER_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&FPU_Handler,
	(uint32_t)0,
	(uint32_t)0,
	(uint32_t)&SPI4_Handler
};


void Default_Handler( void )
{
	while(1){}
}

void __attribute__((used)) Reset_Handler( void )
{
	/// Calculate .data and .bss section sizes
	uint32_t data_mem_size = (uint32_t)&_edata - (uint32_t)&_sdata;
	uint32_t bss_mem_size = (uint32_t)&_ebss - (uint32_t)&_sbss;

	/// Copy .data from FLASH to SRAM
//	uint32_t *p_src_mem = (uint32_t *)&_etext;
	uint32_t *p_src_mem = (uint32_t *)&_sidata;
	uint32_t *p_dest_mem = (uint32_t *)&_sdata;
	for( uint32_t i = 0; i < data_mem_size; i++ )
	{
		*p_dest_mem++ = *p_src_mem++;
	}

	/// Reset .bss to 0
	p_dest_mem = (uint32_t *)&_sbss;
	for( uint32_t i = 0; i < bss_mem_size; i++ )
	{
		*p_dest_mem++ = 0;
	}

	/// Call the main()
	main();
}