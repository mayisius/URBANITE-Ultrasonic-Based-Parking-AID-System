/**
 * @file fsm_ultrasound.c
 * @brief Ultrasound sensor FSM main file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <string.h>

/* HW dependent includes */
#include "port_ultrasound.h"
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_ultrasound.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure of the Ultrasaund FSM.
 * 
 */
struct fsm_ultrasound_t
{
    fsm_t f;                    /*!<Ultrasound FSM*/
    uint32_t distance_cm;       /*!<How much time the ultrasound has been pressed*/
    bool status;                /*!<Indicate if the ultrasound sensor is active or not*/
    bool new_measurement;       /*!<Flag to indicate if a new measurement has been completed*/
    uint32_t ultrasound_id;     /*!<Ultrasound ID. Must be unique*/
    uint32_t distance_arr [FSM_ULTRASOUND_NUM_MEASUREMENTS];    /*!<Array to store the last distance measurements*/
    uint32_t distance_idx;      /*!<Index to store the last distance measurement*/
};

/* Private functions -----------------------------------------------------------*/
// Comparison function for qsort
int _compare(const void *a, const void *b)
{
    return (*(uint32_t *)a - *(uint32_t *)b);
}


/* State machine input or transition functions */
/**
 * @brief Check if the ultrasound sensor is active and ready to start a new measurement.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_ultrasound_t.
 * @return true 
 * @return false 
 */
static bool check_on(fsm_t *p_this) {
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return (port_ultrasound_get_trigger_ready(p_fsm -> ultrasound_id)&&(p_fsm -> status));
}

/**
 * @brief Check if the ultrasound sensor has been set to be inactive (OFF).
 * 
 * @param p_this  Pointer to an fsm_t struct that contains an fsm_ultrasound_t.
 * @return true 
 * @return false 
 */
static bool check_off(fsm_t * p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return !(p_fsm -> status);
}	

/**
 * @brief Check if the ultrasound sensor has finished the trigger signal.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_ultrasound_t
 * @return true 
 * @return false 
 */
static bool check_trigger_end(fsm_t * p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return port_ultrasound_get_trigger_end(p_fsm -> ultrasound_id);
}

/**
 * @brief Check if the ultrasound sensor has received the init (rising edge in the input capture) of the echo signal.
 * 
 * @param p_this    Pointer to an fsm_t struct that contains an fsm_ultrasound_t.
 * @return true 
 * @return false 
 */
static bool check_echo_init(fsm_t *p_this) {
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return port_ultrasound_get_echo_init_tick(p_fsm -> ultrasound_id) > 0;
}

/**
 * @brief Check if the ultrasound sensor has received the end (falling edge in the input capture) of the echo signal.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_ultrasound_t.
 * @return true 
 * @return false 
 */
static bool check_echo_received(fsm_t *p_this) {
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return port_ultrasound_get_echo_received(p_fsm -> ultrasound_id);
}

/**
 * @brief Check if a new measurement is ready.
 * 
 * @param p_this Pointer to an fsm_t struct that contains an fsm_ultrasound_t.
 * @return true 
 * @return false 
 */
static bool check_new_measurement(fsm_t *p_this) {
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    return port_ultrasound_get_trigger_ready(p_fsm -> ultrasound_id);
}


/* State machine output or action functions */
/**
 * @brief Start a measurement of the ultrasound transceiver for the first time after the FSM is started.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an fsm_ultrasound_t.
 */
static void do_start_measurement(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    port_ultrasound_start_measurement(p_fsm -> ultrasound_id);
}

/**
 * @brief Stop the ultrasound sensor.
 * This function is called when the ultrasound sensor is stopped.
 * It stops the ultrasound sensor and resets the echo ticks.
 * 
 * @param p_this  Pointer to an fsm_t struct than contains an fsm_ultrasound_t.
 */
static void do_stop_measurement	(fsm_t * p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    port_ultrasound_stop_ultrasound(p_fsm -> ultrasound_id);
}	

/**
 * @brief Stop the trigger signal of the ultrasound sensor.
 * This function is called when the time to trigger the ultrasound sensor has finished.
 * It stops the trigger signal and the trigger timer
 * 
 * @param p_this Pointer to an fsm_t struct than contains an fsm_ultrasound_t.
 */
