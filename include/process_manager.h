/*
 * process_manager.h - Child Process Management for Phoenix SDR Controller
 * 
 * Manages spawning and monitoring of child processes (SDR server, waterfall).
 * Uses Windows Job Objects to ensure children are terminated when parent exits.
 */

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <windows.h>
#include <stdbool.h>

/*============================================================================
 * Constants
 *============================================================================*/

#define PROCESS_NAME_MAX    64
#define PROCESS_PATH_MAX    260
#define PROCESS_ARGS_MAX    512

/* Process indices */
#define PROC_SDR_SERVER     0
#define PROC_WATERFALL      1
#define PROC_COUNT          2

/*============================================================================
 * Types
 *============================================================================*/

/**
 * Child process state
 */
typedef struct {
    PROCESS_INFORMATION pi;     /* Windows process info */
    char name[PROCESS_NAME_MAX];/* Display name */
    char path[PROCESS_PATH_MAX];/* Executable path */
    char args[PROCESS_ARGS_MAX];/* Command line arguments */
    bool running;               /* Currently running? */
    bool show_window;           /* Show console window? */
} child_process_t;

/**
 * Process manager state
 */
typedef struct {
    HANDLE job;                         /* Job object for child cleanup */
    child_process_t children[PROC_COUNT];
    bool initialized;
} process_manager_t;

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * Initialize the process manager
 * Creates job object to ensure children die when parent exits
 * 
 * @param pm    Process manager to initialize
 * @return      true on success
 */
bool process_manager_init(process_manager_t *pm);

/**
 * Shutdown process manager and kill all children
 * 
 * @param pm    Process manager to shutdown
 */
void process_manager_shutdown(process_manager_t *pm);

/**
 * Configure a child process (must be called before start)
 * 
 * @param pm            Process manager
 * @param index         Process index (PROC_SDR_SERVER or PROC_WATERFALL)
 * @param name          Display name for the process
 * @param exe_path      Path to executable
 * @param args          Command line arguments (NULL for none)
 * @param show_window   true = visible console, false = hidden
 */
void process_manager_configure(process_manager_t *pm, int index,
                               const char *name, const char *exe_path,
                               const char *args, bool show_window);

/**
 * Start a child process
 * 
 * @param pm        Process manager
 * @param index     Process index
 * @return          true on success
 */
bool process_manager_start(process_manager_t *pm, int index);

/**
 * Stop a child process
 * 
 * @param pm        Process manager
 * @param index     Process index
 */
void process_manager_stop(process_manager_t *pm, int index);

/**
 * Check if a child process is running
 * Also updates internal state if process has exited
 * 
 * @param pm        Process manager
 * @param index     Process index
 * @return          true if running
 */
bool process_manager_is_running(process_manager_t *pm, int index);

/**
 * Get process name for display
 * 
 * @param pm        Process manager
 * @param index     Process index
 * @return          Process name string
 */
const char *process_manager_get_name(process_manager_t *pm, int index);

/**
 * Toggle a process (start if stopped, stop if running)
 * 
 * @param pm        Process manager
 * @param index     Process index
 * @return          true if now running, false if now stopped
 */
bool process_manager_toggle(process_manager_t *pm, int index);

/**
 * Load process configuration from INI file
 * Looks for [Processes] section with server_path, server_args, etc.
 * 
 * @param pm        Process manager
 * @param filename  INI file path
 * @return          true if loaded successfully
 */
bool process_manager_load_config(process_manager_t *pm, const char *filename);

/**
 * Save process configuration to INI file
 * Appends [Processes] section to existing file
 * 
 * @param pm        Process manager
 * @param filename  INI file path
 * @return          true if saved successfully
 */
bool process_manager_save_config(process_manager_t *pm, const char *filename);

#endif /* PROCESS_MANAGER_H */
