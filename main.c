#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include "main.h"
#include "uart.h"

#define FSHIFT 12
#define FIXED_1   (1<<FSHIFT) /* 1.0 as fixed-point */
#define EXP_I     10
#define EXP_A     0xfff

uint32_t inst_energy = 0;
uint32_t avg_energy = 0;
uint32_t avg_time = 0;
uint32_t avg_err = 0;
uint32_t counter = 0;
uint32_t decay = 0;
uint32_t jiffies = 0;
uint32_t prev_jiffies = 0;


void adc_init(void)
{
 /* Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz */
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADMUX |= (1 << REFS0); /* Set ADC reference to AVCC */
	/* Left adjust ADC result to allow easy 8 bit reading */
	ADMUX |= (1 << ADLAR);
	// No MUX values needed to be changed to use ADC0
	ADCSRA |= (1 << ADATE);  // Set ADC to Free-Running Mode
	ADCSRA |= (1 << ADEN);  // Enable ADC
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	ADCSRA |= (1 << ADIE);//enable interrupts
}

uint32_t
calc_avg(uint32_t avg, uint32_t exp, uint32_t samp)
{
	avg *= exp;
	avg += samp * (FIXED_1 - exp);
	avg += 1UL << (FSHIFT - 1);
	return avg >> FSHIFT;
}

ISR(ADC_vect)
{
	counter++;
	uint32_t samp;
	uint32_t diff;
	
/*
	cli();	
	int i, j = 0;
	for (i = 0; i < 0x8FFF; i++)
		j++;
*/
	samp = ADCH;
	samp = samp > 0 ? samp * FIXED_1 : 0;

	avg_energy = calc_avg(avg_energy, EXP_A, samp);
	inst_energy = calc_avg(inst_energy, EXP_I, samp);
	if ((inst_energy/avg_energy) > 4 && decay <= 0) {
		diff = jiffies - prev_jiffies;
		prev_jiffies = jiffies;
		avg_time = calc_avg(avg_time, 0x400, diff);
		avg_err = calc_avg(avg_err, 0x400, abs(avg_time - diff));
		printf("beat: %u %u %u\n", (int)diff, (int)avg_time,
			(int)avg_err);
		//PORTB |= (1 << PIN5); /* turn on led */
		decay = 1500;
	}

	if (decay > 0)
		decay --;
	//if (!decay)
	//	PORTB &= ~(1 << PIN5); /* turn off led */
		
	//sei();    
}
uint8_t state = 0;
uint8_t dark = 0;
ISR(TIMER0_OVF_vect)
{
	jiffies++;
	dark++;
	uint8_t level = (uint8_t) avg_err;
	if(avg_err<300) level = 99;
	else level = 0;

	
	if (dark > level) dark = 0;
	PORTB ^= 1 << PIN5;
	if (!dark )
		PORTB |= (1 << PIN5); /* turn on led */
	else
		PORTB &= ~(1 << PIN5); /* turn off led */
}
int main(void) {    
	adc_init();
	uart_init();
	stdout = &uart_output;
	stdin  = &uart_input;

	DDRB |= (1 << PIN5); /* (set up led for output) */

	TCCR0A = 0;
	//TCCR0B |= ((1<<CS02));
	//TCCR0B |= ((1<<CS01));
	TCCR0B |= ((1<<CS00));
	TIMSK0 |= (1<<TOIE0);
	sei();    
	while (1) {
//	//	printf("%d %d\n", (int)(inst_energy >> FSHIFT), (int)(avg_energy >> FSHIFT));
//		printf("%u\n", jiffies);
		//_delay_ms(~avg_err);
	}

	return 0;

}
