/**
 * @file stm32f4_button.c
 * @brief Portable functions to interact with the button FSM library. All portable functions must be implemented in this file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date 15/04/2025
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>


/* HW dependent includes */
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_urbanite.h"


/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure to define the Urbanite FSM.
 * 
 */
struct fsm_urbanite_t
{
    fsm_t f;                                    /*!<    Urbanite FSM*/
    fsm_button_t * p_fsm_button;                /*!<    Pointer to the button FSM*/
    uint32_t on_off_press_time_ms;              /*!<    Time in ms to consider ON/OFF*/
    uint32_t pause_display_time_ms;             /*!<    Time in ms to pause the display*/
    uint32_t emergency_time_ms;                 /*!<    Time in ms to activate emergency*/
    bool is_paused;                             /*!<    Flag to indicate if the system is paused*/
    bool emergency_aux;                         /*!<    To indicate the phase of the emergency mode*/
    bool emergency;                             /*!<    To indicate the emergency state*/
    fsm_ultrasound_t * p_fsm_ultrasound_rear;   /*!<    Pointer to the ultrasound FSM*/
    fsm_display_t * p_fsm_display_rear;         /*!<    Pointer to the display FSM*/
};

/* Private functions -----------------------------------------------------------*/
/* State machine input or transition functions */
/**
 * @brief Check if the button has been pressed for the required time to turn ON the Urbanite system.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true     if the duration is greater than 0 and greater than the required time to turn ON the system.
 * @return false 
 */
static bool check_on(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    uint32_t duration = fsm_button_get_duration(p_fsm -> p_fsm_button);
    return ((duration > 0)&&(duration > p_fsm -> on_off_press_time_ms));
}

/**
 * @brief Check if the button has been pressed for the required time to turn OFF the system.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_off(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    uint32_t duration = fsm_button_get_duration(p_fsm -> p_fsm_button);
    return ((duration > 0)&&(duration > p_fsm -> on_off_press_time_ms)&&(duration < p_fsm -> emergency_time_ms));
}

/**
 * @brief Check if the button has been pressed for the required time to turn ON emergency mode.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_emergency_on(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    uint32_t duration = fsm_button_get_duration(p_fsm -> p_fsm_button);
    return ((duration > 0)&&(duration > p_fsm -> emergency_time_ms));
}

/**
 * @brief Check if the emergency mode must continue to be active
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_emergency_continue(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    return p_fsm -> emergency;
}

/**
 * @brief Check if the button has been pressed for the required time to turn OFF emergency mode.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_emergency_off(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    uint32_t duration = fsm_button_get_duration(p_fsm -> p_fsm_button);
    return ((duration > 0)&&(duration > p_fsm -> emergency_time_ms));
}

/**
 * @brief Check if a new measurement is ready.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_new_measure(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    return fsm_ultrasound_get_new_measurement_ready(p_fsm -> p_fsm_ultrasound_rear);
}

/**
 * @brief Check if it has been required to pause the display.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true     If the duration is greater than 0, less than the required time to turn ON the system, and greater than the required time to pause the display.
 * @return false 
 */
static bool check_pause_display(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    uint32_t duration = fsm_button_get_duration(p_fsm -> p_fsm_button);
    return ((duration > 0)&&(duration < p_fsm -> on_off_press_time_ms)&&(duration >= p_fsm -> pause_display_time_ms));
}


/**
 * @brief Check if any of the elements of the system is active.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true     If any of the elements (button, ultrasound, or display) is active.
 * @return false 
 */
static bool check_activity(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    return (fsm_ultrasound_check_activity(p_fsm -> p_fsm_ultrasound_rear)||fsm_display_check_activity(p_fsm -> p_fsm_display_rear)||fsm_button_check_activity(p_fsm -> p_fsm_button));
}

/**
 * @brief Check if all the elements of the system are inactive.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_urbanite_t.
 * @return true     If all the elements of the system are inactive.
 * @return false 
 */
static bool check_no_activity(fsm_t * p_this)
{
    return !check_activity(p_this);
}

/**
 * @brief Check if any a new measurement is ready while the system is in low power mode.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 * @return true 
 * @return false 
 */
static bool check_activity_in_measure(fsm_t * p_this)
{
    return check_new_measure(p_this);
}


/* State machine output or action functions */
/**
 * @brief Turn the Urbanite system ON.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t
 */
static void do_start_up_measure(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    fsm_button_reset_duration(p_fsm -> p_fsm_button);

    // Start the ultrasound sensor
    fsm_ultrasound_start(p_fsm -> p_fsm_ultrasound_rear);

    // Set Display status tu active
    fsm_display_set_status(p_fsm -> p_fsm_display_rear, true);

    printf("[URBANITE][%ld] Urbanite system ON\n", port_system_get_millis());   // DEBUG
}

