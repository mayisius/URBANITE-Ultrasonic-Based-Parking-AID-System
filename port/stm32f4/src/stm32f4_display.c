/**
 * @file stm32f4_display.c
 * @brief Portable functions to interact with the display system FSM library. All portable functions must be implemented in this file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date 01/04/2025
 */

/* Standard C includes */
#include <stdio.h>

/* HW dependent includes */
#include "port_display.h"
#include "port_system.h"

/* Microcontroller dependent includes */
#include "stm32f4_system.h"
#include "stm32f4_display.h"

/* Defines --------------------------------------------------------------------*/

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the HW dependencies of an RGB LED.
 */
typedef struct {
    GPIO_TypeDef *p_port_red;     /*!< GPIO port for the red LED */
    uint8_t       pin_red;        /*!< Pin number for the red LED */
    GPIO_TypeDef *p_port_green;   /*!< GPIO port for the green LED */
    uint8_t       pin_green;      /*!< Pin number for the green LED */
    GPIO_TypeDef *p_port_blue;    /*!< GPIO port for the blue LED */
    uint8_t       pin_blue;       /*!< Pin number for the blue LED */
} stm32f4_display_hw_t;


/* Global variables */
/**
 * @brief Array of elements that represents the HW characteristics of the RGB LED of the display systems connected to the STM32F4 platform.
 * This must be hidden from the user, so it is declared as static. To access the elements of this array, use the function _stm32f4_display_get().
 * 
 */
static stm32f4_display_hw_t displays_arr [] = {
    [PORT_REAR_PARKING_DISPLAY_ID] = {.p_port_red = STM32F4_REAR_PARKING_DISPLAY_RGB_R_GPIO, .pin_red = STM32F4_REAR_PARKING_DISPLAY_RGB_R_PIN, .p_port_green = STM32F4_REAR_PARKING_DISPLAY_RGB_G_GPIO, .pin_green = STM32F4_REAR_PARKING_DISPLAY_RGB_G_PIN, .p_port_blue = STM32F4_REAR_PARKING_DISPLAY_RGB_B_GPIO, .pin_blue = STM32F4_REAR_PARKING_DISPLAY_RGB_B_PIN},
};

/* Private functions -----------------------------------------------------------*/
/**
 * @brief Get the display struct with the given ID.
 * 
 * @param display_id    Button ID.
 * @return stm32f4_display_hw_t*    Pointer to the display struct. NULL If the display ID is not valid.
 */
stm32f4_display_hw_t *_stm32f4_display_get(uint32_t display_id)
{
    if (display_id < sizeof(displays_arr) / sizeof(displays_arr[0]))
    {
        return &displays_arr[display_id];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Configure the timer that controls the PWM of each one of the RGB LEDs of the display system.
 * This function is called by the port_display_init() public function to configure the timer that controls the PWM of the RGB LEDs of the display.
 * 
 * @param display_id    Display system identifier number.
 */
void _timer_pwm_config (uint32_t display_id)
{
    TIM_TypeDef *TIMx = NULL;
    uint32_t RCC_APB1ENR_TIMxEN = 0;

    switch (display_id)
    {
    case PORT_REAR_PARKING_DISPLAY_ID:
        TIMx = TIM4;
        RCC_APB1ENR_TIMxEN = RCC_APB1ENR_TIM4EN;

        break;

    default:
        return; // Invalid display ID
    }

    // Enable the clock of the timer that controls the trigger signal.
    RCC->APB1ENR |= RCC_APB1ENR_TIMxEN;

    // Disable counter and enable auto-reload preload
    TIMx->CR1 &= ~TIM_CR1_CEN;
    TIMx->CR1 |= TIM_CR1_ARPE;

    // Reset timer counter
    TIMx->CNT = 0;

    // Configure prescaler and auto-reload for 50Hz PWM
    TIMx->PSC = 4;              // Timer frequency
    TIMx->ARR = 63999;          // PWM frequency

    // Disable output for all channels
    TIMx->CCER &= ~TIM_CCER_CC1E;   // CH1 (Red)
    TIMx->CCER &= ~TIM_CCER_CC3E;   // CH3 (Green)
    TIMx->CCER &= ~TIM_CCER_CC4E;   // CH4 (Blue)

    // Clear polarity bits for channels
    TIMx->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);    // CH1 (Red)
    TIMx->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC3NP);    // CH3 (Green)
    TIMx->CCER &= ~(TIM_CCER_CC4P | TIM_CCER_CC4NP);    // CH4 (Blue)

    // Set PWM mode 1 and enable output compare preload
    TIMx->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); // CH1 (Red)
    TIMx->CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1); // CH3 (Green)
    TIMx->CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1); // CH4 (Blue)

    TIMx->CCMR1 |= TIM_CCMR1_OC1PE;                         // CH1 (Reed)
    TIMx->CCMR2 |= (TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE);     // CH3 (Green) & CH4 (Blue)

    // Generate update event to apply changes
    TIMx->EGR |= TIM_EGR_UG;

    //Leave timer disabled at the end.
    TIMx->CR1 &= ~TIM_CR1_CEN;
}


