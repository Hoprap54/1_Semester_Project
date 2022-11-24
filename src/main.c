#define F_CPU 16000000UL //needs to be defined for the delay functions to work.
#define BAUD 9600
#define NUMBER_STRING 1001

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h> //here the delay functions are found
#include "usart.h"

volatile unsigned long int timer = 0, counter = 0; 
volatile char car_move_flag = 0;
char acceleration_index(double, double);
char acceleration_flag = 0;
double seconds, speed = 0, prev_speed = 0, eigthcircumference = 0.02589182;

void initialize(void);

ISR(TIMER1_CAPT_vect){
    timer=ICR1+65535*counter;
    //printf("Input Capture EVENT!!!");
    TCNT1=0;
    TIFR1|=1<<ICF1;//reseting the input capture flag
    counter=0;
    car_move_flag=1;
    
}
ISR(TIMER1_OVF_vect){
    counter++;
    TCNT1=0;
    if(counter>2)
    car_move_flag=0;
}
/* Declare function */
int main(void) {    

    uart_init(); // open the communication to the microcontroller
	io_redirect(); // redirect input and output to the communication

    initialize();
    speed=0;
    prev_speed=0;
    acceleration_flag=0;
    car_move_flag=0;
    printf("%lf", speed);
        while(1){
            
            seconds=((double)timer*1000)/15625000;
            //speed calculation
            if (seconds)
            speed = eigthcircumference/seconds;
            
            if (car_move_flag==1){
                printf("\nCar is moving");
            }
            if (car_move_flag==0){
                printf("\nCar is not moving.");
                speed=0;
                timer=0;
                seconds=0;
                prev_speed=0;
            }

            acceleration_flag= acceleration_index(speed, prev_speed);
            printf("\n This is the current state of the the timer:%lu and seconds:%lf - speed:%lf - accelerationflag:%d",timer,seconds, speed, acceleration_flag);
            _delay_ms(1000);
           

            printf("\n prev-speed:%f",prev_speed);
            prev_speed=speed;
                                           
        }
            
    return 0;
}

/* Function description */

void run_Motor(){
    int count;
    DDRD = 0x60;       // Set Port D as output for the LEDs 0b0010 0000
    TCCR0A |= 0xA2;    // Fast PWM //mode with clear OC0A on compare match, set at bottom. Output B similar. //Page 84 and 86​
    TCCR0B |= 0x05;    // 1024 frequency scaling​
    TCNT0 = 0x0000;
    count += 1;
    if (count > OCR0A){
        PORTD = 0x60;
        OCR0A = 1;  // PWM TO 5v 
        count = 0;
    }else{
        PORTD = 0x00;
        OCR0A = 0;  // PWM TO 0v 
    }
}



void initialize(void){
    sei();//enable global interrupts
    
    TIMSK1|=(1<<ICIE1)|(1<<TOIE1);//timer interrupts must be enabled
    TCCR1A = 0x00;
    TCCR1B = (1<<ICNC1)|/*(1<<ICES1)|*/(1<<CS12)|(1<<CS10);//noise cancel-/*falling*/ raising edge - 1024 prescaling
    DDRB &= ~0x01;
    PORTB |= 0x01;
    TIFR1|=1<<ICF1;//reseting input capture flag
    
    
}


char acceleration_index(double current_speed, double previous_speed){

    char acceleration_flag;
    if(current_speed==0&&previous_speed==0)
    acceleration_flag=0;
    else
    if(current_speed<prev_speed)
    acceleration_flag=1;
    if(current_speed>prev_speed)
    acceleration_flag=2;
    if(speed==0)
    acceleration_flag=0;
    printf("debug");
    

    return acceleration_flag;

}