/**
 * @brief Turn the Urbanite system OFF.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_stop_urbanite(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);

    fsm_button_reset_duration(p_fsm -> p_fsm_button);
    fsm_ultrasound_stop(p_fsm -> p_fsm_ultrasound_rear);
    fsm_display_set_status(p_fsm -> p_fsm_display_rear, false);

    p_fsm -> is_paused = false;     // Remove pause status

    printf("[URBANITE][%ld] Urbanite system OFF\n", port_system_get_millis());  // DEBUG
}	

/**
 * @brief Pause or resume the display system.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_pause_display(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);

    fsm_button_reset_duration(p_fsm -> p_fsm_button);

    // Invert the pause status and activate or deactivate the display depending on the new pause status.
    p_fsm -> is_paused = !(p_fsm -> is_paused);
    fsm_display_set_status(p_fsm -> p_fsm_display_rear, !(p_fsm -> is_paused));

    // DEBUG
    if(p_fsm -> is_paused)
    {
    printf("[URBANITE][%ld] Urbanite system display PAUSE\n", port_system_get_millis());
    }
    else
    {
    printf("[URBANITE][%ld] Urbanite system display RESUME\n", port_system_get_millis());
    }  
}

/**
 * @brief Display the distance measured by the ultrasound sensor.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_display_distance(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    
    uint32_t distance_cm = fsm_ultrasound_get_distance(p_fsm -> p_fsm_ultrasound_rear);

    if(p_fsm -> is_paused)
    {
        if(distance_cm < (WARNING_MIN_CM / 2))
        {
            fsm_display_set_distance(p_fsm -> p_fsm_display_rear, distance_cm);
            fsm_display_set_status(p_fsm -> p_fsm_display_rear, true);
        }
        else
        {
            fsm_display_set_status(p_fsm -> p_fsm_display_rear, false);
        }
    }
    else 
    {
        fsm_display_set_distance(p_fsm -> p_fsm_display_rear, distance_cm);
    }

    printf("[URBANITE][%ld] Distance: %ld cm\n", port_system_get_millis(), distance_cm);    // DEBUG
}

/**
 * @brief Start the emergency mode.
 * 
 * @param p_this p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_start_emergency(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);

    fsm_button_reset_duration(p_fsm -> p_fsm_button);

    fsm_display_set_status(p_fsm -> p_fsm_display_rear, true);
    fsm_ultrasound_stop(p_fsm -> p_fsm_ultrasound_rear);

    p_fsm -> emergency_aux = true;
    p_fsm -> emergency = true;

    printf("[URBANITE][%ld] Urbanite system EMERGENCY is ON\n", port_system_get_millis());
}

/**
 * @brief Stop the emergency mode.
 * 
 * @param p_this p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_stop_emergency(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);

    fsm_button_reset_duration(p_fsm -> p_fsm_button);
    fsm_ultrasound_start(p_fsm -> p_fsm_ultrasound_rear);

    // Deactivate the display if it was paused
    if(p_fsm -> is_paused)
    {
        fsm_display_set_status(p_fsm -> p_fsm_display_rear, false);
    }

    p_fsm -> emergency_aux = false;
    p_fsm -> emergency = false;

    printf("[URBANITE][%ld] Urbanite system EMERGENCY is OFF\n", port_system_get_millis());
}

/**
 * @brief Work in the emergency mode.
 * 
 * @param p_this p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_continue_emergency(fsm_t * p_this)
{
    fsm_urbanite_t *p_fsm = (fsm_urbanite_t *)(p_this);
    
    if(p_fsm -> emergency_aux)
    {
        fsm_display_set_distance(p_fsm -> p_fsm_display_rear, 0);

        port_system_delay_ms(1000);         // Waits 1s
        p_fsm -> emergency_aux = false;
    } 
    else 
    {
        fsm_display_set_distance(p_fsm -> p_fsm_display_rear, 500);

        port_system_delay_ms(1000);;        // Waits 1s
        p_fsm -> emergency_aux = true;
    }

}

/**
 * @brief Start the low power mode while the Urbanite is OFF.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_sleep_off(fsm_t * p_this)
{
    port_system_sleep();
}	

/**
 * @brief Start the low power mode while the Urbanite is measuring the distance and it is waiting for a new measurement.
 * 
 * @param p_this Call function port_system_sleep() to start the low power mode
 */
static void do_sleep_while_measure(fsm_t * p_this)
{
    port_system_sleep();
}	

/**
 * @brief Start the low power mode while the Urbanite is awakened by a debug breakpoint or similar in the SLEEP_WHILE_OFF state.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_sleep_while_off(fsm_t * p_this)
{
    port_system_sleep();
}

/**
 * @brief Start the low power mode while the Urbanite is awakened by a debug breakpoint or similar in the SLEEP_WHILE_ON state.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_urbanite_t.
 */