/* Public functions -----------------------------------------------------------*/
void port_display_init (uint32_t display_id)
{
    // Get the display configuration structure
    stm32f4_display_hw_t *p_display = _stm32f4_display_get(display_id);
    if (p_display == NULL)
    {
        return;
    }
    
    // Configure RGB GPIOs in alternate function mode with no pull-up/pull-down
    stm32f4_system_gpio_config(p_display->p_port_red, p_display->pin_red, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);
    stm32f4_system_gpio_config(p_display->p_port_green, p_display->pin_green, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);
    stm32f4_system_gpio_config(p_display->p_port_blue, p_display->pin_blue, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);

    // Assign the alternate function (AF2 for TIM4)
    stm32f4_system_gpio_config_alternate(p_display->p_port_red, p_display->pin_red, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(p_display->p_port_green, p_display->pin_green, STM32F4_AF2);
    stm32f4_system_gpio_config_alternate(p_display->p_port_blue, p_display->pin_blue, STM32F4_AF2);

    // Configure the PWM timer
    _timer_pwm_config(display_id);

    // Set all RGB values to 0% (turn off display)
    port_display_set_rgb(display_id, COLOR_OFF);
}


void port_display_set_rgb (uint32_t display_id, rgb_color_t color)
{
    // Check display ID
    if (display_id != PORT_REAR_PARKING_DISPLAY_ID) return;

    // Retrieve RGB values
    uint8_t r = color.r;
    uint8_t g = color.g;
    uint8_t b = color.b;

    // Get timer and disable it
    TIM_TypeDef *TIMx = TIM4;
    TIMx->CR1 &= ~TIM_CR1_CEN;

    // All values are zero
    if ((r == 0) && (g == 0) && (b == 0))
    {
        // Disable all output channels
        TIMx->CCER &= ~TIM_CCER_CC1E; // Red
        TIMx->CCER &= ~TIM_CCER_CC3E; // Green
        TIMx->CCER &= ~TIM_CCER_CC4E; // Blue
    }
    else
    {
        // RED
        if (r == 0)
        {
            TIMx->CCER &= ~TIM_CCER_CC1E;
        }
        else
        {
            TIMx->CCR1 = ((uint32_t)r * TIM4->ARR) / PORT_DISPLAY_RGB_MAX_VALUE;
            TIMx->CCER |= TIM_CCER_CC1E;
        }

        // GREEN
        if (g == 0)
        {
            TIMx->CCER &= ~TIM_CCER_CC3E;
        }
        else
        {
            TIMx->CCR3 = ((uint32_t)g * TIM4->ARR) / PORT_DISPLAY_RGB_MAX_VALUE;
            TIMx->CCER |= TIM_CCER_CC3E;
        }

        // BLUE
        if (b == 0)
        {
            TIMx->CCER &= ~TIM_CCER_CC4E;
        }
        else
        {
            TIMx->CCR4 = ((uint32_t)b * TIM4->ARR) / PORT_DISPLAY_RGB_MAX_VALUE;
            TIMx->CCER |= TIM_CCER_CC4E;
        }

        // Update registers and enable timer
        TIMx->EGR |= TIM_EGR_UG;
        TIMx->CR1 |= TIM_CR1_CEN;
    }
}
