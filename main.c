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
uint32_t counter = 0;
uint32_t decay = 0;

void adc_init(void) {
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
	sei();    
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
/*
	cli();	
	int i, j = 0;
	for (i = 0; i < 0x8FFF; i++)
		j++;
*/
	uint8_t tmp = ADCH;
	samp = tmp;
	samp = samp > 0 ? samp * FIXED_1 : 0;

	//if (!(counter % 0x4000)) printf("samp: %ld\n",(samp >> FSHIFT));
	avg_energy = calc_avg(avg_energy, EXP_A, samp);
	inst_energy = calc_avg(inst_energy, EXP_I, samp);
	if ((inst_energy/avg_energy) > 8 && decay <= 0) {
		PORTB ^= (1 << PIN5); /* flip pin5 for visual*/
		decay = 500;
	}

	if (decay > 0)
		decay --;
	//sei();    
}

int main(void) {    
	adc_init();
	uart_init();
	stdout = &uart_output;
	stdin  = &uart_input;

	DDRB |= (1 << PIN5); /* (set up led for output) */

	int foo, bar;
	while (1) {
		//if (!decay) printf("decay done\n");
		//printf("%d %d\n", (int)(inst_energy >> FSHIFT), (int)(avg_energy >> FSHIFT));
		//_delay_ms(50);
	}

	return 0;

}