static void do_stop_trigger(fsm_t *p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);
    port_ultrasound_stop_trigger_timer(p_fsm->ultrasound_id);
    port_ultrasound_set_trigger_end(p_fsm->ultrasound_id, false);
}

/**
 * @brief Start a new measurement of the ultrasound transceiver.
 * This function is called when the ultrasound sensor has finished a measurement and is ready to start a new one.
 * 
 * @param p_this 	Pointer to an fsm_t struct than contains an fsm_ultrasound_t.
 */
static void do_start_new_measurement(fsm_t *p_this)
{
    do_start_measurement(p_this);
}


/**
 * @brief Set the distance measured by the ultrasound sensor.
 * This function is called when the ultrasound sensor has received the echo signal.
 * It calculates the distance in cm and stores it in the array of distances.
 * When the array is full, it computes the median of the array and resets the index of the array.
 * 
 * @param p_this Pointer to an fsm_t struct than contains an fsm_ultrasound_t.
 */
static void do_set_distance(fsm_t * p_this)
{
    fsm_ultrasound_t *p_fsm = (fsm_ultrasound_t *)(p_this);

    uint32_t end_tick = port_ultrasound_get_echo_end_tick(p_fsm -> ultrasound_id);
    uint32_t init_tick = port_ultrasound_get_echo_init_tick(p_fsm -> ultrasound_id);
    uint32_t overflows = port_ultrasound_get_echo_overflows(p_fsm -> ultrasound_id);

    uint32_t ticks_elapsed;
    if (end_tick >= init_tick)
    {
        ticks_elapsed = end_tick - init_tick;
    }
    else
    {
        ticks_elapsed = (65536 - init_tick) + end_tick; 
        if(overflows > 0)
        {
        overflows = overflows - 1;
        }
    }

    ticks_elapsed = ticks_elapsed + (overflows * 65536);                        // 1 tick = 1us
    uint32_t distance = (uint32_t)(((uint64_t)ticks_elapsed * 10) / 583);       // Taking into account the speed of sound (1cm = 58.3us)
    
    p_fsm -> distance_arr[p_fsm -> distance_idx] = distance;

    p_fsm -> distance_idx++;
    if (p_fsm -> distance_idx >= FSM_ULTRASOUND_NUM_MEASUREMENTS) {
        p_fsm -> distance_idx = 0;

        qsort(p_fsm -> distance_arr, FSM_ULTRASOUND_NUM_MEASUREMENTS, sizeof(uint32_t), _compare);

        if (FSM_ULTRASOUND_NUM_MEASUREMENTS % 2 == 0)
        {
            uint32_t mid = FSM_ULTRASOUND_NUM_MEASUREMENTS / 2;
            p_fsm -> distance_cm = (p_fsm -> distance_arr[mid - 1] + p_fsm -> distance_arr[mid]) / 2;
        } 
        else
        {
            p_fsm -> distance_cm = p_fsm -> distance_arr[FSM_ULTRASOUND_NUM_MEASUREMENTS / 2];
        }

        p_fsm -> new_measurement = true;
    }

    port_ultrasound_stop_echo_timer(p_fsm -> ultrasound_id);
    port_ultrasound_reset_echo_ticks(p_fsm -> ultrasound_id);
}


/**
 * @brief Array representing the transitions table of the FSM ultrasaund.
 * 
 */
static fsm_trans_t fsm_trans_ultrasound[] = {
    {WAIT_START, check_on, TRIGGER_START, do_start_measurement},
    {TRIGGER_START, check_trigger_end, WAIT_ECHO_START, do_stop_trigger},
    {WAIT_ECHO_START, check_echo_init, WAIT_ECHO_END, NULL},
    {WAIT_ECHO_END, check_echo_received, SET_DISTANCE, do_set_distance},
    {SET_DISTANCE, check_new_measurement, TRIGGER_START, do_start_new_measurement},
    {SET_DISTANCE, check_off, WAIT_START, do_stop_measurement},
    {-1, NULL, -1, NULL}
};


