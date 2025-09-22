/**
 * @file fsm_button.h
 * @brief Header for fsm_button.c file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date 01/04/2025
 */

#ifndef FSM_BUTTON_H_
#define FSM_BUTTON_H_

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdint.h>
#include <stdbool.h>

/* Other includes */
#include "fsm.h"

/* Defines and enums ----------------------------------------------------------*/
/* Enums */
/**
 * @brief Enumerator for the button finite state machine.
 * This enumerator defines the different states that the button finite state machine can be in.
 * Each state represents a specific condition or step in the button press process.
 */
typedef enum 
{ 
    BUTTON_RELEASED, /*!< Initial state, waiting for button press */ 
    BUTTON_PRESSED_WAIT, /*!< Debounce delay after button press */ 
    BUTTON_PRESSED, /*!< Button is pressed, waiting for release */ 
    BUTTON_RELEASED_WAIT /*!< Debounce delay after button release */ 
} FSM_BUTTON_STATE; 

/* Typedefs --------------------------------------------------------------------*/
typedef struct fsm_button_t fsm_button_t;

/* Function prototypes and explanation -------------------------------------------------*/

/**
 * @brief Check if the button FSM is active, or not.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 * @return true 
 * @return false 
 */
bool fsm_button_check_activity(fsm_button_t * p_fsm);

/**
 * @brief Destroy a button FSM. This function destroys a button FSM and frees the memory.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 */
void fsm_button_destroy	(fsm_button_t * p_fsm);

/**
 * @brief Fire the button FSM.
 * This function is used to fire the button FSM. It is used to check the transitions and execute the actions of the button FSM.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 */
void fsm_button_fire	(	fsm_button_t * 	p_fsm	);

/**
 * @brief Get the debounce time of the button FSM. This function returns the debounce time of the button FSM.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 * @return uint32_t    Debounce time in milliseconds.
 */
uint32_t fsm_button_get_debounce_time_ms (fsm_button_t * p_fsm);

/**
 * @brief Return the duration of the last button press.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 * @return uint32_t Duration of the last button press in milliseconds.
 */
uint32_t fsm_button_get_duration (fsm_button_t * p_fsm);


/**
 * @brief Get the inner FSM of the button. This function returns the inner FSM of the button.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 * @return fsm_t*   Pointer to the inner FSM.
 */
fsm_t* fsm_button_get_inner_fsm	(fsm_button_t * p_fsm);

/**
 * @brief Get the state of the button FSM. This function returns the current state of the button FSM.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 * @return uint32_t Current state of the button FSM.
 */
uint32_t fsm_button_get_state (fsm_button_t * p_fsm);	


/**
 * @brief Create a new button FSM. This function creates a new button FSM with the given debounce time and button ID.
 * 
 * @param debounce_time_ms  Debounce time in milliseconds.
 * @param button_id         Button ID. Must be unique.
 * @return fsm_button_t*    Pointer to the button FSM.
 */
fsm_button_t* fsm_button_new (uint32_t debounce_time_ms, uint32_t button_id);

/**
 * @brief Reset the duration of the last button press.
 * 
 * @param p_fsm     Pointer to an fsm_button_t struct.
 */
void fsm_button_reset_duration (fsm_button_t * p_fsm);


#endif