# define F_CPU 16000000UL
#include <avr/io.h>
#include "util/delay.h"
#include <avr/interrupt.h>
#include "uart.h"


unsigned int diff, edge1, edge2, overflows;
unsigned long pulse_width;

	

ISR(TIMER1_CAPT_vect){
	// if(PINB & 0x01){
	// 	PORTB ^= 0x20;
	// }
	TCCR1B ^= 0x40;
	PORTB ^= 0x20;

}



int main(void)
{

	DDRB |= 0x20; //This turns the PORTB fifth bit to output.
	uart_init();
	TCCR1B |= 0x01; //enable timer
	TCCR1B |= 0x40; //capture rising edge
	TIMSK1 |= 0x20; //enable interrupt
	TIFR1 |= 0x20; // clear the input flag
	sei(); // enable all interrupts.
	// while (!(TIFR1 & 0x20));
	
	// edge1 = ICR1;
	// TCCR1B &= 0xBF;
	// TIFR1 |= 0x20;
	
	// while(!(TIFR1 & 0x20));
	// edge2 = ICR1;
	

	
	//printf("pulse width = %int\n", edge1);
	for(;;);
}