/* Other auxiliary functions */
/**
 * @brief Initialize a ultrasound FSM.
 * This function initializes the default values of the FSM struct and calls to the port to initialize the associated HW given the ID.
 * The FSM stores the distance of the last ultrasound trigger. The FSM contains information of the ultrasound ID.
 * This ID is a unique identifier that is managed by the user in the port. That is where the user provides identifiers and HW information for all the ultrasounds on his system.
 * The FSM does not have to know anything of the underlying HW.
 * 
 * @param p_fsm_ultrasound  
 * @param ultrasound_id 
 */
void fsm_ultrasound_init(fsm_ultrasound_t *p_fsm_ultrasound, uint32_t ultrasound_id)
{
    // Initialize the FSM
    fsm_init(&p_fsm_ultrasound-> f, fsm_trans_ultrasound);

    /* TODO alumnos: */
    // Initialize the fields of the FSM structure
    p_fsm_ultrasound->ultrasound_id = ultrasound_id;

    // Initialize distance fields
    p_fsm_ultrasound->distance_cm = 0;
    p_fsm_ultrasound->distance_idx = 0;

    // Set status and new_measurement to false
    p_fsm_ultrasound->status = false;
    p_fsm_ultrasound->new_measurement = false;

    // Initialize the distance array to 0
    memset(p_fsm_ultrasound->distance_arr, 0, sizeof(p_fsm_ultrasound->distance_arr));

    // Initialize the HW of the ultrasound sensor using the ID
    port_ultrasound_init(ultrasound_id);
}


/* Public functions -----------------------------------------------------------*/
fsm_ultrasound_t *fsm_ultrasound_new(uint32_t ultrasound_id)
{
    fsm_ultrasound_t *p_fsm_ultrasound = malloc(sizeof(fsm_ultrasound_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_ultrasound_init(p_fsm_ultrasound, ultrasound_id);                  /* Initialize the FSM */
    return p_fsm_ultrasound;
}


void fsm_ultrasound_fire(fsm_ultrasound_t * p_fsm)
{
    fsm_fire(&p_fsm->f); 
}

void fsm_ultrasound_destroy(fsm_ultrasound_t * p_fsm)
{
    free(&p_fsm->f);
}


fsm_t* fsm_ultrasound_get_inner_fsm(fsm_ultrasound_t *p_fsm)
{
    return &p_fsm->f;
}

uint32_t fsm_ultrasound_get_state(fsm_ultrasound_t *p_fsm)
{
    return p_fsm->f.current_state;
}


uint32_t fsm_ultrasound_get_distance(fsm_ultrasound_t *p_fsm)
{
    p_fsm->new_measurement = false;  // Reset the flag
    return p_fsm->distance_cm;;
}

void fsm_ultrasound_stop(fsm_ultrasound_t *p_fsm)
{
    p_fsm->status = false;
    port_ultrasound_stop_ultrasound(p_fsm->ultrasound_id);
}

void fsm_ultrasound_start(fsm_ultrasound_t *p_fsm)
{
    p_fsm->status = true;
    p_fsm->distance_idx = 0;

    p_fsm->distance_cm = 0;

    port_ultrasound_reset_echo_ticks(p_fsm->ultrasound_id);
    port_ultrasound_set_trigger_ready(p_fsm->ultrasound_id, true);

    port_ultrasound_start_new_measurement_timer();
}

bool fsm_ultrasound_get_status(fsm_ultrasound_t *p_fsm)
{
    return p_fsm -> status;
}

void fsm_ultrasound_set_status(fsm_ultrasound_t *p_fsm, bool status)
{
    p_fsm->status = status;
}

bool fsm_ultrasound_get_ready(fsm_ultrasound_t *p_fsm)
{
    return port_ultrasound_get_trigger_ready(p_fsm->ultrasound_id);
}

bool fsm_ultrasound_get_new_measurement_ready(fsm_ultrasound_t *p_fsm)
{
    return p_fsm->new_measurement;
}


// Other auxiliary functions
void fsm_ultrasound_set_state(fsm_ultrasound_t *p_fsm, int8_t state)
{
    p_fsm -> f.current_state = state;
}

bool fsm_ultrasound_check_activity(fsm_ultrasound_t * p_fsm)
{
    return false;
}