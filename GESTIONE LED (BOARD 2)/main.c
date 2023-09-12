#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define VOLTAGE_RES            ((float)3.3/4096)

#define ADC_GRP_NUM_CHANNELS   1
#define ADC_GRP_BUF_DEPTH      16

ioline_t LED[7]= {
 PAL_LINE(GPIOC, 4),PAL_LINE(GPIOB, 10), PAL_LINE(GPIOA, 8), PAL_LINE(GPIOA, 9), PAL_LINE(GPIOC, 7), PAL_LINE(GPIOB, 6), PAL_LINE(GPIOD, 2)
};
//PIN SU ARDUINO CORRISPONDENTI: D1, D6, D7, D8, D9, D10
//GLI ULTIMI DUE PIN SONO SUL CONNETTORE MORPHO, PIN 4 E 29

static adcsample_t samples[ADC_GRP_NUM_CHANNELS * ADC_GRP_BUF_DEPTH];

/*
 * ADC error callback.
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
  (void)adcp;
  (void)err;
}
/*
 * ADC conversion group 1.
 * Mode:        Linear buffer, 16 samples of 1 channel, SW triggered.
 * Channels:    IN1.
 */
static const ADCConversionGroup adcgrpcfg1 = {
          .circular     = false,
          .num_channels = ADC_GRP_NUM_CHANNELS,
          .end_cb       = NULL,
          .error_cb     = adcerrorcallback,
          .cfgr         = ADC_CFGR_CONT,
          .cfgr2        = 0U,
          .tr1          = ADC_TR_DISABLED,
          .tr2          = ADC_TR_DISABLED,
          .tr3          = ADC_TR_DISABLED,
          .awd2cr       = 0U,
          .awd3cr       = 0U,
          .smpr         = {
            ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_247P5),
            0U
          },
          .sqr          = {
            ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1),
            0U,
            0U,
            0U
          }
        };

/*
 * Application entry point.
 */
int main(void) {

  halInit();
  chSysInit();

  /* ADC input.*/
  palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
  for (uint8_t i=0; i<7; i++){
    palSetLineMode(LED[i], PAL_MODE_OUTPUT_PUSHPULL);
  }

  /*
   * Starting ADCD1 driver and the temperature sensor.
   */
  adcStart(&ADCD1, NULL);
  adcSTM32EnableVREF(&ADCD1);

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (true) {
      float vol = 0.0f;
      /*
       * Capture, Convert samples and then make the average value!
       */
      adcConvert(&ADCD1, &adcgrpcfg1, samples, ADC_GRP_BUF_DEPTH);

      for( int i = 0; i < ADC_GRP_BUF_DEPTH; i++ ) {
        vol += (float) samples[i] * VOLTAGE_RES;
      }
      vol /= ADC_GRP_BUF_DEPTH;

      if(vol< 0.15f) { //Spegnimento
         chThdSleepMilliseconds(4);
      }
      if( vol > 0.15f ) { //LED 1
        palSetLine(LED[0]);
        for (uint8_t i = 1; i<7; i++){
          palClearLine(LED[i]);
        }
      }
      if( vol > 0.65f ) { //LED 2
        for (uint8_t i = 0; i<2; i++){
          palSetLine(LED[i]);
          }
        for (uint8_t i = 2; i<7; i++){
           palClearLine(LED[i]);
           }
      }
      if( vol > 1.15f ) { //LED 3
        for (uint8_t i = 0; i<3; i++){
          palSetLine(LED[i]);
          }
        for (uint8_t i = 3; i<7; i++){
           palClearLine(LED[i]);
           }
      }
      if( vol > 1.65f ) { //LED 4
        for (uint8_t i = 0; i<4; i++){
          palSetLine(LED[i]);
          }
        for (uint8_t i = 4; i<7; i++){
                   palClearLine(LED[i]);
                   }
      }
      if( vol > 2.15f ) { //LED 5
             for (uint8_t i = 0; i<5; i++){
               palSetLine(LED[i]);
               }
             for (uint8_t i = 5; i<7; i++){
                        palClearLine(LED[i]);
                        }
           }
      if( vol > 2.30f ) { //LED 6
             for (uint8_t i = 0; i<6; i++){
               palSetLine(LED[i]);
               }
             for (uint8_t i = 6; i<7; i++){
                        palClearLine(LED[i]);
                        }
           }
      if( vol > 2.73f ) { //LED 7
             for (uint8_t i = 0; i<7; i++){
               palSetLine(LED[i]);
               }
           }

    chThdSleepMilliseconds(50);
  }

  adcStop(&ADCD1);
}
