#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <string.h>
#include <stdlib.h> /* atoi */
#include "keyboard_handler.h"
#include "ssd1306.h"// Requires I2C Driver enabled in HAL and MCU
#include "image.c"

#define SWITCH_MODE_LIB 1
#define SWITCH_MODE_BOX 2
static int switch_mode = 1;
static int startCanzone = 0;

#include "speaker_handler.h"


#define BUFF_SIZE   20
static char lastPressed[BUFF_SIZE];
char buff[BUFF_SIZE];

// costanti per display
static const I2CConfig i2ccfg1 = {
                                  // I2C_TIMINGR address offset
                                  .timingr = 0x10,
                                  .cr1 = 0,
                                  .cr2 = 1,
};

static const SSD1306Config ssd1306cfg1 = {
                                          &I2CD1,
                                          &i2ccfg1,
                                          SSD1306_SAD_0X78, //indirizzo display
};
static const SSD1306Config ssd1306cfg2 = {
                                          &I2CD2,
                                          &i2ccfg1,
                                          SSD1306_SAD_0X78, //indirizzo display
};
//Thread per modalita' display

static SSD1306Driver SSD1306D1;
static SSD1306Driver SSD1306D2;


void printImageLogo(void){
  ssd1306FillScreen(&SSD1306D2, 0x00);
  ssd1306UpdateScreen(&SSD1306D2);
  // PRINT LOGO
  int x = 0;
  int y = 0;
  int c = 0;
  ssd1306GotoXy(&SSD1306D2, 0, 0);
  for(y = 0; y<SSD1306_HEIGHT; y++){
    for(x = 0; x<SSD1306_WIDTH; x++){
      ssd1306DrawPixel(&SSD1306D2, x, y, logo[c]);
      c++;
    }
    ssd1306UpdateScreen(&SSD1306D2);
  }
  ssd1306UpdateScreen(&SSD1306D2);
}


#define WA_LOGO   512
static THD_WORKING_AREA(waLogo, WA_LOGO);
static THD_FUNCTION(logoThread, arg) { // thread del display con logo
  (void)arg;

  chRegSetThreadName("Logo_Schermo");
  /*
   * Create logo Screen Thread!
   */

  //Initialize SSD1306 Display Driver Object.
  ssd1306ObjectInit(&SSD1306D2);
  //Start the SSD1306 Display Driver Object with configuration.
  ssd1306Start(&SSD1306D2, &ssd1306cfg2);
  ssd1306FillScreen(&SSD1306D2, 0x00);
  ssd1306UpdateScreen(&SSD1306D2);

  printImageLogo();
  while(true){
    chThdSleepMilliseconds(2000);
  }

}

static uint32_t pixeleffect = 1;
static uint32_t pixeleffect2 = 64;

static void rainEffect(void){
  //
  // START Rain animation
  //
  if(pixeleffect > 2000) pixeleffect = 1;
  ssd1306DrawPixel(&SSD1306D1,pixeleffect%128, pixeleffect%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect+3)%128, -(pixeleffect+3)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect+4)%128, (pixeleffect+4)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect+3)%128, -(pixeleffect+4)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect+4)%128, (pixeleffect+3)%48+24, SSD1306_COLOR_WHITE);

  if(pixeleffect2 > 2000) pixeleffect2 = 1;
  ssd1306DrawPixel(&SSD1306D1,pixeleffect2%128, pixeleffect2%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect2+3)%128, (pixeleffect2+3)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,-(pixeleffect2+4)%128, -(pixeleffect2+4)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect2+3)%128, (pixeleffect2+4)%48+24, SSD1306_COLOR_WHITE);
  ssd1306DrawPixel(&SSD1306D1,(pixeleffect2+4)%128, -(pixeleffect2+3)%48+24, SSD1306_COLOR_WHITE);
  pixeleffect+=5;
  pixeleffect2+=3;
  //
  // END ANIMATION
  //
}

