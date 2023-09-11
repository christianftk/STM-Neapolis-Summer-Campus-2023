
#ifndef SPEAKER_HANDLER_H
#define SPEAKER_HANDLER_H

#include "ch.h"
#include "hal.h"
#include "notes.h"
#include "keyboard_handler.h"

#define PORTAB_LINE_LED1            LINE_LED
#define PORTAB_LED_OFF              PAL_LOW
#define PORTAB_LED_ON               PAL_HIGH
#define PORTAB_LINE_BUTTON          LINE_BUTTON
#define PORTAB_BUTTON_PRESSED       PAL_HIGH

#define PORTAB_SD1                  LPSD1

#define PORTAB_DAC_TRIG             7

static unsigned short flag = 0;

size_t nx = 0, ny = 0, nz = 0;
static void end_cb1(DACDriver *dacp) {

  nz++;
  if (dacIsBufferComplete(dacp)) {
    nx += DAC_BUFFER_SIZE / 4;
  }
  else {
    ny += DAC_BUFFER_SIZE / 4;
  }
}


/*
 * DAC error callback.
 */
static void error_cb1(DACDriver *dacp, dacerror_t err) {

  (void)dacp;
  (void)err;

  chSysHalt("DAC failure");
}

static const DACConfig dac1cfg1 = {
  .init         = 2047U,
  .datamode     = DAC_DHRM_12BIT_RIGHT,
  .cr           = 0
};

static const DACConversionGroup dacgrpcfg1 = {
  .num_channels = 1U,
  .end_cb       = end_cb1,
  .error_cb     = error_cb1,
  .trigger      = DAC_TRG(PORTAB_DAC_TRIG)
};

/*
 * GPT6 configuration.
 */
static GPTConfig gpt6cfg1 = {
  .frequency    = 2*DAC_BUFFER_SIZE*canzoni[0][0].freq,
  .callback     = NULL,
  .cr2          = TIM_CR2_MMS_1,    /* MMS = 010 = TRGO on Update Event.    */
  .dier         = 0U
};


static int32_t notes[3][12] ={{NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4},
                              {NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5},
                              {NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6}};

static int32_t current_note = -1;
static int32_t current_octave = 0;

static THD_WORKING_AREA(waSpeakerHandler, 1024);
static THD_FUNCTION(thdSpeakerHandler, arg) {
  (void)arg;
  /* Starting DAC1 driver.*/
  dacStart(&DACD1, &dac1cfg1);
  /* Starting GPT6 driver, it is used for triggering the DAC.*/
  gptStart(&GPTD6, &gpt6cfg1);
  int cont = 0;
  int sizeCanzone = 0;
  current_note = -1;
  current_octave = 0;


  while(true) {
    chBSemWait(&msg_ready);
    chBSemSignal(&space_ready);


    if(switch_mode == SWITCH_MODE_LIB){
      if(current_note != -1 && flag == 0){
        gpt6cfg1.frequency = 2*DAC_BUFFER_SIZE*notes[current_octave][current_note];
        gptStart(&GPTD6, &gpt6cfg1);
        dacStartConversion(&DACD1, &dacgrpcfg1, (dacsample_t*)dac_buffer, DAC_BUFFER_SIZE);
        gptStartContinuous(&GPTD6, 2U);
        chThdSleepMilliseconds(200);
        flag = 1;

      }else if(current_note == -1 || flag == 1){
        gptStopTimer(&GPTD6);
        dacStopConversion(&DACD1);
        flag = 0;
      }
    } else {
      if (startCanzone != 1) continue;
      // Dimensione canzone
      cont = 0;
      switch(current_note){
      case 0:
        sizeCanzone = NINO_SIZE;
        break;
      case 1:
        sizeCanzone = LOVE_SIZE;
        break;
      case 2:
        sizeCanzone = VESUVIO_SIZE;
        break;
      default:
        continue;
      }
      while (true && startCanzone) {
          while(true && startCanzone){
            if(cont == sizeCanzone) break;
            /* Starting a continuous conversion.*/
              dacStartConversion(&DACD1, &dacgrpcfg1, (dacsample_t *)dac_buffer, DAC_BUFFER_SIZE);
              gptStartContinuous(&GPTD6, 2U);
              chThdSleepMilliseconds(canzoni[current_note][cont].duration * 2.3 );
              gptStopTimer(&GPTD6);
              chThdSleepMilliseconds(canzoni[current_note][cont].pause * 2.3);
              cont++;
              gpt6cfg1.frequency= 2*DAC_BUFFER_SIZE*canzoni[current_note][cont].freq;
              gptStart(&GPTD6, &gpt6cfg1);
            }
          cont = 0;
          gpt6cfg1.frequency= 2*DAC_BUFFER_SIZE*canzoni[current_note][cont].freq;
        }
    }
  }

}


#endif /* SPEAKER_HANDLER_H_ */
