/*
 Pins used in this application:
 
 P0.06 [X] - ADC1: Input for the distance sensor
 P0.14 [O] - GPO: SPI0 isr indicator (active high)
 P0.19 [O] - CLKOUT: main/1
 P0.20 [X] - SCT_IN0/SPI0_SCK: reference clock
 P0.21 [X] - SCT_IN1/SPI0_MOSI: SPI data
 P0.22 [O] - SCT_OUT0: WS2812 data
 P0.23 [O] - SCT_OUT1: active transfer indicator (high)
 
 To run this demo hook the DATA/DIN pin on the Neopixels to
 P0.22 (A3 on the LPC824MAX Arduino header), and power the
 NeoPixels with the 5V output on the board.

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "LPC8xx.h"
#include "lpc8xx_syscon.h"
#include "lpc8xx_swm.h"
#include "lpc8xx_adc.h"

#include "sct_lpc82x_addon.h"
#include "sct_generic_addon.h"

volatile uint32_t main_loop_counter, i, j;
volatile uint32_t user_gate_pattern, user_gate_counter;
volatile uint32_t dummy_32b;

#define	TSAMPLE	1				// input data sampling point; frequency independent
#define	T0H		5				//  5/12 MHz = 0.417 us @ 12 MHz (spec: 0.35 us); 10 for 24 MHz setup
#define	T0L		10				// 10/12 MHz = 0.833 us @ 12 MHz (spec: 0.80 us); 20 for 24 MHz setup
#define	T1H		8				//  8/12 MHz = 0.667 us @ 12 MHz (spec: 0.70 us); 16 for 24 MHz setup
#define	T1L		7				//  7/12 MHz = 0.583 us @ 12 MHz (spec: 0.60 us); 14 for 24 MHz setup
#define	TDONE	(12*(50+10))	// 50+10 us @ 12 MHz delay for the done event

enum app_sct_in		{app_in_clock = 0, app_in_data};
enum app_sct_out	{app_out_data = 0, app_out_active};
enum app_sct_match	{app_m_data_sample = 0, app_m_out0, app_m_out1, app_m_done};
enum app_sct_state	{app_st_0 = 0, app_st_1};
enum app_sct_event	{app_ev_in_clock_re = 0, app_ev_switch1, app_ev_out0, app_ev_out1, app_ev_done};

#define	APP_ALL_STATES	(1<<app_st_0 | 1<<app_st_1)

#define	SPI0_ISR_PORT	(LPC_GPIO_PORT)		//SPI ISR activity port pin...
#define	SPI0_ISR_PIN	(14)				//...

#define	WS2812_LED_CNT	(16)
#define WS2812_LED_CLRS	(4)					// 3 = RGB, 4 = RGBW

#define DEBOUNCE_PRE	(32)				// The number of samples to average before debouncing
#define DEBOUNCE_DEV    (0.5f)				// +/- 0.5cm
#define DEBOUNCE_DELAY	(2400000)			// 200ms delay (in ticks) between debounce reads

#if (WS2812_LED_CLRS != 3) && (WS2812_LED_CLRS != 4)
#error "WS2812_LED_CLRS must be either 3 or 4"
#endif

volatile struct _led_data { uint8_t green;
							uint8_t red;
							uint8_t blue;
#if (WS2812_LED_CLRS == 4)
							uint8_t white;
#endif
} ws2812_data[WS2812_LED_CNT+1];
volatile uint32_t ws2812_tx_cnt, ws2812_tx_index;
volatile uint8_t * pnt_8bit_data;

volatile float debounce_last;

uint32_t init_mcu(uint32_t _setup);
uint32_t init_sct(uint32_t _setup);
uint32_t init_adc(uint32_t _setup);
uint16_t read_adc(uint8_t adc_ch);
uint32_t start_transfer(uint32_t _wait_for_done);
uint32_t delay_systick(uint32_t _delay_count);
int		 debounce_dist(float dist_cm);

int main(void)
{
	float adcmv;
	float cm;
	float avg_cm;
	int   samp_valid;

	main_loop_counter = 0;
	
	init_mcu(0);
	init_sct(0);
	init_adc(0);
	
	// Clear all LEDs
	for (i = 0; i != WS2812_LED_CNT; i++)
	{
		ws2812_data[i].red = 0;
		ws2812_data[i].green = 0;
		ws2812_data[i].blue = 0;
#if (WS2812_LED_CLRS == 4)
		ws2812_data[i].white = 0;
#endif
	}
	
	while(1)
	{
		// Take several ADC samples to average out noise
		avg_cm=0;
		for (i=0; i<DEBOUNCE_PRE; i++)
		{
			// Convert raw ADC value to cm (measured value >=20cm)
			adcmv = (float)(read_adc(1)) * (3300.0F/4096.0F);
			avg_cm += 61.573F * pow(adcmv/1000.0F, -1.1068F);
			delay_systick(100);
		}

		// Get simple average
		cm=avg_cm/DEBOUNCE_PRE;

		// Run the sample through the debounce filter
		samp_valid = debounce_dist(cm);
		while (!samp_valid) {
			delay_systick(DEBOUNCE_DELAY);
		}

		// Light up the number of LEDs matching the distance in 0.5 cm steps
		for (i = 0; i != WS2812_LED_CNT; i++)
		{
			// Object detected, but distance OK (green)
			if (cm > 24)
			{
					if (cm*2 >= i+39) {
						ws2812_data[i].red 		= 0x00;
						ws2812_data[i].green 	= 0x00;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
					else {
						ws2812_data[i].red 		= 0x00;
						ws2812_data[i].green 	= 0x1F;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
			}

			// Distance warning (amber)
			else if (cm <= 24 && cm > 22 )
			{
					if (cm*2 >= i+39) {
						ws2812_data[i].red 		= 0x00;
						ws2812_data[i].green 	= 0x00;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
					else {
						ws2812_data[i].red 		= 0x2F;
						ws2812_data[i].green 	= 0x0F;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
			}

			// Object too close (red)
			else {
					if (cm*2 >= i+39) {
						ws2812_data[i].red 		= 0x00;
						ws2812_data[i].green 	= 0x00;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
					else {
						ws2812_data[i].red 		= 0x3F;
						ws2812_data[i].green 	= 0x00;
						ws2812_data[i].blue		= 0x00;
#if (WS2812_LED_CLRS == 4)
						ws2812_data[i].white	= 0x00;
#endif
					}
			}
			start_transfer(0);
			delay_systick(2*100*100);
		}
		
		main_loop_counter++;
	}
	
	return 0;
}

uint32_t init_mcu(uint32_t setup)
{
	uint32_t result;
	
	// CLKOUT setup begin
	// CLKOUT = main/1 @ P0.19
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<7;			// Enable SWM clock
	LPC_SYSCON->CLKOUTDIV = 1;					// Prepare CLKOUT/1
	LPC_SYSCON->CLKOUTSEL = 3;					// CLKOUT =  main clock
	LPC_SYSCON->CLKOUTUEN = 0;					// Update CLKOUT...
	LPC_SYSCON->CLKOUTUEN = 1;					// ... selection
	// CLKOUT setup end
	
	// SPI setup begin
	NVIC_DisableIRQ(SPI0_IRQn);					// Disable SPI0 interrupts
	NVIC_SetPriority(SPI0_IRQn, 0);				// Make sure this is the top priority ISR
	do											// Clear pending SPI0 interrupt...
	{											//...
		NVIC_ClearPendingIRQ(SPI0_IRQn);		//...
	}											//...
	while(NVIC_GetPendingIRQ(SPI0_IRQn) != 0);	//...
	
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<11;			// Enable SPI0 clock
	LPC_SYSCON->PRESETCTRL &= ~(1<<0);			// Reset SPI0
	LPC_SYSCON->PRESETCTRL |= 1<<0;				// Release SPI0 reset
	
	LPC_SWM->PINASSIGN3 = (LPC_SWM->PINASSIGN3 & ~(0xFF<<24)) | ((0*32+20)<<24);	//P0.20 = SCK0
	LPC_SWM->PINASSIGN4 = (LPC_SWM->PINASSIGN4 & ~(0xFF<< 0)) | ((0*32+21)<< 0);	//P0.21 = MOSI0
	
	LPC_SPI0->DIV = 15-1;						// SPI bitrate = system clock/15 (see WS2812 timing)
	LPC_SPI0->DLY = 0;							// No added delays
	LPC_SPI0->INTENCLR = 0x3F;					// Disable all interrupts
	LPC_SPI0->TXCTL = (8-1)<<24 | 1<<22; 		// 8 bit frame,ignore RX data
	LPC_SPI0->CFG = 0<<8 | 0<<7 | 1<<5 | 1<<4 | 0<<3 | 1<<2 | 1<<0;	//SSEL=0,no loop,CPOL=1,CPHA=1,MSB,master,enable
	NVIC_EnableIRQ(SPI0_IRQn);					// Enable SPI0 interrupts
	
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<6;			// Enable access to GPIOs
	SPI0_ISR_PORT->DIRSET0 = 1<<SPI0_ISR_PIN;	// SPI0 isr indicator...
	SPI0_ISR_PORT->CLR0 = 1<<SPI0_ISR_PIN;		// ... output low
	
	pnt_8bit_data = (uint8_t *)&ws2812_data[0];	// Prepare the 8-bit data pointer
	ws2812_tx_cnt = WS2812_LED_CLRS*WS2812_LED_CNT;	// Calculate number of 8-bit transfers

	// SPI setup end
	
	// SysTick setup begin
	SysTick->CTRL = 1<<2 | 0<<1 | 0<<0;			// Processor clock,no int,disabled
	SysTick->LOAD = 0xFFFFFF;					// Max range
	SysTick->CTRL = 1<<2 | 0<<1 | 1<<0;			// Processor clock,no int,enabled
	// SysTick setup end
	
	result = 0;
	
	return result;
}

uint32_t init_sct(uint32_t setup)
{
	uint32_t result;
	
	// SCT setup begin
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<8;			// Enable SCT clock...
	LPC_SYSCON->PRESETCTRL &= ~(1<<8);			// ... reset...
	LPC_SYSCON->PRESETCTRL |= 1<<8;				// ... release SCT
	
	LPC_SWM->PINASSIGN6 = (LPC_SWM->PINASSIGN6 & ~(0xFF<<24)) | ((0*32+20)<<24);	// CTIN_0 @ P0.20 (input clock)
	LPC_INPUTMUX->SCT0_INMUX0 = 0;													// SCT0_PIN0 = SWM
	
	LPC_SWM->PINASSIGN7 = (LPC_SWM->PINASSIGN7 & ~(0xFF<< 0)) | ((0*32+21)<< 0);	// CTIN_1 @ P0.21 (input data)
	LPC_INPUTMUX->SCT0_INMUX1 = 1;													// SCT0_PIN1 = SWM
	
	LPC_SWM->PINASSIGN7 = (LPC_SWM->PINASSIGN7 & ~(0xFF<<24)) | ((0*32+22)<<24);	// CTOUT_0 @ P0.22 (output data)
	LPC_SWM->PINASSIGN8 = (LPC_SWM->PINASSIGN8 & ~(0xFF<<0 )) | ((0*32+23)<<0);		// CTOUT_1 @ P0.23 (activity indicator data)
	
	LPC_SCT0->CONFIG =  0<<18 |					// NA
						0<<17 |					// No autolimit U
						(1<<app_in_clock  |		// Synchronize inputs...
						1<<app_in_data)<<9 |	// ...
						1<<8 |					// NA
						1<<7 |					// Do not reload U match registers
						0<<1 |					// System clock drives the SCT
						1<<0;					// 1x 32-bit counter
	LPC_SCT0->CTRL =  (1-1)<<5 | 1<<2;			// U: no prescaler, halt
	LPC_SCT0->EVEN = 0;							// Disable all interrupts
	LPC_SCT0->EVFLAG = 0xFFFFFFFF;				// Clear all flags
	
	// Match register setup
	// ====================
	LPC_SCT0->REGMODE_L &= ~(1<<app_m_data_sample);		// Sampling point match value
	LPC_SCT0->MATCH[app_m_data_sample].L = TSAMPLE-1;	// ...
	LPC_SCT0->REGMODE_L &= ~(1<<app_m_out0);			// Out 0 match value
	LPC_SCT0->MATCH[app_m_out0].L = T0H-1;				// ...
	LPC_SCT0->REGMODE_L &= ~(1<<app_m_out1);			// Out 1 match value
	LPC_SCT0->MATCH[app_m_out1].L = T1H-1;				// ...
	LPC_SCT0->REGMODE_L &= ~(1<<app_m_done);			// Sequence done match value
	LPC_SCT0->MATCH[app_m_done].L = TDONE-1;			// ...

	// Events setup
	// ============

	//    setup for event: app_ev_in_clock_fe
	//   enabled in state: all states
	//       generated by: RISING EDGE(app_in_clock)
	//      state machine: go to app_st_0 state
	//       misc control: limiting, start event
	//     output control: drive app_out_data, app_out_active high
	LPC_SCT0->EVENT[app_ev_in_clock_re].STATE = APP_ALL_STATES;
	LPC_SCT0->EVENT[app_ev_in_clock_re].CTRL = EVCTR_SCT_U | EVCTR_IN_RISE(app_in_clock) | EVCTR_STATE_SET(app_st_0);
	LPC_SCT0->LIMIT |= 1<<app_ev_in_clock_re;
	LPC_SCT0->START |= 1<<app_ev_in_clock_re;
	LPC_SCT0->OUT[app_out_data].SET |= 1<<app_ev_in_clock_re;
	LPC_SCT0->OUT[app_out_active].SET |= 1<<app_ev_in_clock_re;
	
	//    setup for event: app_ev_switch1
	//   enabled in state: app_st_0
	//       generated by: MATCH(app_m_data_sample) AND HIGH(app_in_data)
	//      state machine: go to app_st_1 state
	LPC_SCT0->EVENT[app_ev_switch1].STATE = 1<<app_st_0;
	LPC_SCT0->EVENT[app_ev_switch1].CTRL = EVCTR_SCT_U | EVCTR_MATCH_AND_IN_HIGH(app_m_data_sample,app_in_data) | EVCTR_STATE_SET(app_st_1);
	
	//    setup for event: app_ev_out0
	//   enabled in state: app_st_0
	//       generated by: MATCH(app_m_out0)
	//      state machine: do not change state
	//     output control: drive app_out_data low
	LPC_SCT0->EVENT[app_ev_out0].STATE = 1<<app_st_0;
	LPC_SCT0->EVENT[app_ev_out0].CTRL = EVCTR_SCT_U | EVCTR_MATCH(app_m_out0) | EVCTR_STATE_NO_CHANGE;
	LPC_SCT0->OUT[app_out_data].CLR |= 1<<app_ev_out0;
	
	//    setup for event: app_ev_out1
	//   enabled in state: app_st_1
	//       generated by: MATCH(app_m_out1)
	//      state machine: do not change state
	//     output control: drive app_out_data low
	LPC_SCT0->EVENT[app_ev_out1].STATE = 1<<app_st_1;
	LPC_SCT0->EVENT[app_ev_out1].CTRL = EVCTR_SCT_U | EVCTR_MATCH(app_m_out1) | EVCTR_STATE_NO_CHANGE;
	LPC_SCT0->OUT[app_out_data].CLR |= 1<<app_ev_out1;
	
	//    setup for event: app_ev_done
	//   enabled in state: all states
	//       generated by: MATCH(app_m_done)
	//      state machine: do not change state
	//       misc control: stop event
	//     output control: drive app_out_active low
	LPC_SCT0->EVENT[app_ev_done].STATE = APP_ALL_STATES;
	LPC_SCT0->EVENT[app_ev_done].CTRL = EVCTR_SCT_U | EVCTR_MATCH(app_m_done) | EVCTR_STATE_NO_CHANGE;
	LPC_SCT0->STOP |= 1<<app_ev_done;
	LPC_SCT0->OUT[app_out_active].CLR |= 1<<app_ev_done;

	LPC_SCT0->EVFLAG = (1<<CONFIG_SCT0_nEV)-1;	// Clear all event flags
	
	LPC_SCT0->OUTPUT =  0<<app_out_data |
						0<<app_out_active;		// Initial output: data, active indicator low
	
	LPC_SCT0->STATE = app_st_0; 				// Set initial state
	LPC_SCT0->COUNT = 0;						// Prime the counter

	LPC_SCT0->CTRL = (LPC_SCT0->CTRL & ~(1<<2)) | 1<<1;	// HALT->STOP
	
	// SCT setup end
	
	result = 0;
	
	return result;
}

uint32_t init_adc(uint32_t setup)
{
	uint32_t result = 0;

	// This function configures ADC1 (P0_6) on the LPC824,
	// which corresponds to pin A0 on the Arduino headers.
	// Configure the SWM (see utilities_lib and lpc8xx_swm.h)
	LPC_SWM->PINENABLE0 &= ~(ADC_1);		// Enable ADC on P0_6
	LPC_IOCON->PIO0_6 = 1<<7;	 			// No pull-up/down

	// ADC initialization
	LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);		// Power up the ADC
	LPC_SYSCON->SYSAHBCLKCTRL |= (ADC);		// Enable ADC clock
	LPC_SYSCON->PRESETCTRL &= (ADC_RST_N);	// Reset the ADC
	LPC_SYSCON->PRESETCTRL |= ~(ADC_RST_N);

	// Start the self-calibration
	LPC_ADC->CTRL = 1     << ADC_CALMODE  |	// Calibration mode
			        0     << ADC_LPWRMODE | // Not low power mode
					(1-1) << ADC_CLKDIV;	// @ 12MHz CLKDIV = 1, ADC=500kHz

	// Poll the calibration mode bit until it is cleared
	while (LPC_ADC->CTRL & (1<<ADC_CALMODE));

	// Configure the ADC for the appropriate analog supply voltage using the TRM register
	// For a sampling rate higher than 1 Msamples/s, VDDA must be higher than 2.7 V (on the Max board it is 3.3 V)
	LPC_ADC->TRM &= ~(1<<ADC_VRANGE); // '0' for high voltage

	// Exit calibration mode
	LPC_ADC->CTRL = 0     << ADC_CALMODE  |	// Disable calibration mode
			        0     << ADC_LPWRMODE |	// Not low power mode
					(1-1) << ADC_CLKDIV;	// & 12MHz CLKDIV = 1, ADC=~500kHz

	// Write the sequence control word with enable bit set for both sequences
	LPC_ADC->SEQA_CTRL = 0x00;
	LPC_ADC->SEQB_CTRL = 0x00;

	return result;
}

uint16_t read_adc(uint8_t adc_ch)
{
	uint32_t sample;
	uint16_t sample_x;

	LPC_ADC->SEQA_CTRL = 1UL << ADC_SEQ_ENA |	// Enable sequence
						 0   << ADC_BURST   |   // Single read
						 0   << ADC_START   |   // Do not start
						 1   << ADC_TRIGPOL |   // Trigger pos edge
						 0   << ADC_TRIGGER |   // SW trigger
						 1   << adc_ch;         // Select channel

	// Clear DATAVALID before the next sample
	do
	{
	   sample = LPC_ADC->SEQA_GDAT;
	}
	while((sample & (1UL<<31)) != 0);

	LPC_ADC->SEQA_CTRL = 1UL << ADC_SEQ_ENA | 	// Enable sequence
						 0   << ADC_BURST   | 	// Single read
						 1   << ADC_START   | 	// Start
						 1   << ADC_TRIGPOL | 	// Trigger pos edge
						 0   << ADC_TRIGGER | 	// SW trigger
						 1   << adc_ch;       	// Select channel

	// Wait for DATAVALID = 1
	do
	{
	   sample = LPC_ADC->SEQA_GDAT;
	}
	while((sample & (1UL<<31)) == 0);

	sample_x = (uint16_t)((sample & 0xFFF0) >> 4) ;

	return (uint16_t)((sample & 0xFFF0) >> 4) ;
}

// Debounce the distance results to remove noise
// Returns 0 if the sample is outside DEBOUNCE_DEV
// or 1 if it's within range (meaning a valid sample)
int debounce_dist(float dist_cm)
{
	int results = 0;

	// Check if the new sample is within DEBOUNCE_DEV
	// of the last valid sample
	if ((dist_cm >= debounce_last - DEBOUNCE_DEV) ||
		(dist_cm <= debounce_last + DEBOUNCE_DEV)) {
		results = 1;
	} else {
		results = 0;
	}

	debounce_last = dist_cm;

	return results;
}

uint32_t start_transfer(uint32_t wait_for_done)
{
	uint32_t result;
	
	ws2812_tx_index = 0;									// Clear the tx data index
	if (wait_for_done != 0)									// Wait for the done flag if needed...
	{														//...
		while((LPC_SCT->EVFLAG & (1<<app_ev_done)) == 0);	//...
	}														//...
	LPC_SCT->EVFLAG = 1<<app_ev_done;						// Clear the done flag
	LPC_SPI0->INTENSET = 1<<1;								// Enable TXDAT based interrupt
	
	result = 0;
	
	return result;
}

/* SysTick based delay routine */
uint32_t delay_systick(uint32_t delay_count)
{
	int result;
	
	delay_count &= 0xFFFFFF;
	if (delay_count < 1000)
	{
		delay_count = 1000;
	}
	
	SysTick->LOAD = delay_count-1;
	SysTick->VAL = 0x123456;
	result = SysTick->VAL;
	
	while((SysTick->CTRL & (1<<16)) == 0);
	
	result = 0;
	
	return result;
}

void SPI0_IRQHandler(void)
{
	SPI0_ISR_PORT->SET0 = 1<<SPI0_ISR_PIN;
	
	LPC_SPI0->TXDAT = *(pnt_8bit_data+ws2812_tx_index);	// Send next data
	ws2812_tx_index++;									// Update the index
	if (ws2812_tx_index == ws2812_tx_cnt)
	{
		LPC_SPI0->INTENCLR = 1<<1;						// In case no more data left disable TXDAT interrupt
	}

	SPI0_ISR_PORT->CLR0 = 1<<SPI0_ISR_PIN;
	
	return;
}