#define WA_SCHERMO   512
static THD_WORKING_AREA(waSchermo, WA_SCHERMO);
static THD_FUNCTION(schermoThread, arg) { //thread che inizializza il display
  (void)arg;

  chRegSetThreadName("Mod_Schermo");

  /*
   * Create Screen Thread!
   */
  //Initialize SSD1306 Display Driver Object.
  ssd1306ObjectInit(&SSD1306D1);
  //Start the SSD1306 Display Driver Object with configuration.
  ssd1306Start(&SSD1306D1, &ssd1306cfg1);
  ssd1306FillScreen(&SSD1306D1, 0x00);


  ioline_t swt = PAL_LINE(GPIOB, 13);
  palSetLineMode(swt, PAL_MODE_INPUT);

  // Switch che seleziona modalità
  // starta thread mod schermo
  while (true) {
    ssd1306FillScreen(&SSD1306D1, 0x00);

    rainEffect();

    if (palReadLine(swt) == PAL_LOW){
      switch_mode = SWITCH_MODE_LIB;
      startCanzone = 0;
      ssd1306GotoXy(&SSD1306D1, 0, 1);
      chsnprintf(buff, BUFF_SIZE, "   Piano    ");
      ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_11x18, SSD1306_COLOR_BLACK);
      ssd1306GotoXy(&SSD1306D1, 48, 34);
      ssd1306Puts(&SSD1306D1, lastPressed, &ssd1306_font_11x18, SSD1306_COLOR_WHITE);
    }
    else if (palReadLine(swt) == PAL_HIGH){
      switch_mode = SWITCH_MODE_BOX;
      ssd1306GotoXy(&SSD1306D1, 0, 1);
      chsnprintf(buff, BUFF_SIZE, "  Jukebox   ");
      ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_11x18, SSD1306_COLOR_BLACK);
      if(strcmp(lastPressed, "Stop") == 0 || strcmp(lastPressed, " ~") == 0){
        // Jukebox homescreen quando non e' selezionata nessuna canzone
        ssd1306GotoXy(&SSD1306D1, 0, 28);
        chsnprintf(buff, BUFF_SIZE, "Seleziona canzone");
        ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_7x10, SSD1306_COLOR_WHITE);
        ssd1306GotoXy(&SSD1306D1, 0, 38);
        chsnprintf(buff, BUFF_SIZE, "da pulsantiera");
        ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_7x10, SSD1306_COLOR_WHITE);
      }
      else{
        ssd1306GotoXy(&SSD1306D1, 0, 28);
        ssd1306Puts(&SSD1306D1, lastPressed, &ssd1306_font_7x10, SSD1306_COLOR_WHITE);
        ssd1306GotoXy(&SSD1306D1, 0, 52);
        chsnprintf(buff, BUFF_SIZE, "Premi Play");
        ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_7x10, SSD1306_COLOR_WHITE);
      }

      // Esempio selezione
    }
    else{
      ssd1306GotoXy(&SSD1306D1, 0, 1);
      chsnprintf(buff, BUFF_SIZE, "Errore bottone");
      ssd1306Puts(&SSD1306D1, buff, &ssd1306_font_11x18, SSD1306_COLOR_WHITE);
    }
    ssd1306UpdateScreen(&SSD1306D1);
    chThdSleepMilliseconds(100);
  }//while
}//thd


