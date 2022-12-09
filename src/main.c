#define F_CPU 16000000UL //needs to be defined for the delay functions to work.
#define BAUD 9600
#define NUMBER_STRING 1001
//#define CIRCUMFERENCE 0.02589182

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h> //here the delay functions are found
#include "usart.h"
#include "subfunctions.h"
#include <stdbool.h>

// Variables for the interrupts
volatile unsigned long int timer = 0, counter = 0;                        // Timer: variable for the time; counter: counter to count timeroverflows
volatile bool car_move_flag = false, stringbeginflag=false;               // Variable to indicate whether the car is moving
volatile int i, distancecounter = 0, test = 3, readBufferindex = 0, buffersize;     // For for loop in interrupt
volatile char readBuffer[100]= {0}, rxexpect=0x71;
volatile double seconds, secondstogo, speed = 0, neededspeed=0, prev_speed = 0, eigthcircumference = 0.02589182, distance = 0, distancetogo;

// Variables for the functions

char acceleration_flag = 0;                 // Variable for indicating the state of aceleration
char savereadBuffer[100]= {0};
int currentpagenumber = 0, stagesexpexted = 0, stagenumber = 0, stages_driven = 0, ocr0asetter=100;
uint32_t setspeed=0;
bool displayreadsuccess = false;


typedef struct{

    double stagetime;
    double stagedistance;
    double stagespeed;

}rallystage_t;

volatile rallystage_t rallystages[10];
char acceleration_index(double, double);    // Function for checking the state of acceleration
/* Declare functions */
void initialize(void);      //Function for initializing the timer and interrupts
char displaysave(void);
void getpage(void);
void updatedata(void);
void PWM_Motor(unsigned char);
unsigned int read_adc(void);
void cardriver(int);


//interrupts
ISR(USART_RX_vect){

    scanf("%c", &readBuffer[readBufferindex]);

    if(stringbeginflag==false){                    //making sure that only first indicator gets detected
        if(readBuffer[readBufferindex]==rxexpect){//saving first element of string at index 0
        
        readBufferindex=0;
        readBuffer[0]=rxexpect;
        stringbeginflag=true;

        }
    }

    readBufferindex++;
    switch(rxexpect){

        case 0x71:
        buffersize=8;
        break;
        case 0x65:
        buffersize=7;
        break;
        case 0x66:
        buffersize=5;

    }
    if(readBufferindex == buffersize){//maybe adding expected_byte_count - buffersize
    readBufferindex = 0;
    stringbeginflag = false;
    /*UCSR0B &= ~(1<<RXEN0);
    UCSR0B |= 1<<RXEN0;*/
    }
}

ISR(TIMER1_CAPT_vect){
    timer=ICR1+65535*counter;// Updating timer value
    //printf("Input Capture EVENT!!!"); line used for debugging
    TCNT1=0;                // Reseting the timer to zero
    TIFR1|=1<<ICF1;         // Reseting the input capture flag
    counter=0;              // Reseting overflow counter
    car_move_flag = true;   // Car is being moved
    distancecounter++;
    seconds = ((double)timer*1000)/15625000;    // Time calculation (Seconds)
    speed = eigthcircumference/seconds;
    distance = (double)distancecounter*eigthcircumference;  // Distance calculation
    distancetogo = (double)rallystages[stages_driven].stagedistance - distance;
    secondstogo = secondstogo - seconds;
    
}

ISR(TIMER1_OVF_vect){
    counter++;            // Adding one to the overflow counter
    TCNT1=0;
    if(counter>=2){        // The car has not really moved for a long time
        car_move_flag = 0;// So the move-flag is reset
        speed = 0;
    }
}

