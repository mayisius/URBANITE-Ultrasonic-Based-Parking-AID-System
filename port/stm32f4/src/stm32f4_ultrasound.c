/**
 * @file stm32f4_ultrasound.c
 * @brief Portable functions to interact with the ultrasound FSM library. All portable functions must be implemented in this file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date date
 */

/* Standard C includes */
#include <stdio.h>
#include <math.h>


/* HW dependent includes */
#include "stm32f4_system.h"
#include "stm32f4_ultrasound.h"

/* Microcontroller dependent includes */
#include "port_ultrasound.h"
#include "port_system.h"

/* Typedefs --------------------------------------------------------------------*/
typedef struct
{
    GPIO_TypeDef* p_echo_port;      /*!<    GPIO where the echo signal is connected   */
    GPIO_TypeDef* p_trigger_port;   /*!<    GPIO where the trigger signal is connected   */
    uint8_t trigger_pin;            /*!<    Pin/line where the trigger signal is connected   */
    uint8_t echo_alt_fun;           /*!<    Alternate function for the echo signal   */
    uint8_t echo_pin;               /*!<    Pin/line where the echo signal is connected   */
    bool echo_received;             /*!<    Flag to indicate that the echo signal has been received     false at the begining   */
    bool trigger_end;               /*!<    Flag to indicate that the trigger signal has been sent     false at the begining   */
    bool trigger_ready;             /*!<    Flag to indicate that a new measurement can be started     true at the begining   */
    uint32_t echo_end_tick;         /*!<    Tick time when the echo signal was received      0 at the begining  */
    uint32_t echo_init_tick;        /*!<    Tick time when the echo signal was received     0 at the begining   */
    uint32_t echo_overflows;        /*!<    Number of overflows of the timer during the echo signal     0 at the begining   */
}stm32f4_ultrasound_hw_t;

/* Global variables */
/**
 * @brief Array of elements that represents the HW characteristics of the ultrasounds connected to the STM32F4 platform.
 * This must be hidden from the user, so it is declared as static.
 * To access the elements of this array, use the function _stm32f4_ultrasound_get().
 */
static stm32f4_ultrasound_hw_t ultrasounds_arr[] = {
    [PORT_REAR_PARKING_SENSOR_ID] = {.p_echo_port = STM32F4_REAR_PARKING_SENSOR_ECHO_GPIO, .p_trigger_port = STM32F4_REAR_PARKING_SENSOR_TRIGGER_GPIO, .trigger_pin = STM32F4_REAR_PARKING_SENSOR_TRIGGER_PIN,  .echo_pin = STM32F4_REAR_PARKING_SENSOR_ECHO_PIN},
};


/* Private functions ----------------------------------------------------------*/

/**
 * @brief Get the ultrasound struct with the given ID.
 * 
 * @param ultrasound_id                 Ultrasound ID. This index is used to select the element of the ultrasound_arr[] array.
 * @return stm32f4_ultrasound_hw_t*     Pointer to the ultrasound struct. NULL If the ultrasound ID is not valid.
 */
