/**
 * @file main.c
 * @brief Main file.
 * @author Sistemas Digitales II
 * @date 2025-01-01
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* HW libraries */
#include "port_system.h"
#include "port_button.h"
#include "port_ultrasound.h"
#include "port_display.h"

#include "fsm.h"
#include "fsm_button.h"
#include "fsm_ultrasound.h"
#include "fsm_display.h"
#include "fsm_urbanite.h"

/* Defines ------------------------------------------------------------------*/
#define URBANITE_ON_OFF_PRESS_TIME_MS 1000  // Time in ms to activate the Urbanite system, started mainly due to a parking maneuver (long press) (1 s)
#define URBANITE_PAUSE_DISPLAY_TIME_MS 250  // Time in ms to pause the display (0,25 s)
#define URBANITE_EMERGENCY_TIME_MS 3000     // Time in ms to activate emergency mode (5 s)


/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* Init board */
    port_system_init();

    
    // Create the state machines.
    fsm_button_t* p_fsm_button = fsm_button_new(PORT_PARKING_BUTTON_DEBOUNCE_TIME_MS, PORT_PARKING_BUTTON_ID);  //es porque el PORT_PARKING_BUTTON_DEBOUNCE_TIME_MS no tiene valor
    fsm_ultrasound_t* p_fsm_ultrasound_rear = fsm_ultrasound_new(PORT_REAR_PARKING_SENSOR_ID);
    fsm_display_t* p_fsm_display_rear = fsm_display_new(PORT_REAR_PARKING_DISPLAY_ID);

    fsm_urbanite_t*  p_fsm_urbanite = fsm_urbanite_new(p_fsm_button,URBANITE_ON_OFF_PRESS_TIME_MS, URBANITE_PAUSE_DISPLAY_TIME_MS, URBANITE_EMERGENCY_TIME_MS, p_fsm_ultrasound_rear, p_fsm_display_rear);

    /* Infinite loop */
    while (1)
    {        
        fsm_button_fire(p_fsm_button);
        fsm_ultrasound_fire(p_fsm_ultrasound_rear);
        fsm_display_fire(p_fsm_display_rear);
        fsm_urbanite_fire(p_fsm_urbanite);
        
    } // End of while(1)

    
    // Free the memory
    fsm_button_destroy(p_fsm_button);
    fsm_ultrasound_destroy(p_fsm_ultrasound_rear);
    fsm_display_destroy(p_fsm_display_rear);
    fsm_urbanite_destroy(p_fsm_urbanite);
    

    return 0;
}