static void readKey(int32_t key){
  if(switch_mode == SWITCH_MODE_LIB){
    if(key > KEY_1){
      current_note = 0;
      chsnprintf(lastPressed, BUFF_SIZE, "Do");
    } else if (key > KEY_2){
      current_note = 1;
      chsnprintf(lastPressed, BUFF_SIZE, "Do#");
    } else if (key > KEY_3){
      current_note = 2;
      chsnprintf(lastPressed, BUFF_SIZE, "Re");
    } else if (key > KEY_4){
      current_note = 3;
      chsnprintf(lastPressed, BUFF_SIZE, "Re#");
    } else if (key > KEY_5){
      current_note = 4;
      chsnprintf(lastPressed, BUFF_SIZE, "Mi");
    } else if (key > KEY_6){
      current_note = 5;
      chsnprintf(lastPressed, BUFF_SIZE, "Fa");
    } else if (key > KEY_7){
      current_note = 6;
      chsnprintf(lastPressed, BUFF_SIZE, "Fa#");
    } else if (key > KEY_8){
      current_note = 7;
      chsnprintf(lastPressed, BUFF_SIZE, "Sol");
    } else if (key > KEY_9){
      current_note = 8;
      chsnprintf(lastPressed, BUFF_SIZE, "Sol#");
    } else if (key > KEY_10){
      current_note = 9;
      chsnprintf(lastPressed, BUFF_SIZE, "La");
    } else if (key > KEY_11){
      current_note = 10;
      chsnprintf(lastPressed, BUFF_SIZE, "La#");
    } else if (key > KEY_12){
      current_note = 11;
      chsnprintf(lastPressed, BUFF_SIZE, "Si");
    } else if (key > KEY_13){
      current_octave = (current_octave < 2) ? current_octave++ : 0;
      chsnprintf(lastPressed, BUFF_SIZE, "Scala +");
    } else if (key > KEY_14){
      current_octave = (current_octave > 0) ? current_octave-- : 2;
      chsnprintf(lastPressed, BUFF_SIZE, "Scala -");
    }
    else if (key > KEY_15){
      current_note = -1;
    }
    else if (key > KEY_16){
      current_note = -1;
    }
    else{
      current_note = -1;
      chsnprintf(lastPressed, BUFF_SIZE, " ~");
    }
  }
  else if(switch_mode == SWITCH_MODE_BOX){
    if(key > KEY_1){
      current_note = 0;
      chsnprintf(lastPressed, BUFF_SIZE, "Nino D'Angelo");
    } else if (key > KEY_2){
      current_note = 1;
      chsnprintf(lastPressed, BUFF_SIZE, "What is Love");
    } else if (key > KEY_3){
      current_note = 2;
      chsnprintf(lastPressed, BUFF_SIZE, "Vesuvio Erutta");
    } else if (key > KEY_4){
      current_note = 3;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 4");
    } else if (key > KEY_5){
      current_note = 4;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 5");
    } else if (key > KEY_6){
      current_note = 5;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 6");
    } else if (key > KEY_7){
      current_note = 6;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 7");
    } else if (key > KEY_8){
      current_note = 7;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 8");
    } else if (key > KEY_9){
      current_note = 8;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 9");
    } else if (key > KEY_10){
      current_note = 9;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 10");
    } else if (key > KEY_11){
      current_note = 10;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 11");
    } else if (key > KEY_12){
      current_note = 11;
      chsnprintf(lastPressed, BUFF_SIZE, "Canzone 12");
    } else if (key > KEY_13){
      current_octave = (current_octave < 2) ? current_octave++ : 0;
      chsnprintf(lastPressed, BUFF_SIZE, "Scala +");
    } else if (key > KEY_14){
      current_octave = (current_octave > 0) ? current_octave-- : 2;
      chsnprintf(lastPressed, BUFF_SIZE, "Scala -");
    }
    else if (key > KEY_15){
      if (current_note > -1){
        startCanzone = 1;
      }
    }
    else if (key > KEY_16){
      startCanzone = 0;
      strcpy(lastPressed, "Stop");
    }
  }

}

#define WA_PULSANTIERA  1024
static THD_WORKING_AREA(waPulsantiera, WA_PULSANTIERA);
static THD_FUNCTION(pulsantieraThread, arg){ // thread della pulsantiera
  (void) arg;
  keyboard_t keyboard;
  chThdCreateStatic(waKeyboardHandler, sizeof(waKeyboardHandler), NORMALPRIO, thdKeyboardHandler, (void*)& keyboard);

  while(true) {
    chBSemWait(&msg_ready);
    readKey(keyboard.key);
    chBSemSignal(&space_ready);
    chThdSleepMilliseconds(50);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  halInit();
  chSysInit();
  strcpy(lastPressed, " ~");
  // Configuring I2C1 related PINs
  palSetLineMode(PAL_LINE(GPIOB, 8U), PAL_MODE_ALTERNATE(4) | //D15
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(PAL_LINE(GPIOB, 9U), PAL_MODE_ALTERNATE(4) | //D14
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);

  // Configuring I2C2 related PINs
  palSetLineMode(PAL_LINE(GPIOA, 8U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(PAL_LINE(GPIOA, 9U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);
  /*
   * Correspondence with ADC1_IN7 and ADC1_IN8
   */
  palSetPadMode( GPIOC, 1, PAL_MODE_INPUT_ANALOG );
  palSetPadMode( GPIOC, 2, PAL_MODE_INPUT_ANALOG );

  // Start ADC
  adcStart(&ADCD1, NULL);
  adcSTM32EnableVREF(&ADCD1);

  initSem();

  chThdCreateStatic(waLogo, sizeof(waLogo), NORMALPRIO, logoThread, NULL);
  chThdCreateStatic(waSchermo, sizeof(waSchermo), NORMALPRIO, schermoThread, NULL);
  chThdCreateStatic(waPulsantiera, sizeof(waPulsantiera), NORMALPRIO , pulsantieraThread, NULL);
  chThdCreateStatic(waSpeakerHandler, sizeof(waSpeakerHandler), NORMALPRIO, thdSpeakerHandler, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}





















