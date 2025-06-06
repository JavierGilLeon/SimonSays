/* --COPYRIGHT--,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//
// HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h -
//           Hardware abstraction layer for using the Educational Boosterpack's
//           Crystalfontz128x128 LCD with MSP-EXP432P401R LaunchPad
//
//*****************************************************************************

#ifndef __HAL_MSP_EXP432P401R_CRYSTALFONTZLCD_H_
#define __HAL_MSP_EXP432P401R_CRYSTALFONTZLCD_H_


#include <stdint.h>
//#include "driverlib.h"
//*****************************************************************************
//
// User Configuration for the LCD Driver
//
//*****************************************************************************

// System clock speed (in Hz)
#define LCD_SYSTEM_CLOCK_SPEED                 8000000
// SPI clock speed (in Hz)
#define LCD_SPI_CLOCK_SPEED                    (LCD_SYSTEM_CLOCK_SPEED)


#define CS_PIN BIT3
#define RES_PIN BIT7
#define RS_PIN BIT0

#define CS_LOW 		P2OUT&=~CS_PIN
#define CS_HIGH 	P2OUT|=CS_PIN
#define RS_LOW 		P2OUT&=~RS_PIN
#define RS_HIGH 	P2OUT|=RS_PIN
#define RES_LOW 	P2OUT&=~RES_PIN
#define RES_HIGH 	P2OUT|=RES_PIN



// Ports from MSP430 connected to LCD
#define LCD_MOSI_PORT                       P1OUT
#define LCD_SCLK_PORT                       P1OUT
#define LCD_POWER_DISP_PORT                 P1OUT
#define LCD_POWER_DISP_DIR                  P1DIR
#define LCD_SCS_PORT                        P1OUT
#define LCD_SCS_DIR                         P1DIR

// Pins from MSP430 connected to LCD
#define LCD_MOSI_PIN                        BIT7
#define LCD_MOSI_PORT_SEL1                                      P1SEL
#define LCD_MOSI_PORT_SEL2                  P1SEL2
#define LCD_SCLK_PIN                        BIT5
#define LCD_SCLK_PORT_SEL1                                      P1SEL
#define LCD_SCLK_PORT_SEL2                                      P1SEL2
#define LCD_POWER_PIN                       BIT0
#define LCD_DISP_PIN                        BIT3
#define LCD_SCS_PIN                         BIT4
/*

// Ports from MSP432 connected to LCD
#define LCD_SCK_PORT          GPIO_PORT_P1
#define LCD_SCK_PIN_FUNCTION  GPIO_PRIMARY_MODULE_FUNCTION
#define LCD_MOSI_PORT         GPIO_PORT_P1
#define LCD_MOSI_PIN_FUNCTION GPIO_PRIMARY_MODULE_FUNCTION
#define LCD_RST_PORT          GPIO_PORT_P5
#define LCD_CS_PORT           GPIO_PORT_P5
#define LCD_DC_PORT           GPIO_PORT_P3

// Pins from MSP432 connected to LCD
#define LCD_SCK_PIN           GPIO_PIN5
#define LCD_MOSI_PIN          GPIO_PIN6
#define LCD_RST_PIN           GPIO_PIN7
#define LCD_CS_PIN            GPIO_PIN0
#define LCD_DC_PIN            GPIO_PIN7
*/

// Definition of USCI base address to be used for SPI communication
#define LCD_EUSCI_BASE        EUSCI_B0_BASE

//*****************************************************************************
//
// Prototypes for the globals exported by this driver.
//
//*****************************************************************************
extern void HAL_LCD_writeCommand(uint8_t command);
extern void HAL_LCD_writeData(uint8_t data);
extern void HAL_LCD_PortInit(void);
extern void HAL_LCD_SpiInit(void);
extern void HAL_LCD_delay(int x);

#define HAL_LCD_delay(x)     __delay_cycles(x * 8)  //original: x * 48


#endif /* HAL_MSP_EXP432P401R_CRYSTALFONTZ128X128_ST7735_H_ */
