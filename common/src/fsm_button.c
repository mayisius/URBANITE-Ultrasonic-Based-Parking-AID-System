/**
 * @file fsm_button.c
 * @brief Button FSM main file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date fecha
 */

 
/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>

/* HW dependent includes */
#include "port_button.h"
#include "port_system.h"

/* Project includes */
#include "fsm_button.h"


/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure of the Button FSM.
 * 
 */
struct fsm_button_t
{
    fsm_t f;                    /*!< Button FSM */
    uint32_t debounce_time;     /*!< Button debounce time in ms */
    uint32_t next_timeout;      /*!< Next timeout for the anti-debounce in ms */
    uint32_t tick_pressed;      /*!< Number of ticks when the button was pressed */
    uint32_t duration;          /*!< How much time the button has been pressed */
    uint32_t button_id;         /*!< Button ID. Must be unique. */
};

/* State machine input or transition functions */
/**
 * @brief Construct a new check button pressed object
 * 
 */
static bool check_button_pressed(fsm_t * p_this)
{
    fsm_button_t *p_fsm = (fsm_button_t *)(p_this);
    return (bool) port_button_get_pressed(p_fsm -> button_id);
}

/**
 * @brief Check if the button has been released.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_button_t.
 * @return true 
 * @return false 
 */
static bool check_button_released ( fsm_t * p_this )
{
    fsm_button_t *p_fsm = (fsm_button_t *)(p_this); 
    return !port_button_get_pressed(p_fsm->button_id);
}	

/**
 * @brief Check if the debounce-time has passed.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_button_t.
 * @return true 
 * @return false 
 */
static bool check_timeout(fsm_t * p_this)
{
    fsm_button_t *p_fsm = (fsm_button_t *)(p_this); 
    uint32_t time = port_system_get_millis();

    if(time > p_fsm -> next_timeout)
    {
        return true;
    } 
    else
    {
        return false;
    }
}


/* State machine output or action functions */

/**
 * @brief Store the system tick when the button was pressed.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_button_t.
 */
static void do_store_tick_pressed (fsm_t * p_this)
{
    fsm_button_t *p_fsm = (fsm_button_t *)(p_this); 
    uint32_t time = port_system_get_millis();

    p_fsm -> tick_pressed = time;
    p_fsm -> next_timeout = time + p_fsm->debounce_time;
}

/**
 * @brief Store the duration of the button press.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_button_t.
 */
static void do_set_duration	(fsm_t * p_this)
{
    fsm_button_t *p_fsm = (fsm_button_t *)(p_this); 
    uint32_t time = port_system_get_millis();

    p_fsm->duration = time - p_fsm->tick_pressed;
    p_fsm->next_timeout = time + p_fsm->debounce_time;

    printf("[DEBUG][%ld] Duración: %ld\n", time ,p_fsm -> duration);
}	


/**
 * @brief Array representing the transitions table of the FSM button.
 * 
 */
static fsm_trans_t fsm_trans_button[] = { 
    { BUTTON_RELEASED, check_button_pressed, BUTTON_PRESSED_WAIT, do_store_tick_pressed },
    { BUTTON_PRESSED_WAIT, check_timeout, BUTTON_PRESSED, NULL },
    { BUTTON_PRESSED, check_button_released, BUTTON_RELEASED_WAIT, do_set_duration },
    { BUTTON_RELEASED_WAIT, check_timeout, BUTTON_RELEASED, NULL },
    { -1, NULL, -1, NULL }

};


/* Other auxiliary functions */
void fsm_button_init(fsm_button_t *p_fsm_button, uint32_t debounce_time, uint32_t button_id)
{
    fsm_init(&p_fsm_button->f, fsm_trans_button);

    /* TODO alumnos: */
    p_fsm_button->debounce_time = debounce_time;
    p_fsm_button->button_id = button_id; 
    
    p_fsm_button->tick_pressed = 0;
    p_fsm_button->duration = 0;
    port_button_init(button_id);
}

/* Public functions -----------------------------------------------------------*/
fsm_button_t *fsm_button_new(uint32_t debounce_time, uint32_t button_id)
{
    fsm_button_t *p_fsm_button = malloc(sizeof(fsm_button_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_button_init(p_fsm_button, debounce_time, button_id);   /* Initialize the FSM */
    return p_fsm_button;                                       /* Composite pattern: return the fsm_t pointer as a fsm_button_t pointer */
}




/* FSM-interface functions. These functions are used to interact with the FSM */
void fsm_button_fire(fsm_button_t *p_fsm)
{
    fsm_fire(&p_fsm->f); // Is it also possible to it in this way: fsm_fire((fsm_t *)p_fsm);
}

void fsm_button_destroy(fsm_button_t *p_fsm)
{
    free(&p_fsm->f);
}

fsm_t *fsm_button_get_inner_fsm(fsm_button_t *p_fsm)
{
    return &p_fsm->f;
}

uint32_t fsm_button_get_state(fsm_button_t *p_fsm)
{
    return p_fsm->f.current_state;
}

uint32_t fsm_button_get_duration (fsm_button_t * p_fsm)	
{
    return p_fsm -> duration;
}

void fsm_button_reset_duration	(fsm_button_t * p_fsm)
{
    p_fsm -> duration = 0;
}

uint32_t fsm_button_get_debounce_time_ms( fsm_button_t * p_fsm)
{
    return (uint32_t)(p_fsm -> debounce_time);
}


bool fsm_button_check_activity(fsm_button_t * p_fsm)
{
    return !(p_fsm -> f.current_state == BUTTON_RELEASED);
}