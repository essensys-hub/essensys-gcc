/**
 * BP (SC944D) Firmware - Main Entry Point
 *
 * Minimal test application for CI/CD pipeline validation.
 * Full application sources should be integrated from essensys-board-SC944D.
 */

#include <mqx.h>
#include <lwevent.h>

/* Task IDs */
#define MAIN_TASK 1

/* Stack size for tasks */
#define MAIN_STACK_SIZE 1024

/* Forward declaration: referenced by MQX_template_list below, defined later
 * in this file. Required by GCC (unlike CodeWarrior, GCC does not allow a
 * bare identifier to be used as a value without a prior declaration). */
void main_task(uint32_t param);

/* Global task table */
const TASK_TEMPLATE_STRUCT MQX_template_list[] = {
    /* Task Index,   Function,   Stack,      Priority,   Name,          Attributes,          Param,  Time Slice */
    {   MAIN_TASK,   main_task,  MAIN_STACK_SIZE,  9,  "Main",   MQX_AUTO_START_TASK,    0,     0           },
    {   0,           0,          0,                0,  0,        0,                       0,     0           }
};

/**
 * main_task — Main application task
 *
 * This is a minimal placeholder for the SC944D main firmware task.
 * In production, this would contain:
 * - Initialization of domotique controllers
 * - Alarm management
 * - Heating control (fil pilote)
 * - Scenario execution
 * - Ethernet communication with server
 * - etc.
 */
void main_task(uint32_t param) {
    (void)param;  /* Unused */

    /* LED blink: Signal successful boot */
    while (1) {
        /* Placeholder: In production, this would run the domotique logic */
        _time_delay(1000);  /* Delay 1 second */
    }
}

/**
 * _mqx_fatal_error_handler — MQX fatal error handler
 */
void _mqx_fatal_error_handler(uint32_t error_code, void* error_context) {
    (void)error_code;
    (void)error_context;

    /* Halt: In production, this might log to persistent storage or signal hardware */
    while (1) {
        _time_delay(100);
    }
}
