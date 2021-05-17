/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

// Timer for our SynchSM
unsigned long ThreeLED_time = 300;
unsigned long BlinkLED_time = 1000;
unsigned long Sound_time = 2;


volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clea$

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start counter from here to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal clock of 1ms ticks

void set_PWM(double frequency) {
    static double current_frequency;

    if(frequency != current_frequency){
    if(!frequency) {TCCR3B &= 0x08;}
    else{TCCR3B |= 0x03;}

    if(frequency < 0.954) {OCR3A = 0xFFFF;}
    else if(frequency > 31250) {OCR3A = 0x0000;}
    else{OCR3A = (short)(8000000/(128*frequency)) - 1;}
    TCNT3 = 0;
    current_frequency = frequency;
    }
}

void PWM_on(){
    TCCR3A = (1 << COM3A0);
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

    set_PWM(0);
}

void PWM_off(){
    TCCR3A = 0x00;
    TCCR3B = 0x00;
}

void TimerOn(){
    // AVR timer/counter controller register TCCR1
    TCCR1B = 0x0B;      // bit3 = 0: CTC mode(clear timer on compare)
                        // bit2bit1bit0 = 011;
                        // 0000 1011 : 0x0B
                        // So 8 MHz clock or 8,000,000 / 64  = 125,000 ticks
                        // Thus TCNT1 register will count 125,000 ticks

    // AVR out compare register OCR1A
    OCR1A = 125;        // Timer interrupt will be generated when TCNT1==OCR1A
                        // We want a 1ms tick. .001s * 125,000 ticks = 125
                        // So when TCNT1 == 125 then that means 1ms has passed

    // AVR timer register interrupt mask register
    TIMSK1 = 0x02;      // bit1: OCIE1A -- enables compare match register

    // Initiliaze AVR counter
    TCNT1 = 0;

    _avr_timer_cntcurr = _avr_timer_M;

    // enable global interrupts
    SREG |= 0x80;       // 1000 0000
}

void TimerOff(){
    TCCR1B = 0x00;
}

void TimerISR(){
    TimerFlag = 1;
}

// In our approach C program does not touch this ISR, only TimerISR()
ISR(TIMER1_COMPA_vect){
    // CPU automatically calls when TCNT1 == OCR1 (Every 1ms per TimerOn settings)
    _avr_timer_cntcurr--;       // count down to 0
    if (_avr_timer_cntcurr == 0){
        TimerISR();     // call the ISR that the user uses
        _avr_timer_cntcurr = _avr_timer_M;
    }
}

// Set timer to tick every M ms.
void TimerSet(unsigned long M){
    _avr_timer_M = M;
    _avr_timer_cntcurr =  _avr_timer_M;
}



unsigned char threeLEDs;
unsigned char button;

enum ThreeLED_States {ThreeLED_Start, ThreeLED_B0, ThreeLED_B1, ThreeLED_B2} ThreeLED_State;

void ThreeLED(){
    switch(ThreeLED_State){
        case ThreeLED_Start:
            ThreeLED_State = ThreeLED_B0;
            break;
        
        case ThreeLED_B0:
            ThreeLED_State = ThreeLED_B1;
            break;
            
        case ThreeLED_B1:
            ThreeLED_State = ThreeLED_B2;
            break;
            
        case ThreeLED_B2:
            ThreeLED_State = ThreeLED_B0;
            break;
    }
    
    switch(ThreeLED_State){
         case ThreeLED_B0:
            threeLEDs = 0x01;
            break;
        case ThreeLED_B1:
            threeLEDs = 0x02;
            break;
        case ThreeLED_B2:
            threeLEDs = 0x04;
            break;
    }
    
}

unsigned char blinkingLED;
enum BlinkLED_States {BlinkLED_Start, BlinkLED_Off, BlinkLED_On} BlinkLED_State;

