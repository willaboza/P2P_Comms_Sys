/*
 * pwm0.c
 *
 *  Created on: Apr 29, 2020
 *      Author: willi
 */

#include 'pwm0.h'

void initPwm0()
{
    // Enable clocks
    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;
    _delay_cycles(3);

    enablePort(PORTB);
    _delay_cycles(3);

    enablePort(PORTE);
    _delay_cycles(3);

    // Configure PWM0 pins
    selectPinPushPullOutput(PWM0_RED_LED);
    selectPinDigitalInput(PWM0_BLUE_LED);
    selectPinDigitalInput(PWM0_GREEN_LED);

    setPinAuxFunction(PWM0_RED_LED, GPIO_PCTL_PB5_PWM0);
    setPinAuxFunction(PWM0_BLUE_LED, GPIO_PCTL_PE4_PWM0);
    setPinAuxFunction(PWM0_GREEN_LED, GPIO_PCTL_PE5_PWM0);

    // output 3 on PWM0, gen 1b, cmpb
    PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
    // output 4 on PWM0, gen 2a, cmpa
    PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
    // output 5 on PWM0, gen 2b, cmpb
    PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;

    // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM0_1_LOAD_R = 1024;
    PWM0_2_LOAD_R = 1024;

    // invert outputs for duty cycle increases with increasing compare values
    PWM0_1_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
    PWM0_2_CMPB_R = 0;                               // green off
    PWM0_2_CMPA_R = 0;                               // blue off

    PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
    PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
    PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;// enable outputs


}

//Set Red,Green,and Blue LED Colors
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue)
{
    PWM0_1_CMPB_R = red;      //set value recorded for red
    PWM0_2_CMPA_R = blue;     //set value recorded for blue
    PWM0_2_CMPB_R = green;    //set value recorded for green
}

//return measured value in the range of 0... 255 using
// m = (m/th)*(tMax-tMin), where th = threshold, tMax
//and tMin is target max and min, and m = measured value
// Result = ((Input - Input_Low)/(Input_High - Input_Low)) * (Output_High - Output_low) + Output_Low
int normalizeRgbColor(int measurement)
{
    uint16 result;
    float numerator, denominator, ratio;

    snprintf(buffer, sizeof(buffer), "%u.0\0",measurement);
    numerator = atof(buffer);
    snprintf(buffer, sizeof(buffer), "%u.0\0",threshold);
    denominator = atof(buffer);
    ratio = (numerator/denominator) * 255.0;
    result = ratio;

    if(result > 255)
    {
        result = 255;
    }

    return result;
}

// Result = ((Input - Input_Low)/(Input_High - Input_Low)) *
// (Output_High - Output_low) + Output_Low
int scaleRgbColor()
{



}