stm32f4_ultrasound_hw_t* _stm32f4_ultrasound_get (uint32_t ultrasound_id)
{
    // Return the pointer to the button with the given ID. If the ID is not valid, return NULL.
    if (ultrasound_id < sizeof(ultrasounds_arr) / sizeof(ultrasounds_arr[0]))
    {
        return &ultrasounds_arr[ultrasound_id];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Configure the timer that controls the duration of the trigger signal.
 * 
 */
static void _timer_trigger_setup(void)
{
    // Enable the clock of the timer that controls the trigger signal.
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  

    // Disable the counter of the timer.
    TIM3->CR1 &= ~TIM_CR1_CEN;

    // Enable the autoreload preload
    TIM3->CR1 |= TIM_CR1_ARPE;

    // Set the counter of the timer to 0
    TIM3-> CNT = 0;

    // Compute the prescaler and the auto-reload register to set the duration of the trigger signal.
    double sysclk_as_double = (double)SystemCoreClock; 
    double trigger_us_as_double = (double)PORT_PARKING_SENSOR_TRIGGER_UP_US; 
 
    double psc_temp = round((sysclk_as_double / (1000000.0 * 65535.0)) - 1.0);
 
    double arr_temp = round((sysclk_as_double / ((psc_temp + 1.0) * 1000000.0)) * trigger_us_as_double);
 
    if (arr_temp > 65535.0) {
        psc_temp += 1.0;
        arr_temp = round((sysclk_as_double / ((psc_temp + 1.0) * 1000000.0)) * trigger_us_as_double);
    }
 
    // Load the values computed for ARR and PSC into the corresponding registers of the timer.
    TIM3->PSC = (uint32_t)psc_temp;
    TIM3->ARR = (uint32_t)arr_temp;
 
    TIM3->EGR |= TIM_EGR_UG;
 
    // Clear the update interrupt flag.
    TIM3->SR &= ~TIM_SR_UIF;
 
    // Enable the interrupts of the timer by setting the UIE bit of the DIER register.
    TIM3->DIER |= TIM_DIER_UIE;
 
    // Set the priority of the timer interrupt in the NVIC.
    NVIC_SetPriority(TIM3_IRQn, 4);
 
}

/**
 * @brief Configure the timer that controls the duration of the new measurement.
 * 
 */
void _timer_new_measurement_setup(void)
{
    // Enable the clock of the timer that controls the trigger signal.
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  

    // Disable the counter of the timer.
    TIM5->CR1 &= ~TIM_CR1_CEN;

    // Enable the autoreload preload
    TIM5->CR1 |= TIM_CR1_ARPE;

    // Set the counter of the timer to 0
    TIM5->CNT = 0;

    // Compute the prescaler and the auto-reload register to set the duration of the trigger signal.
    double sysclk_as_double = (double)SystemCoreClock; 
    double timeout_ms_d = (double)PORT_PARKING_SENSOR_TIMEOUT_MS;
 
    double psc_temp = round((sysclk_as_double / (1000.0 * 65535.0)) - 1.0);
 
    double arr_temp = round(timeout_ms_d * (sysclk_as_double / 1000.0) / (psc_temp + 1.0));
 
    if (arr_temp > 65535.0) {
        psc_temp += 1.0;
        arr_temp = round(timeout_ms_d * (sysclk_as_double / 1000.0) / (psc_temp + 1.0));
    }
    // Load the values computed for ARR and PSC into the corresponding registers of the timer.
    TIM5->PSC = (uint32_t)psc_temp;
    TIM5->ARR = (uint32_t)arr_temp;
 
    TIM5->EGR |= TIM_EGR_UG;
 
    // Clear the update interrupt flag.
    TIM5->SR &= ~TIM_SR_UIF;
 
    // Enable the interrupts of the timer by setting the UIE bit of the DIER register.
    TIM5->DIER |= TIM_DIER_UIE;
 
    // Set the priority of the timer interrupt in the NVIC.
    NVIC_SetPriority(TIM5_IRQn, 5);
}

/**
 * @brief Configure the timer that controls the duration of the echo signal.
 * 
 * @param ultrasound_id     Ultrasound ID. This ID is used to configure the timer that controls the echo signal of the ultrasound sensor.
 */
static void _timer_echo_setup(uint32_t ultrasound_id)
{   
    
    TIM_TypeDef *TIMx;          // Timer of the ultrasound
    uint32_t TIMx_IRQn;     // Timer IRQn of the ultrasound

    // Select the timer based on the ID
    switch (ultrasound_id) {
        case PORT_REAR_PARKING_SENSOR_ID:
            TIMx = TIM2;  
            TIMx_IRQn = TIM2_IRQn;

            break;
        default:
            return;  
    }
    
    // Enable the clock of the timer that controls the trigger signal.
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIMx->CR1 &= ~TIM_CR1_CEN;

    // Set the values of the prescaler and the auto-reload registers.
    TIMx->PSC = (SystemCoreClock / 1000000) - 1;  // Convert to 1MHz
    TIMx->ARR = 65535;                            // MAX value
    
    // Set the auto-reload preload bit (ARPE) in the control register (CR1) to enable the auto-reload register. 
    TIMx->CR1 |= TIM_CR1_ARPE;  
    TIMx->EGR |= TIM_EGR_UG;  

    // Set the direction as input in the Capture/Compare mode register (CCMRx) for CH2.
    if (ultrasound_id == PORT_REAR_PARKING_SENSOR_ID) {
        TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;  
        TIM2->CCMR1 |= TIM_CCMR1_CC2S_0; 
    }

    TIMx->CCMR1 &= ~TIM_CCMR1_IC2F;     // Disable digital filtering by clearing the ICxF bits in the Capture/Compare mode register (CCMRx).

    // Select the edge of the active transition in the Capture/Compare enable register (CCER) for CH2.
    TIMx->CCER |= TIM_CCER_CC2P;  
    TIMx->CCER |= TIM_CCER_CC2NP;

    TIMx->CCMR1 &= ~TIM_CCMR1_IC2PSC;   // Program the input prescaler to capture each valid transition.

    TIMx->CCER |= TIM_CCER_CC2E;    // Enable the Capture/compare enable register (CCER) for the corresponding channel.
    TIMx->DIER |= TIM_DIER_CC2IE;   // Enable the Capture/Compare interrupts bit (CCxIE) for the corresponding channel in the DMA/interrupt enable register (DIER).
    TIMx->DIER |= TIM_DIER_UIE;     // Enable the update interrupt bit (UIE) in the DMA/interrupt enable register (DIER).

    // Set the priority of the timer interrupt in the NVIC.
    NVIC_SetPriority(TIMx_IRQn, 3);
}


/* Public functions -----------------------------------------------------------*/
void port_ultrasound_init(uint32_t ultrasound_id)
{
    /* Get the ultrasound sensor */
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    /* TO-DO alumnos: */

    /* Trigger pin configuration */
    p_ultrasound -> trigger_ready = true;
    p_ultrasound -> trigger_end = false;

    /* Echo pin configuration */
    p_ultrasound -> echo_received = false;

    p_ultrasound -> echo_end_tick = 0;
    p_ultrasound -> echo_init_tick = 0;
    p_ultrasound -> echo_overflows = 0;

    /* Configure timers */
    stm32f4_system_gpio_config(p_ultrasound-> p_trigger_port, p_ultrasound->trigger_pin, STM32F4_GPIO_MODE_OUT, STM32F4_GPIO_PUPDR_NOPULL);

    stm32f4_system_gpio_config(p_ultrasound->p_echo_port, p_ultrasound->echo_pin, STM32F4_GPIO_MODE_AF, STM32F4_GPIO_PUPDR_NOPULL);
    stm32f4_system_gpio_config_alternate(p_ultrasound->p_echo_port, p_ultrasound->echo_pin, STM32F4_AF1);

    _timer_trigger_setup();
    _timer_echo_setup(ultrasound_id);
    _timer_new_measurement_setup();
}


void port_ultrasound_stop_trigger_timer(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    stm32f4_system_gpio_write(p_ultrasound -> p_trigger_port, p_ultrasound -> trigger_pin, 0);

    TIM3 -> CR1 &= ~TIM_CR1_CEN; 
}

void port_ultrasound_stop_echo_timer(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    stm32f4_system_gpio_write(p_ultrasound -> p_echo_port, p_ultrasound -> echo_pin, 0);

    TIM2 -> CR1 &= ~TIM_CR1_CEN;
}

void port_ultrasound_reset_echo_ticks(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    p_ultrasound -> echo_received = false;

    p_ultrasound -> echo_end_tick = 0;
    p_ultrasound -> echo_init_tick = 0;
    p_ultrasound -> echo_overflows = 0;
}


void port_ultrasound_start_measurement(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);

    p_ultrasound -> trigger_ready = false;
    TIM5 -> CNT = 0;

    if(ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
    {
        TIM2 -> CNT = 0;
        TIM3 -> CNT = 0;
    }

    stm32f4_system_gpio_write(p_ultrasound -> p_trigger_port, p_ultrasound -> trigger_pin, HIGH);
    NVIC_EnableIRQ(TIM5_IRQn);
    TIM5 -> CR1 |= TIM_CR1_CEN;

    if(ultrasound_id == PORT_REAR_PARKING_SENSOR_ID)
    {
        NVIC_EnableIRQ(TIM2_IRQn);
        NVIC_EnableIRQ(TIM3_IRQn);

        TIM2 -> CR1 |= TIM_CR1_CEN;
        TIM3 -> CR1 |= TIM_CR1_CEN;
    }
}

void port_ultrasound_start_new_measurement_timer(void)
{
    NVIC_EnableIRQ(TIM5_IRQn);
    TIM5 -> CR1 |= TIM_CR1_CEN;
}

void port_ultrasound_stop_new_measurement_timer(void)
{   
    TIM5 -> CR1 &= ~TIM_CR1_CEN;
}

void port_ultrasound_stop_ultrasound(uint32_t ultrasound_id)
{
    port_ultrasound_stop_trigger_timer(ultrasound_id);
    port_ultrasound_stop_echo_timer(ultrasound_id);
    port_ultrasound_stop_new_measurement_timer();

    port_ultrasound_reset_echo_ticks(ultrasound_id);
}	


// Getters and setters functions
bool port_ultrasound_get_trigger_ready(uint32_t ultrasound_id) {
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> trigger_ready;
}

void port_ultrasound_set_trigger_ready(uint32_t ultrasound_id, bool trigger_ready) {
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound -> trigger_ready = trigger_ready;
}

bool port_ultrasound_get_trigger_end(uint32_t ultrasound_id) {
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> trigger_end;
}

void port_ultrasound_set_trigger_end(uint32_t ultrasound_id, bool trigger_end) {
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound -> trigger_end = trigger_end;
}


uint32_t port_ultrasound_get_echo_end_tick(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> echo_end_tick;
}

void port_ultrasound_set_echo_end_tick(uint32_t ultrasound_id, uint32_t echo_end_tick)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound -> echo_end_tick = echo_end_tick;
}	

uint32_t port_ultrasound_get_echo_init_tick	(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> echo_init_tick;
}

void port_ultrasound_set_echo_init_tick(uint32_t ultrasound_id, uint32_t echo_init_tick)	
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
     p_ultrasound -> echo_init_tick = echo_init_tick;
}

uint32_t port_ultrasound_get_echo_overflows	(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> echo_overflows;
}

void port_ultrasound_set_echo_overflows(uint32_t ultrasound_id, uint32_t echo_overflows)	
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound -> echo_overflows = echo_overflows;
}

bool port_ultrasound_get_echo_received(uint32_t ultrasound_id)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    return p_ultrasound -> echo_received;
}

void port_ultrasound_set_echo_received(uint32_t ultrasound_id, bool echo_received)	
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound -> echo_received = echo_received;
}


// Util
void stm32f4_ultrasound_set_new_trigger_gpio(uint32_t ultrasound_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound->p_trigger_port = p_port;
    p_ultrasound->trigger_pin = pin;
}

void stm32f4_ultrasound_set_new_echo_gpio(uint32_t ultrasound_id, GPIO_TypeDef *p_port, uint8_t pin)
{
    stm32f4_ultrasound_hw_t *p_ultrasound = _stm32f4_ultrasound_get(ultrasound_id);
    p_ultrasound->p_echo_port = p_port;
    p_ultrasound->echo_pin = pin;
}
