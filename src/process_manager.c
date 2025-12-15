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

bool process_manager_load_config(process_manager_t *pm, const char *filename) {
    if (!pm || !filename) return false;
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        LOG_DEBUG("No config file found: %s", filename);
        return false;
    }
    
    char line[512];
    bool in_processes_section = false;
    
    while (fgets(line, sizeof(line), f)) {
        /* Trim newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        nl = strchr(line, '\r');
        if (nl) *nl = '\0';
        
        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;
        
        /* Section headers */
        if (line[0] == '[') {
            in_processes_section = (strcmp(line, "[Processes]") == 0);
            continue;
        }
        
        /* Key=value pairs in [Processes] section */
        if (in_processes_section) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                const char *key = line;
                const char *value = eq + 1;
                
                if (strcmp(key, "server_path") == 0) {
                    strncpy(pm->children[PROC_SDR_SERVER].path, value, PROCESS_PATH_MAX - 1);
                } else if (strcmp(key, "server_args") == 0) {
                    strncpy(pm->children[PROC_SDR_SERVER].args, value, PROCESS_ARGS_MAX - 1);
                } else if (strcmp(key, "server_show_window") == 0) {
                    pm->children[PROC_SDR_SERVER].show_window = (atoi(value) != 0);
                } else if (strcmp(key, "waterfall_path") == 0) {
                    strncpy(pm->children[PROC_WATERFALL].path, value, PROCESS_PATH_MAX - 1);
                } else if (strcmp(key, "waterfall_args") == 0) {
                    strncpy(pm->children[PROC_WATERFALL].args, value, PROCESS_ARGS_MAX - 1);
                } else if (strcmp(key, "waterfall_show_window") == 0) {
                    pm->children[PROC_WATERFALL].show_window = (atoi(value) != 0);
                }
            }
        }
    }
    
    fclose(f);
    LOG_INFO("Loaded process config from %s", filename);
    return true;
}

bool process_manager_save_config(process_manager_t *pm, const char *filename) {
    if (!pm || !filename) return false;
    
    /* Read existing file content (without [Processes] section) */
    char *existing_content = NULL;
    size_t existing_size = 0;
    
    FILE *f = fopen(filename, "r");
    if (f) {
        /* Read file and filter out [Processes] section */
        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (file_size > 0) {
            char *buffer = malloc(file_size + 1);
            existing_content = malloc(file_size + 1);
            if (buffer && existing_content) {
                fread(buffer, 1, file_size, f);
                buffer[file_size] = '\0';
                
                /* Copy everything except [Processes] section */
                char *src = buffer;
                char *dst = existing_content;
                bool skip_section = false;
                
                while (*src) {
                    /* Find end of line */
                    char *eol = strchr(src, '\n');
                    size_t line_len = eol ? (eol - src + 1) : strlen(src);
                    
                    /* Check for section header */
                    if (src[0] == '[') {
                        skip_section = (strncmp(src, "[Processes]", 11) == 0);
                    }
                    
                    /* Copy line if not in [Processes] section */
                    if (!skip_section) {
                        memcpy(dst, src, line_len);
                        dst += line_len;
                    }
                    
                    src += line_len;
                }
                *dst = '\0';
                existing_size = dst - existing_content;
                
                free(buffer);
            }
        }
        fclose(f);
    }
    
    /* Write file with [Processes] section at end */
    f = fopen(filename, "w");
    if (!f) {
        LOG_ERROR("Failed to open %s for writing", filename);
        free(existing_content);
        return false;
    }
    
    /* Write existing content */
    if (existing_content && existing_size > 0) {
        fwrite(existing_content, 1, existing_size, f);
        /* Ensure newline before new section */
        if (existing_content[existing_size - 1] != '\n') {
            fprintf(f, "\n");
        }
    }
    free(existing_content);
    
    /* Write [Processes] section */
    fprintf(f, "[Processes]\n");
    fprintf(f, "server_path=%s\n", pm->children[PROC_SDR_SERVER].path);
    fprintf(f, "server_args=%s\n", pm->children[PROC_SDR_SERVER].args);
    fprintf(f, "server_show_window=%d\n", pm->children[PROC_SDR_SERVER].show_window ? 1 : 0);
    fprintf(f, "waterfall_path=%s\n", pm->children[PROC_WATERFALL].path);
    fprintf(f, "waterfall_args=%s\n", pm->children[PROC_WATERFALL].args);
    fprintf(f, "waterfall_show_window=%d\n", pm->children[PROC_WATERFALL].show_window ? 1 : 0);
    
    fclose(f);
    LOG_INFO("Saved process config to %s", filename);
    return true;
}