//main function
int main(void) {    
    
    uart_init();   // Open the communication to the microcontroller
	io_redirect(); // Redirect input and output to the communication
    initialize();

    while(1){
    // Reseting all the values
    speed=0;
    prev_speed=0;
    acceleration_flag=0;
    car_move_flag = false;
    currentpagenumber=0;
    stages_driven = 0;
    ocr0asetter = 0;

    //printf("%lf", speed); 
        rxexpect=0x65;
        while(!(readBuffer[0]==0x65 && readBuffer[1]==0x00 && readBuffer[2]==0x01));
        /*for(i=0;i<8;i++){//stringreader for debugging purpose
                //printf("%c",savereadBuffer[0]);
                printf("%c",readBuffer[i]);
                if(readBuffer[i]==0x65)
                printf("Elements %d",i);
            }*/
        /*
        do{
        getpage();
        }while(currentpagenumber!=4);
        */
       //printf("debug");
        while(!(readBuffer[0]==0x65 && readBuffer[1]==0x04 && readBuffer[2]==0x06)){
        _delay_ms(1000);
        //printf("debug");
        /*for(i=0;i<8;i++){//stringreader for debugging purpose
                //printf("%c",readBuffer[0]);
                printf("%c",readBuffer[i]);
                if(readBuffer[i]==0x65)
                printf("Elements %d",i);
            }*/
        }
        _delay_ms(250);
        rxexpect=0x71;
        printf("get %s.val%c%c%c","etap.n0",255,255,255);	//sends "get secpag.n0.val"
            _delay_ms(500);
            
            if(readBuffer[0] == 0x71 && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF && readBuffer[7] == 0xFF){
                
                stagesexpexted = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16)| (readBuffer[4] << 24);
                                
            }
        
        //Page rallystages
        
        printf("page 3%c%c%c",255,255,255);
        printf("Rallystages.n2.val=%d%c%c%c", stagesexpexted, 255, 255, 255);
        stagenumber=0;
        
        do{
        //wait for button
       
       
        printf("Rallystages.n0.val=%d%c%c%c", stagenumber+1, 255, 255, 255);
        rxexpect=0x65;
        while(!(readBuffer[0]==0x65 && readBuffer[1]==0x03 && readBuffer[2]==0x04));
        //reading the confirmed values
        _delay_ms(51);
        rxexpect=0x71;
        printf("get %s.val%c%c%c","Rallystages.x0",255,255,255);	//sends "get secpag.n0.val"
            _delay_ms(51);
            
            if(readBuffer[0] == 0x71 && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF && readBuffer[7] == 0xFF){
                
                rallystages[stagenumber].stagedistance = (readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16)| (readBuffer[4] << 24));
                rallystages[stagenumber].stagedistance /= 10;
                //printf("v:%f",rallystages[stagenumber].distance);
                //printf("Rallystages.x0.val=%d%c%c%c",/*(int)rallystages[stagenumber].distance*10*/0 , 255,255,255);
                
            }
        printf("get %s.val%c%c%c","Rallystages.n1",255,255,255);	//sends "get secpag.n0.val"
            _delay_ms(51);

            if(readBuffer[0] == 0x71 && readBuffer[5] == 0xFF && readBuffer[6] == 0xFF && readBuffer[7] == 0xFF){
                //printf("debug2");
                rallystages[stagenumber].stagetime = readBuffer[1] | (readBuffer[2] << 8) | (readBuffer[3] << 16)| (readBuffer[4] << 24);
                //printf("v:%f",rallystages[stagenumber].stagetime);             
            }
        
        //calculating the expected average speed
        //rallystages[stagenumber].stagespeed = rallystages[stagenumber].stagedistance/rallystages[stagenumber].stagetime;
        

        stagenumber++;
        }while(stagesexpexted>stagenumber);//check condition
        //printf("%f",rallystages[0].stagespeed);
        stagenumber=0;

        //starting the car
        //go to next page
        printf("page 5%c%c%c",255,255,255);

        rxexpect=0x65;
        while(!(readBuffer[0]==0x65 && readBuffer[1]==0x05 && readBuffer[2]==0x01 && readBuffer[3]==0x01));//stops the car form doing anything until start button is pressed
       // _delay_ms(51);
        
        
        
        cardriver(stagesexpexted);
        printf("progress.x0.val=%lu%c%c%c", (unsigned long int)(speed*1000), 255,255,255);
        printf("progress.x1.val=%lu%c%c%c", (unsigned long int)(distance*1000), 255,255,255);
        printf("progress.x2.val=%lu%c%c%c", (unsigned long int)(distancetogo*1000), 255,255,255);
        printf("progress.n1.val=%u%c%c%c",read_adc(),255,255,255);
        printf("progress.x3.val=%lu%c%c%c", (unsigned long int)(secondstogo*1000), 255,255,255);
        printf("progress.j0.val=%lu%c%c%c", (unsigned long int)((rallystages[stages_driven].stagedistance/distance)*100), 255,255,255);
        
        rxexpect = 0x65;
        while(!(readBuffer[0]==0x65 && readBuffer[1]==0x06 && readBuffer[2]==0x10));

        printf("page 0%c%c%c",255,255,255);
        }
            return 0;
        }

/* Function descriptions */

//function for initializing interrups and the timer for the optocoupler
inline void initialize(void){
    sei();  // Enable global interrupts
    test = 12;
    // Usart interrupts
    SREG |= 1<<SREG_I;
    UCSR0B |=1<<RXCIE0; // Enabeling interrupt for rx complete
    
    TIMSK1 |= (1<<ICIE1)|(1<<TOIE1);    // Timer interrupts must be enabled
    TCCR1A = 0x00;
    TCCR1B = (1<<ICNC1)|/*(1<<ICES1)|*/(1<<CS12)|(1<<CS10);//noise cancel-/*falling*/ raising edge - 1024 prescaling
    DDRB &= ~0x01;//opto
    PORTB |= 0x01;
    TIFR1 |= 1<<ICF1;                   // Reseting input capture flag


    ADMUX = ADMUX | 0x40;//ADC0 single ended input on PortC0
    ADCSRB = ADCSRB & (0xF8);//Free running mode
    ADCSRA = ADCSRA | 0xE7; //Enable, Start conversion, slow input clock

}

