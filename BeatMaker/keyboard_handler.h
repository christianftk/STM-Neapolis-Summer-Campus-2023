
#ifndef KEYBOARD_HANDLER_H
#define KEYBOARD_HANDLER_H

#include "ch.h"
#include "hal.h"

#define KEY_1       995-30
#define KEY_2       900-30
#define KEY_3       830-30
#define KEY_4       765-30
#define KEY_5       663-30
#define KEY_6       620-30
#define KEY_7       583-20
#define KEY_8       550-40
#define KEY_9       493 - 10
#define KEY_10      464
#define KEY_11      447
#define KEY_12      425
#define KEY_13      385
#define KEY_14      310

#define KEY_15      255
#define KEY_16      228-30


binary_semaphore_t msg_ready;
binary_semaphore_t space_ready;

typedef struct {
  int32_t key;
} keyboard_t;


#define MAX_VALUE            1000
#define HALF_ADC             2047
#define ADC_CHANNEL_NUM                                  1
#define ADC_BUF_DEPTH                                    12
static adcsample_t sample_buff[ADC_BUF_DEPTH] [ADC_CHANNEL_NUM];
/*
 * ADC conversion group.
 * Mode:        Linear buffer, SW triggered.
 * Channels:    IN1.
 */
static const ADCConversionGroup adcgrpcfg1 = {
          .circular     = false,
          .num_channels = ADC_CHANNEL_NUM,
          .end_cb       = NULL,
          .error_cb     = NULL,
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


void initSem(void) {
  chBSemObjectInit(&msg_ready, true);
  chBSemObjectInit(&space_ready, false);
}

static THD_WORKING_AREA(waKeyboardHandler, 1024);
static THD_FUNCTION(thdKeyboardHandler, arg) {

     keyboard_t* result = (keyboard_t*)arg;
    /*
     * Activates the ADC1 driver.
     */
    adcStart(&ADCD1, NULL);
    adcSTM32EnableVREF(&ADCD1);

    /* Improving Joystick calibration with offset computing */
    adcConvert(&ADCD1, &adcgrpcfg1, (adcsample_t*) sample_buff,
               ADC_BUF_DEPTH);

    /* Computing mean to remove noise */

    int32_t x_raw = 0;

    for(unsigned i = 0; i < ADC_BUF_DEPTH; i++){
      x_raw += sample_buff[i][0];
    }

    x_raw /= ADC_BUF_DEPTH;

    /* Computing offset. The value sampled from the joystick will be between [HALF_ADC - offset, HALF_ADC + offset] */
    int32_t x_offset = x_raw - HALF_ADC;

    while(TRUE) {

      /* Sampling data from Joystick */

      adcConvert(&ADCD1, &adcgrpcfg1, (adcsample_t*) sample_buff,
                 ADC_BUF_DEPTH);

      /* Computing mean to remove noise */
      x_raw = 0;

      for(unsigned i = 0; i < ADC_BUF_DEPTH; i++){
        x_raw += sample_buff[i][0];
      }
      x_raw /= ADC_BUF_DEPTH;


      /* Removing offset and centring range */
      x_raw -= (HALF_ADC + x_offset);

      chBSemWait(&space_ready);
      /* Resizing value properly */
      result->key = (x_raw < 0) ? x_raw * MAX_VALUE / (HALF_ADC + x_offset) : x_raw * MAX_VALUE / (HALF_ADC - x_offset);

      chBSemSignal(&msg_ready);
    }
  }


#endif
