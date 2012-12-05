/* 
   Direct-digital synthesis
   ADSR Demo
   
*/

// ------- Preamble -------- //
#include <avr/io.h>		/* Defines pins, ports, etc */
#include <util/delay.h>		/* Functions to waste time */
#include <avr/interrupt.h>	
#include "pinDefines.h"
#include "macros.h"
#include "scale.h"
#include "fullWaves.h"
#include "USART.h"

#define FULL_VOLUME     31 	/* 5-bit volumes */

// Volume envelope default values (slightly percussive)
#define ATTACK_RATE    7
#define DECAY_RATE     50
#define SUSTAIN_LEVEL  20
#define SUSTAIN_TIME   2000
#define RELEASE_RATE   200

static inline void initTimer0(void){
  set_bit(TCCR0A, COM0A1);	/* PWM output on OCR0A */
  set_bit(SPEAKER_DDR, SPEAKER); /* enable output on pin */

  set_bit(TCCR0A, WGM00);	/* Fast PWM mode */
  set_bit(TCCR0A, WGM01);	/* Fast PWM mode, pt.2 */
  
  set_bit(TCCR0B, CS00);	/* Clock with /1 prescaler */
}

static inline void initLEDs(void){
  uint8_t i;
  LED_DDR = 0xff;	/* All LEDs for diagnostics */
  for (i=0; i<8; i++){
    set_bit(LED_PORT, i);
    _delay_ms(100);
    clear_bit(LED_PORT, i);
  }
}

int main(void){

  // -------- Inits --------- //

  uint16_t accumulator = 0;  
  uint8_t  volume = 0;
  uint16_t clock  = 0;
  uint16_t tuningWord = C1;    
  uint8_t waveStep;
  int8_t PWM;
  uint8_t i;
  uint8_t buttonPressed;

  // Initialize envelope parameters to default
  uint8_t attackRate = ATTACK_RATE;
  uint8_t decayRate =  DECAY_RATE;    
  uint8_t sustainLevel = SUSTAIN_LEVEL;
  uint16_t sustainTime = SUSTAIN_TIME;
  uint16_t releaseRate = RELEASE_RATE;

  uint16_t attackTime = attackRate * FULL_VOLUME;
  uint16_t decayTime = (attackTime + (FULL_VOLUME-sustainLevel) * DECAY_RATE);
  
  
  initLEDs();
  initTimer0();
  initUSART();
  

  set_bit(BUTTON_PORT, BUTTON);	/* pullup on button */
  set_bit(SPEAKER_DDR, SPEAKER); /* speaker output */
  
  // ------ Event loop ------ //
  while(1){		       

    // Take care of sound generation in this loop
    set_bit(LED_PORT, LED0);		/* debugging -- begins wait time */
    loop_until_bit_is_set(TIFR0, TOV0); /* wait for timer0 overflow */
    clear_bit(LED_PORT, LED0);		/* debugging -- ends wait time */
    accumulator += tuningWord;
    waveStep = (uint8_t) (accumulator >> 8);
    PWM = (fullTriangle[waveStep] * volume) >> 5;
    OCR0A = 128 + PWM; 		/* int8_t to uint8_t */
    set_bit(TIFR0, TOV0);		/* reset the overflow bit */

    /* Dynamic Volume stuff here */
    /* Note: this would make a good state machine */
  

    if (clock){		     /* if clock already running */
      clock++;
      if (clock < attackTime) { /* attack */
	if (clock > attackRate*volume){
	  if (volume < 31){
	    volume++;
	  }
	}
      }
      else if (clock < decayTime) {  /* decay */
	if ((clock - attackTime) > (FULL_VOLUME-volume)*decayRate){
	  if (volume > sustainLevel){
	    volume--;
	  }
	}
      }
      else if (clock > (decayTime + sustainTime)){  /* release  */
	if ((clock - (decayTime + sustainTime)) > (sustainLevel-volume)*releaseRate){
	  if (volume > 0){
	    volume--;
	  }
	  else{
	    clock = 0;
	  }
	}
      }
    }
    else {		       /* if not in clock loop, check USART */
      i = receiveByte();
      switch(i){
      case 'a':
	tuningWord = G1;
	break;
      case 's':
	tuningWord = A1;
	break;
      case 'd':
	tuningWord = B1;
	break;
      case 'f':
	tuningWord = C2;
	break;
      case 'g':
	tuningWord = D2;
	break;
      case 'h':
	tuningWord = E2;
	break;
      case 'j':
	tuningWord = F2;
	break;
      case 'k':
	tuningWord = G2;
	break;
      case 'l':
	tuningWord = A2;
	break;
      case ';':
	tuningWord = B2;
	break;
      case '\'':
	tuningWord = C3;
	break;
	
	// Change parameters
	
      }	/* end switch */

      	// Trigger clock
      clock = 1;
      uint16_t attackTime = attackRate * FULL_VOLUME;
      uint16_t decayTime = (attackTime + (FULL_VOLUME-sustainLevel) * DECAY_RATE);
      
    } /* end receive data "else" */
    
    
  } /* End event loop */
  return(0);		      /* This line is never reached  */
}
