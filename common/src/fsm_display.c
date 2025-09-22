/**
 * @file fsm_display.c
 * @brief Display system FSM main file.
 * @author Alvaro Castillo Esteban
 * @author Maya Lopez Romero
 * @date fecha
 */

/* Includes ------------------------------------------------------------------*/
/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>

/* HW dependent includes */
#include "port_display.h"
#include "port_system.h"

/* Project includes */
#include "fsm.h"
#include "fsm_display.h"

/* Typedefs --------------------------------------------------------------------*/
/**
 * @brief Structure of the Display FSM.
 * 
 */
struct fsm_display_t 
{
    fsm_t f;                /*!<    Display system FSM*/
    int32_t distance_cm;    /*!<    Distance in cm to the object*/
    bool new_color;         /*!<    Flag to indicate if a new color has to be set*/
    bool status;            /*!<    Flag to indicate if the display is active*/
    bool idle;              /*!<    Flag to indicate if the display being active is idle, or not*/
    uint32_t display_id;    /*!<    Unique display system identifier number*/
};

/* Private functions -----------------------------------------------------------*/
/**
 * @brief Interpolates between two RGB colors.
 * 
 * This function computes a linear interpolation between two RGB color values based on 
 * the interpolation factor t. The result is stored in the p_color output parameter.
 * - t = 0   → 100% colour_1
 * - t = 255 → 100% colour_2
 * 
 * @param p_color   Pointer to an rgb_color_t structure where the interpolated color will be stored.
 * @param colour_1  The starting color (corresponding to t = 0.0).
 * @param colour_2  The ending color (corresponding to t = 1.0).
 * @param t         Interpolation factor between 0.0 and 1.0. Values outside this range are extrapolated.
 */
void _interpolate_color(rgb_color_t *p_color, rgb_color_t colour_1, rgb_color_t colour_2, uint8_t t) {
    p_color -> r = (((uint16_t)(255 - t)) * colour_1.r + ((uint16_t)t) * colour_2.r) / 255;
    p_color -> g = (((uint16_t)(255 - t)) * colour_1.g + ((uint16_t)t) * colour_2.g) / 255;
    p_color -> b = (((uint16_t)(255 - t)) * colour_1.b + ((uint16_t)t) * colour_2.b) / 255;
}

/**
 * @brief Set color levels of the RGB LEDs according to the distance.
 * This function sets the levels of an RGB LED according to the distance measured by the ultrasound sensor.
 * This RGB LED structure is later passed to the port_display_set_rgb() function to set the color of the RGB LED.
 * 
 * @param p_color       Pointer to an rgb_color_t struct that will store the levels of the RGB LED.
 * @param distance_cm   Distance measured by the ultrasound sensor in centimeters.
 */
void _compute_display_levels(rgb_color_t *p_color, int32_t distance_cm)
{
    if ((distance_cm >= DANGER_MIN_CM) && (distance_cm <= WARNING_MIN_CM))
    {
        uint8_t t = ((uint8_t)(distance_cm - DANGER_MIN_CM) * 255) / (WARNING_MIN_CM - DANGER_MIN_CM);
        _interpolate_color(p_color, COLOR_RED, COLOR_YELLOW, t);
        return;
    }
    else if ((distance_cm > WARNING_MIN_CM) && (distance_cm <= NO_PROBLEM_MIN_CM))
    {
        uint8_t t = ((uint8_t)(distance_cm - WARNING_MIN_CM) * 255) / (NO_PROBLEM_MIN_CM - WARNING_MIN_CM);
        _interpolate_color(p_color, COLOR_YELLOW, COLOR_GREEN, t);
        return;
    }
    else if ((distance_cm > NO_PROBLEM_MIN_CM) && (distance_cm <= INFO_MIN_CM))
    {
        uint8_t t = ((uint8_t)(distance_cm - NO_PROBLEM_MIN_CM) * 255) / (INFO_MIN_CM - NO_PROBLEM_MIN_CM);
        _interpolate_color(p_color, COLOR_GREEN, COLOR_TURQUOISE, t);
        return;
    }
    else if ((distance_cm > INFO_MIN_CM) && (distance_cm <= OK_MIN_CM))
    {
        uint8_t t = ((uint8_t)(distance_cm - INFO_MIN_CM) * 255) / (OK_MIN_CM - INFO_MIN_CM);
        _interpolate_color(p_color, COLOR_TURQUOISE, COLOR_BLUE, t);
        return;
    }
    else if ((distance_cm > OK_MIN_CM)&&(distance_cm <= OK_MAX_CM))
    {
        *p_color = COLOR_BLUE;
        return;
    }
    else
    {
        *p_color = COLOR_OFF;
        return;
    }
}

/* State machine input or transition functions */
/**
 * @brief Check if the display is set to be active (ON), independently if it is idle or not.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 * @return true     If the display system has been indicated to be active independently if it is idle or not.
 * @return false    If the display system has been indicated to be inactive.
 */