//potential function to save input
inline char displaysave(void){
    for(i=0;i<100;i++){
        savereadBuffer[i] = readBuffer[i];
    }
}

//checking acceleration with optocoupler data
inline char acceleration_index(double current_speed, double previous_speed){
    char acceleration_flag;
    if(current_speed == 0 && previous_speed == 0){
        acceleration_flag=0;
    }else if(current_speed < prev_speed){
        acceleration_flag=1;
    }
        
    if(current_speed > prev_speed){
        acceleration_flag = 2;
    }
    if(speed==0){
        acceleration_flag = 0;
        //printf("debug");
    }
    return acceleration_flag;
}

 
inline void PWM_Motor(unsigned char duty){  
    DDRD = 0x60;    // Set Port D as output for the ENA (Motor) 0b0010 0000

    TCCR0A |= 0XA3;  // Fast PWM
    TCCR0B |= 0X05;    // 1024 Prescaler

    OCR0A = duty;
}

inline void updatedata(void){

    // printf("secpag.n0.val=%d%c%c%c", 7, 255,255,255);
    // printf("speed.val=%ld%c%c%c", (long int)(speed*1000), 255,255,255);

}

inline void getpage(void){

    rxexpect=0x66;
    printf("sendme%c%c%c",255,255,255);
    _delay_ms(51);
    currentpagenumber=readBuffer[1];
            
}

void cardriver(int stagecount){

    printf("progress.n0.val=%d%c%c%c",stages_driven+1,255,255,255);
    bool stagecompleteflag = false;

    distancetogo = rallystages[stages_driven].stagedistance;    
    printf("progress.x2.val=%ld%c%c%c", (long int)(distancetogo*1000), 255,255,255);
    
    secondstogo = rallystages[stages_driven].stagetime;    
    neededspeed = distancetogo/secondstogo;     
    printf("progress.x3.val=%ld%c%c%c", (unsigned long int)(secondstogo*1000), 255,255,255);
    
    speed=0;
    distance=0;
    if(rallystages[stages_driven].stagedistance>=2)
    ocr0asetter = 50;
    if(rallystages[stages_driven].stagedistance<=2)
    ocr0asetter = 30;
    PWM_Motor(ocr0asetter);
    _delay_ms(50);
    while(!stagecompleteflag){
        
        printf("progress.x0.val=%ld%c%c%c", (long int)(speed*1000), 255,255,255);
        printf("progress.x1.val=%ld%c%c%c", (long int)(distance*1000), 255,255,255);
        printf("progress.x2.val=%ld%c%c%c", (long int)(distancetogo*1000), 255,255,255);
        printf("progress.n1.val=%u%c%c%c",read_adc(),255,255,255);
        printf("progress.x3.val=%ld%c%c%c", (long int)(secondstogo*1000), 255,255,255);
        //printf("progress.j0.val=%lu%c%c%c", (unsigned long int)((rallystages[stages_driven].stagedistance/distance)*100), 255,255,255);

    
        neededspeed = distancetogo/secondstogo;
        if (speed<neededspeed && ocr0asetter<250){
           
            ocr0asetter+=1;
        }
        if (speed>neededspeed && ocr0asetter>25){
            ocr0asetter-=2;
        }
        



    PWM_Motor(ocr0asetter);
    
    acceleration_flag = acceleration_index(speed, prev_speed);
    prev_speed = speed;
    switch(acceleration_flag){

        case 1: 
        printf("progress.t4.txt=%s%c%c%c","\"breaking\"",255,255,255);
        break;
        case 2:
        printf("progress.t4.txt=%s%c%c%c","\"accelerating\"",255,255,255);
    }
    
    if(distance >= rallystages[stages_driven].stagedistance){
    stagecompleteflag = true;
    distancecounter = 0;
    distance=0;
    stages_driven++;
    }
    }
    
    //rallystages[stages_driven].stagespeed
    /*
    Get distance and time
    Run motor (set speed using time given) 
    Stop motor when distance has been reached*/
    

    if(stages_driven < stagecount){
        
        cardriver(stagecount);
    }
    
    PWM_Motor(0);
}

inline unsigned int read_adc(void){

    unsigned int adclow = ADCL;
    
    
    return (adclow + ((ADCH & 0x03) << 8));//need to ensure that ADCL is //read first as it is not updated otherwise
}