void BlinkingLED(){
    
    switch(BlinkLED_State){
        case BlinkLED_Start:
            BlinkLED_State = BlinkLED_Off;
            break;
        case BlinkLED_Off:
            BlinkLED_State = BlinkLED_On;
            break;
            
        case BlinkLED_On:
            BlinkLED_State = BlinkLED_Off;
            break;
        default:
            BlinkLED_State = BlinkLED_Off;
            break;
            
    }
    
    
    switch(BlinkLED_State){
        case BlinkLED_Off:
            blinkingLED = 0x00;
            break;
            
        case BlinkLED_On:
            blinkingLED = 0x08;
            break;
    }
    
}


unsigned char sound_status;
unsigned char sound_count;
unsigned char freq_count;
enum Sound_States {Sound_Start, Sound_Wait, Sound_Increment, Sound_Decrement, Sound_On, Sound_Off} Sound_State;
void Sound(){
    
    switch(Sound_State){
        case Sound_Start:
            Sound_State = Sound_Wait;
            sound_count = 0;
            break;
        
        case Sound_Wait:
            if(button & 0x04){
                Sound_State = Sound_On;
            } else if(button & 0x01){
	        Sound_State = Sound_Increment;
	    } else if(button & 0x02){
		Sound_State = Sound_Decrement;
	    }
	    else{
                Sound_State = Sound_Wait;
            }
            sound_status = 0;
            break;
            
    	case Sound_Increment:
	    Sound_State = Sound_Wait;
	    break;

	case Sound_Decrement:
	    Sound_State = Sound_Wait;
	    break;
    	
        case Sound_On:
            if(button && (sound_count < Sound_time)){
                switch(freq_count){
		    case 1: 
			set_PWM(261.63); 
			break;
		    case 2: 
			set_PWM(293.66); 
			break;
		    case 3: 
			set_PWM(329.63); 
			break;
		    case 4: 
			set_PWM(349.23);
			 break;
		    case 5: 
			set_PWM(392.00); 
			break;
		    case 6: 
			set_PWM(440.00);
			break;
		    case 7: 
			set_PWM(493.88); 
			break;
	   	    case 8: 
			set_PWM(500);
			 break;
		    default: 
			break;
		}
		Sound_State = Sound_On;
                sound_count++;
            } else if(button){
                Sound_State = Sound_Off;
                sound_count =0;
            } else {
                Sound_State = Sound_Wait;
            }
            break;

            
        case Sound_Off:
            if(button && (sound_count < Sound_time)){
                Sound_State = Sound_Off;
                sound_count++;
            }
            else if(button){
                Sound_State = Sound_On;
                sound_count =0;
            } else{
                Sound_State = Sound_Wait;
            }
            break;

        default:
            Sound_State = Sound_Start;
    }
    
    
    switch(Sound_State){
        case Sound_Off:
            sound_status = 0x00;
            break;
        case Sound_On:
            sound_status = 0x10;
            break;
	case Sound_Increment:
            if(freq_count!= 8){
                freq_count++;
            }
            break;
        case Sound_Decrement:
            if(freq_count != 1){
                freq_count--;
            }
	    break;
 

   }
    
}


void Combine(){
    PORTB = threeLEDs | blinkingLED | sound_status;
}



int main(void) {
/* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0x0F;
    DDRB = 0xFF; PORTB = 0x00;

    

    TimerSet(1);
    TimerOn();
    
    ThreeLED_State = ThreeLED_Start;
    BlinkLED_State = BlinkLED_Start;
    Sound_State = Sound_Start;
    freq_count =1;
    PWM_on();
    set_PWM(0);
    /* Insert your solution below */
    while (1) {
        
        button = ~PINA & 0x04;
        
        if(BlinkLED_time >= 1000){
            BlinkingLED();
            BlinkLED_time = 0;
        }
        if(ThreeLED_time >= 300){
            ThreeLED();
            ThreeLED_time = 0;
        }
        if(Sound_time >= 3){
            Sound();
            Sound_time=0;
	}
	
        Combine();
        
        while(!TimerFlag);
        TimerFlag = 0;
	    ThreeLED_time +=1;
	    BlinkLED_time +=1;
	    Sound_time +=1; 

   }
    return 1;
}


