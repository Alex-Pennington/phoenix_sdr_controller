/*
 * process_manager.c - Child Process Management Implementation
 * 
 * Spawns and monitors child processes (SDR server, waterfall).
 * Uses Windows Job Objects to ensure children are terminated when parent exits.
 */

#include "process_manager.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

/*============================================================================
 * Internal Helpers
 *============================================================================*/

/**
 * Spawn a single child process
 */
static bool spawn_process(HANDLE job, child_process_t *child) {
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    
    /* Hide window if requested */
    if (!child->show_window) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    
    /* Build command line */
    char cmd_line[PROCESS_PATH_MAX + PROCESS_ARGS_MAX + 4];
    if (child->args[0]) {
        snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %s", child->path, child->args);
    } else {
        snprintf(cmd_line, sizeof(cmd_line), "\"%s\"", child->path);
    }
    
    /* Creation flags */
    DWORD flags = child->show_window ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;
    
    /* Create process */
    if (!CreateProcess(
            NULL,           /* Use cmd_line for exe name */
            cmd_line,       /* Command line */
            NULL,           /* Process security */
            NULL,           /* Thread security */
            FALSE,          /* Don't inherit handles */
            flags,          /* Creation flags */
            NULL,           /* Use parent environment */
            NULL,           /* Use parent directory */
            &si,            /* Startup info */
            &child->pi      /* Process info (output) */
    )) {
        LOG_ERROR("Failed to start %s: error %lu", child->path, GetLastError());
        return false;
    }
    
    /* Add to job object so it dies when parent dies */
    if (job) {
        AssignProcessToJobObject(job, child->pi.hProcess);
    }
    
    child->running = true;
    LOG_INFO("Started: %s (PID %lu)", child->name, child->pi.dwProcessId);
    
    return true;
}

/**
 * Kill a single child process
 */
static void kill_child(child_process_t *child) {
    if (child->running) {
        LOG_INFO("Stopping: %s (PID %lu)", child->name, child->pi.dwProcessId);
        TerminateProcess(child->pi.hProcess, 0);
        WaitForSingleObject(child->pi.hProcess, 1000);  /* Wait up to 1s */
        child->running = false;
    }
    
    if (child->pi.hProcess) {
        CloseHandle(child->pi.hProcess);
        child->pi.hProcess = NULL;
    }
    if (child->pi.hThread) {
        CloseHandle(child->pi.hThread);
        child->pi.hThread = NULL;
    }
}

/*============================================================================
 * Public API
 *============================================================================*/

bool process_manager_init(process_manager_t *pm) {
    if (!pm) return false;
    
    memset(pm, 0, sizeof(*pm));
    
    /* Create job object */
    pm->job = CreateJobObject(NULL, NULL);
    if (!pm->job) {
        LOG_ERROR("Failed to create job object: error %lu", GetLastError());
        return false;
    }
    
    /* Configure job to kill children when job closes */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(pm->job, JobObjectExtendedLimitInformation, 
                                  &jeli, sizeof(jeli))) {
        LOG_WARN("Failed to configure job object: error %lu", GetLastError());
        /* Continue anyway - children just won't auto-die */
    }
    
    /* Set default configurations */
    process_manager_configure(pm, PROC_SDR_SERVER,
                              "SDR Server", "sdr_server.exe", NULL, false);
    process_manager_configure(pm, PROC_WATERFALL,
                              "Waterfall", "waterfall.exe", "--tcp localhost:4536", true);
    
    pm->initialized = true;
    LOG_INFO("Process manager initialized");
    
    return true;
}

void process_manager_shutdown(process_manager_t *pm) {
    if (!pm || !pm->initialized) return;
    
    LOG_INFO("Process manager shutting down...");
    
    /* Stop all children */
    for (int i = 0; i < PROC_COUNT; i++) {
        if (pm->children[i].running) {
            kill_child(&pm->children[i]);
        }
    }
    
    /* Close job object (also kills any stragglers) */
    if (pm->job) {
        CloseHandle(pm->job);
        pm->job = NULL;
    }
    
    pm->initialized = false;
    LOG_INFO("Process manager shutdown complete");
}

void process_manager_configure(process_manager_t *pm, int index,
                               const char *name, const char *exe_path,
                               const char *args, bool show_window) {
    if (!pm || index < 0 || index >= PROC_COUNT) return;
    
    child_process_t *child = &pm->children[index];
    
    if (name) {
        strncpy(child->name, name, PROCESS_NAME_MAX - 1);
        child->name[PROCESS_NAME_MAX - 1] = '\0';
    }
    
    if (exe_path) {
        strncpy(child->path, exe_path, PROCESS_PATH_MAX - 1);
        child->path[PROCESS_PATH_MAX - 1] = '\0';
    }
    
    if (args) {
        strncpy(child->args, args, PROCESS_ARGS_MAX - 1);
        child->args[PROCESS_ARGS_MAX - 1] = '\0';
    } else {
        child->args[0] = '\0';
    }
    
    child->show_window = show_window;
}

bool process_manager_start(process_manager_t *pm, int index) {
    if (!pm || !pm->initialized || index < 0 || index >= PROC_COUNT) {
        return false;
    }
    
    child_process_t *child = &pm->children[index];
    
    /* Already running? */
    if (process_manager_is_running(pm, index)) {
        LOG_WARN("%s is already running", child->name);
        return true;
    }
    
    /* Check if executable exists */
    if (GetFileAttributesA(child->path) == INVALID_FILE_ATTRIBUTES) {
        LOG_ERROR("Executable not found: %s", child->path);
        return false;
    }
    
    return spawn_process(pm->job, child);
}

void process_manager_stop(process_manager_t *pm, int index) {
    if (!pm || !pm->initialized || index < 0 || index >= PROC_COUNT) {
        return;
    }
    
    kill_child(&pm->children[index]);
}

bool process_manager_is_running(process_manager_t *pm, int index) {
    if (!pm || !pm->initialized || index < 0 || index >= PROC_COUNT) {
        return false;
    }
    
    child_process_t *child = &pm->children[index];
    
    if (!child->running) return false;
    
    /* Check if process is still active */
    DWORD exit_code;
    if (GetExitCodeProcess(child->pi.hProcess, &exit_code)) {
        if (exit_code != STILL_ACTIVE) {
            LOG_INFO("%s exited with code %lu", child->name, exit_code);
            child->running = false;
            
            /* Clean up handles */
            CloseHandle(child->pi.hProcess);
            CloseHandle(child->pi.hThread);
            child->pi.hProcess = NULL;
            child->pi.hThread = NULL;
            
            return false;
        }
    }
    
    return true;
}

const char *process_manager_get_name(process_manager_t *pm, int index) {
    if (!pm || index < 0 || index >= PROC_COUNT) {
        return "Unknown";
    }
    return pm->children[index].name;
}

bool process_manager_toggle(process_manager_t *pm, int index) {
    if (!pm || !pm->initialized || index < 0 || index >= PROC_COUNT) {
        return false;
    }
    
    if (process_manager_is_running(pm, index)) {
        process_manager_stop(pm, index);
        return false;
    } else {
        return process_manager_start(pm, index);
    }
}