static void do_sleep_while_on	(fsm_t * p_this)	
{
    port_system_sleep();
}


/**
 * @brief Array representing the transitions table of the FSM Urbanite.
 * 
 */
static fsm_trans_t fsm_trans_urbanite[] = {
    {OFF, check_no_activity, SLEEP_WHILE_OFF, do_sleep_off},
    {SLEEP_WHILE_OFF, check_activity, OFF, NULL},
    {SLEEP_WHILE_OFF, check_no_activity, SLEEP_WHILE_OFF, do_sleep_while_off},
    
    {OFF, check_on, MEASURE, do_start_up_measure},
    {MEASURE, check_pause_display, MEASURE, do_pause_display},
    {MEASURE, check_new_measure, MEASURE, do_display_distance},

    {MEASURE, check_no_activity, SLEEP_WHILE_ON, do_sleep_while_measure},
    {SLEEP_WHILE_ON, check_activity_in_measure, MEASURE, NULL},
    {SLEEP_WHILE_ON, check_no_activity, SLEEP_WHILE_ON, do_sleep_while_on},

    {MEASURE, check_emergency_on, EMERGENCY, do_start_emergency},
    {EMERGENCY, check_emergency_off, MEASURE, do_stop_emergency},
    {EMERGENCY, check_emergency_continue, EMERGENCY, do_continue_emergency},
    

    {MEASURE, check_off, OFF, do_stop_urbanite},
    {-1, NULL, -1, NULL}
};


/* Other auxiliary functions */
/**
 * @brief This function initializes the default values of the FSM struct and calls to the port to initialize the HWs associated to the devices.
 * 
 * @param p_fsm_urbanite        Pointer to the Urbanite FSM.
 * @param p_fsm_button          Pointer to the button FSM that activates the system and disables the display if it disturbs the driver.
 * @param on_off_press_time_ms  Button press time in milliseconds to turn the system ON or OFF.
 * @param pause_display_time_ms Time in milliseconds to pause the display after a short press of the button.
 * @param emergency_time_ms     Time in milliseconds to activate emergency mode.
 * @param p_fsm_ultrasound_rear Pointer to the ultrasound FSM that measures the distance to the rear obstacle.
 * @param p_fsm_display_rear    Pointer to the display FSM that shows the distance to the rear obstacle.
 */
static void fsm_urbanite_init(fsm_urbanite_t * p_fsm_urbanite, fsm_button_t * p_fsm_button,  uint32_t on_off_press_time_ms, uint32_t pause_display_time_ms, uint32_t emergency_time_ms, fsm_ultrasound_t * p_fsm_ultrasound_rear, fsm_display_t * p_fsm_display_rear)
{
    // Initialize the FSM
    fsm_init(&p_fsm_urbanite-> f, fsm_trans_urbanite);

    // Initialize the fields of the FSM structure with the received parameters
    // Pointers
    p_fsm_urbanite -> p_fsm_button = p_fsm_button;
    p_fsm_urbanite -> p_fsm_ultrasound_rear = p_fsm_ultrasound_rear;
    p_fsm_urbanite -> p_fsm_display_rear = p_fsm_display_rear;

    // Times
    p_fsm_urbanite -> on_off_press_time_ms = on_off_press_time_ms;
    p_fsm_urbanite -> pause_display_time_ms = pause_display_time_ms;
    p_fsm_urbanite -> emergency_time_ms = emergency_time_ms;
    
    // Initialize the field is_paused to false.
    p_fsm_urbanite -> is_paused = false;
    p_fsm_urbanite -> emergency_aux = false;
    p_fsm_urbanite -> emergency = false;
}


/* Public functions -----------------------------------------------------------*/
fsm_urbanite_t* fsm_urbanite_new(fsm_button_t * p_fsm_button, uint32_t on_off_press_time_ms, uint32_t pause_display_time_ms, uint32_t emergency_time_ms, fsm_ultrasound_t * p_fsm_ultrasound_rear, fsm_display_t * p_fsm_display_rear)
{
    fsm_urbanite_t *p_fsm_urbanite = malloc(sizeof(fsm_urbanite_t));        /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_urbanite_init(p_fsm_urbanite, p_fsm_button, on_off_press_time_ms, pause_display_time_ms, emergency_time_ms, p_fsm_ultrasound_rear, p_fsm_display_rear);    /* Initialize the FSM */
    return p_fsm_urbanite;
}

void fsm_urbanite_fire(fsm_urbanite_t * p_fsm)
{
    fsm_fire(&p_fsm->f); 
}

void fsm_urbanite_destroy(fsm_urbanite_t * p_fsm)	
{
    free(&p_fsm->f);
}
