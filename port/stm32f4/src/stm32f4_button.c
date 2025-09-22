/**
 * @file stm32f4_button.c
 * @brief Portable functions to interact with the button FSM library. All portable functions must be implemented in this file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdio.h>

/* HW dependent includes */
#include "port_button.h"    // Used to get general information about the buttons (ID, etc.)
#include "port_system.h"    // Used to get the system tick

/* Microcontroller dependent includes */
// TO-DO alumnos: include the necessary files to interact with the GPIOs
#include "stm32f4_system.h"
#include "stm32f4_button.h"


/* Typedefs --------------------------------------------------------------------*/
typedef struct
{
    GPIO_TypeDef *p_port;
    uint8_t pin;
    uint8_t pupd_mode;
    bool flag_pressed;
} stm32f4_button_hw_t;

/* Global variables ------------------------------------------------------------*/
static stm32f4_button_hw_t buttons_arr[] = {
    [PORT_PARKING_BUTTON_ID] = {.p_port = STM32F4_PARKING_BUTTON_GPIO, .pin = STM32F4_PARKING_BUTTON_PIN, .pupd_mode = STM32F4_GPIO_PUPDR_NOPULL},
};

/* Private functions ----------------------------------------------------------*/
/**
 * @brief Get the button status struct with the given ID.
 *
 * @param button_id Button ID.
 *
 * @return Pointer to the button state struct.
 * @return NULL If the button ID is not valid.
 */
stm32f4_button_hw_t *_stm32f4_button_get(uint32_t button_id)
{
    // Return the pointer to the button with the given ID. If the ID is not valid, return NULL.
    if (button_id < sizeof(buttons_arr) / sizeof(buttons_arr[0]))
    {
        return &buttons_arr[button_id];
    }
    else
    {
        return NULL;
    }
}

/* Public functions -----------------------------------------------------------*/

/**
 * @brief Configure the HW specifications of a given button.
 * 
 * @param button_id Button ID. This index is used to select the element of the buttons_arr[] array
 */
void port_button_init(uint32_t button_id)
{
    // Retrieve the button struct using the private function and the button ID
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);


    /* TO-DO alumnos */
    stm32f4_system_gpio_config(p_button->p_port, p_button->pin, STM32F4_GPIO_MODE_IN, p_button->pupd_mode);
    stm32f4_system_gpio_config_exti(p_button->p_port, p_button->pin, STM32F4_TRIGGER_BOTH_EDGE | STM32F4_TRIGGER_ENABLE_INTERR_REQ);
    stm32f4_system_gpio_exti_enable(p_button->pin, 1, 0);

}

void stm32f4_button_set_new_gpio(uint32_t button_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    p_button->p_port = p_port;
    p_button->pin = pin;
}

/**
 * @brief Get the value of the GPIO connected to the button.
 * 
 * @param button_id Button ID. This index is used to select the element of the buttons_arr[] array
 */
bool port_button_get_value (uint32_t button_id)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    return stm32f4_system_gpio_read(p_button->p_port, p_button->pin);
}

/**
 * @brief Return the status of the button (pressed or not).
 * 
 * @param button_id Button ID. This index is used to get the correct button status struct.
 * @return true If the button has been pressed
 * @return false If the button has not been pressed
 */
bool port_button_get_pressed (uint32_t button_id)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    return p_button ->flag_pressed;
}

/**
 * @brief Get the status of the interrupt line connected to the button.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 * @return true 
 * @return false 
 */
bool port_button_get_pending_interrupt	(uint32_t button_id)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    return (bool) ((EXTI -> PR) & (BIT_POS_TO_MASK(p_button -> pin)));
}

/**
 * @brief Clear the pending interrupt of the button.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttstons_arr[] array.
 */
void port_button_clear_pending_interrupt ( uint32_t button_id)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    EXTI -> PR |= BIT_POS_TO_MASK(p_button -> pin);
    
}

/**
 * @brief Set the status of the button (pressed or not). This function is used to force the status of the button. It is used to simulate the button press in the tests.
 * 
 * @param button_id Button ID. This index is used to get the correct button struct of the buttons_arr[] array.
 * @param pressed   Status of the button.
 */
void port_button_set_pressed (uint32_t button_id, bool pressed)	
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    p_button -> flag_pressed = pressed;
}

/**
 * @brief Disable the interrupts of the button.
 *          This function is used to disable the interrupts of the button. It is used in the unit tests to avoid unwanted interrupts.
 * 
 * @param button_id     Button ID. This index is used to get the correct button struct of the buttons_arr[] array.

 */
void port_button_disable_interrupts	(uint32_t button_id)
{
    stm32f4_button_hw_t *p_button = _stm32f4_button_get(button_id);
    stm32f4_system_gpio_exti_disable((p_button -> pin));
}
