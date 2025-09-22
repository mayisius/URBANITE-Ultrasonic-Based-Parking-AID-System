/**
 * @file stm32f4_display.h
 * @brief Header for stm32f4_display.c file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date 01/04/2025
 */
#ifndef STM32F4_DISPLAY_SYSTEM_H_
#define STM32F4_DISPLAY_SYSTEM_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>

/* HW dependent includes */
#include "stm32f4xx.h"

/* Defines and enums ----------------------------------------------------------*/
/* Defines */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_R_GPIO GPIOB   /*!<    Red LED GPIO port   */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_R_PIN  6       /*!<    Red LED GPIO pin    */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_G_GPIO GPIOB   /*!<    Green LED GPIO port */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_G_PIN  8       /*!<    Green LED GPIO pin  */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_B_GPIO GPIOB   /*!<    Blue LED GPIO port  */
#define 	STM32F4_REAR_PARKING_DISPLAY_RGB_B_PIN  9       /*!<    Blue LED GPIO pin   */


#endif /* STM32F4_DISPLAY_SYSTEM_H_ */