static bool check_active (fsm_t *p_this)
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);
    return p_fsm -> status;
}

/**
 * @brief Check if a new color has to be set.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 * @return true     If a new color has to be set
 * @return false    If a new color does not have to be set
 */
static bool check_set_new_color (fsm_t *p_this)
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);
    return p_fsm -> new_color;
}

/**
 * @brief Check if the display is set to be inactive (OFF).
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 * @return true     If the display system has been indicated to be inactive.
 * @return false    If the display system has been indicated to be active.
 */
static bool check_off (fsm_t *p_this)
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);
    return !(p_fsm -> status);
}


/* State machine output or action functions */
/**
 * @brief Turn the display system ON for the first time.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 */
static void do_set_on (fsm_t * p_this)
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);
    port_display_set_rgb(p_fsm -> display_id, COLOR_OFF);
}

/**
 * @brief Set the color of the RGB LED according to the distance measured by the ultrasound sensor.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 */
static void do_set_color (fsm_t * p_this)
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);
    rgb_color_t color;

    _compute_display_levels (&color, p_fsm -> distance_cm);
    port_display_set_rgb(p_fsm -> display_id, color);

    p_fsm -> new_color = false;
    p_fsm -> idle = true;
}

/**
 * @brief Turn the display system OFF.
 * 
 * @param p_this    Pointer to an fsm_t struct than contains an fsm_display_t.
 */
static void do_set_off (fsm_t * p_this)	
{
    fsm_display_t *p_fsm = (fsm_display_t *)(p_this);

    port_display_set_rgb(p_fsm -> display_id, COLOR_OFF);

    p_fsm -> idle = false;
}


/**
 * @brief Array representing the transitions table of the FSM display.
 * 
 */
static fsm_trans_t fsm_trans_display[] = {
    {WAIT_DISPLAY, check_active, SET_DISPLAY, do_set_on},
    {SET_DISPLAY, check_set_new_color, SET_DISPLAY, do_set_color},
    {SET_DISPLAY, check_off, WAIT_DISPLAY, do_set_off},
    {-1, NULL, -1, NULL}
};


/* Other auxiliary functions */
/**
 * @brief Initialize a display system FSM.
 * This function initializes the default values of the FSM struct and calls to the port to initialize the associated HW given the ID.
 * The FSM stores the display level of the display system. The FSM contains information of the RGB LED ID. This ID is a unique identifier that is managed by the user in the port.
 * The FSM does not have to know anything of the underlying HW.
 * 
 * @param p_fsm_display Pointer to the display FSM.
 * @param display_id    Unique display identifier number.
 */
static void fsm_display_init (fsm_display_t * p_fsm_display, uint32_t display_id)
{
    fsm_init(&p_fsm_display-> f, fsm_trans_display);

    // Initialize ID and Distance
    p_fsm_display -> display_id = display_id;
    p_fsm_display -> distance_cm = -1;

    // Initialize Flags to false
    p_fsm_display -> new_color = false;
    p_fsm_display -> status = false;
    p_fsm_display -> idle = false;

    // Initialize HW
    port_display_init(display_id);
}


/* Public functions -----------------------------------------------------------*/
fsm_display_t *fsm_display_new(uint32_t display_id)
{
    fsm_display_t *p_fsm_display = malloc(sizeof(fsm_display_t)); /* Do malloc to reserve memory of all other FSM elements, although it is interpreted as fsm_t (the first element of the structure) */
    fsm_display_init(p_fsm_display, display_id); /* Initialize the FSM */
    return p_fsm_display;
}


void fsm_display_fire (fsm_display_t * 	p_fsm)
{
    fsm_fire(&p_fsm->f);
}

void fsm_display_destroy(fsm_display_t * p_fsm)
{
    free(&p_fsm->f);
}


fsm_t* fsm_display_get_inner_fsm (fsm_display_t * p_fsm)	
{
    return &p_fsm -> f;
}

uint32_t fsm_display_get_state (fsm_display_t * p_fsm)
{
    return p_fsm-> f.current_state;
}

void fsm_display_set_state (fsm_display_t * p_fsm, int8_t state)
{
    p_fsm-> f.current_state = state;
}


uint32_t fsm_display_get_distance(fsm_display_t * p_fsm)
{
    return p_fsm -> distance_cm;
}

void fsm_display_set_distance (fsm_display_t * p_fsm, uint32_t distance_cm)
{
    p_fsm -> distance_cm = distance_cm;
    p_fsm -> new_color = true;
}

bool fsm_display_get_status (fsm_display_t * p_fsm)	
{
    return p_fsm -> status;
}

void fsm_display_set_status (fsm_display_t * p_fsm, bool pause)
{
    p_fsm -> status = pause;
}


bool fsm_display_check_activity	(fsm_display_t * p_fsm)
{
    return (p_fsm -> status) && !(p_fsm -> idle);
}
