# define F_CPU 16000000UL
#include <avr/io.h>
#include "util/delay.h"
#include <avr/interrupt.h>
#include "uart.h"


unsigned int diff, edge1, edge2, overflows;
unsigned long pulse_width;

	

ISR(TIMER1_CAPT_vect){
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
	TIFR1 |= 0x20; // clear the input flagt 	 
	sei(); // enable all interrupts.
	for(;;);
}
