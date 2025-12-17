/**
 * Phoenix SDR Controller - Main Application
 * 
 * WinMain entry point and main event loop.
 */

#include "common.h"
#include "tcp_client.h"
#include "sdr_protocol.h"
#include "app_state.h"
#include "ui_core.h"
#include "ui_widgets.h"
#include "ui_layout.h"
#include "process_manager.h"
#include "udp_telemetry.h"

#include <SDL.h>

/* Timing constants */
#define STATUS_POLL_INTERVAL_MS  500
#define KEEPALIVE_INTERVAL_MS    60000

/* Application context */
typedef struct {
    tcp_client_t* tcp;
    sdr_protocol_t* proto;
    app_state_t* state;
    ui_core_t* ui;
    ui_layout_t* layout;
    process_manager_t proc_mgr;
    udp_telemetry_t* telemetry;
} app_context_t;

/* Forward declarations */
static bool app_init(app_context_t* app);
static void app_shutdown(app_context_t* app);
static void app_handle_actions(app_context_t* app, const ui_actions_t* actions);
static void app_periodic_tasks(app_context_t* app);
static void app_connect(app_context_t* app);
static void app_disconnect(app_context_t* app);

/*
 * Windows entry point
 */
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
#else
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
#endif
    
    LOG_INFO("Phoenix SDR Controller v%s starting", APP_VERSION);
    
    /* Initialize application context */
    app_context_t app = {0};
    
    if (!app_init(&app)) {
        LOG_ERROR("Application initialization failed");
        app_shutdown(&app);
        return 1;
    }
    
    LOG_INFO("Application initialized successfully");
    
    /* Main event loop */
    mouse_state_t mouse = {0};
    ui_actions_t actions = {0};
    
    while (app.ui->running && !app.state->quit_requested) {
        /* Begin frame - poll events, clear screen */
        if (!ui_core_begin_frame(app.ui, &mouse)) {
            break;
        }
        
        /* Handle window resize */
        if (app.layout) {
            ui_layout_recalculate(app.layout);
        }
        
        /* Sync UI state from app state */
        ui_layout_sync_state(app.layout, app.state);
        
        /* Sync process button states */
        ui_layout_sync_process_state(app.layout, &app.proc_mgr);
        
        /* Update UI and get actions */
        ui_layout_update(app.layout, &mouse, NULL, &actions);
        
        /* Handle UI actions */
        app_handle_actions(&app, &actions);
        
        /* Periodic tasks (status polling, keepalive) */
        app_periodic_tasks(&app);
        
        /* Process async notifications from server */
        if (sdr_is_connected(app.proto)) {
            if (sdr_process_async(app.proto)) {
                /* Update state from protocol status */
                app_state_update_from_sdr(app.state, &app.proto->status);
            }
        }
        
        /* Debug: Toggle overload with 'O' key for testing */
        if (app.ui->last_key == SDLK_o) {
            app.state->overload = !app.state->overload;
            LOG_INFO("Debug: Overload toggled to %s", app.state->overload ? "ON" : "OFF");
        }
        
        /* Poll UDP telemetry */
        if (app.telemetry) {
            udp_telemetry_poll(app.telemetry);
            ui_layout_sync_telemetry(app.layout, app.telemetry);
        }
        
        /* Draw UI */
        ui_layout_draw(app.layout, app.state);
        
        /* Draw WWV telemetry panel (overlays main UI) */
        if (app.telemetry) {
            ui_layout_draw_wwv_panel(app.layout, app.telemetry);
        }
        
        /* End frame - present */
        ui_core_end_frame(app.ui);
    }
    
    LOG_INFO("Main loop ended");
    
    /* Cleanup */
    app_shutdown(&app);
    
    LOG_INFO("Phoenix SDR Controller exiting");
    return 0;
}

/*
 * Initialize application
 */
static bool app_init(app_context_t* app)
{
    /* Initialize TCP subsystem */
    if (!tcp_client_init()) {
        LOG_ERROR("Failed to initialize TCP client subsystem");
        return false;
    }
    
    /* Create TCP client */
    app->tcp = tcp_client_create();
    if (!app->tcp) {
        LOG_ERROR("Failed to create TCP client");
        return false;
    }
    
    /* Create protocol handler */
    app->proto = sdr_protocol_create(app->tcp);
    if (!app->proto) {
        LOG_ERROR("Failed to create protocol handler");
        return false;
    }
    
    /* Create application state */
    app->state = app_state_create();
    if (!app->state) {
        LOG_ERROR("Failed to create app state");
        return false;
    }
    
    /* Load presets from file */
    if (app_load_presets_from_file(app->state, PRESETS_FILENAME)) {
        LOG_INFO("Loaded presets from %s", PRESETS_FILENAME);
    }
    
    /* Initialize UI */
    char title[128];
    snprintf(title, sizeof(title), "%s v%s", APP_NAME, APP_VERSION);
    
    app->ui = ui_core_init(title);
    if (!app->ui) {
        LOG_ERROR("Failed to initialize UI core");
        return false;
    }
    
    /* Create UI layout */
    app->layout = ui_layout_create(app->ui);
    if (!app->layout) {
        LOG_ERROR("Failed to create UI layout");
        return false;
    }
    
    /* Initialize process manager */
    if (!process_manager_init(&app->proc_mgr)) {
        LOG_WARN("Failed to initialize process manager - external apps won't be managed");
        /* Non-fatal - continue anyway */
    } else {
        /* Load process paths from config file */
        process_manager_load_config(&app->proc_mgr, PRESETS_FILENAME);
    }
    
    /* Initialize UDP telemetry receiver */
    app->telemetry = udp_telemetry_create(TELEMETRY_UDP_PORT);
    if (app->telemetry) {
        if (udp_telemetry_start(app->telemetry)) {
            LOG_INFO("UDP telemetry listening on port %d", TELEMETRY_UDP_PORT);
        } else {
            LOG_WARN("Failed to start UDP telemetry - WWV stats will not be available");
        }
    }
    
    return true;
}

/*
 * Shutdown application
 */
static void app_shutdown(app_context_t* app)
{
    /* Save process config before shutdown */
    process_manager_save_config(&app->proc_mgr, PRESETS_FILENAME);
    
    /* Shutdown process manager (kills child processes) */
    process_manager_shutdown(&app->proc_mgr);
    
    /* Shutdown UDP telemetry */
    if (app->telemetry) {
        udp_telemetry_destroy(app->telemetry);
        app->telemetry = NULL;
    }
    
    /* Disconnect if connected */
    if (app->proto && sdr_is_connected(app->proto)) {
        sdr_disconnect(app->proto);
    }
    
    /* Destroy components in reverse order */
    if (app->layout) {
        ui_layout_destroy(app->layout);
        app->layout = NULL;
    }
    
    if (app->ui) {
        ui_core_shutdown(app->ui);
        app->ui = NULL;
    }
    
    if (app->state) {
        /* Save presets to file before cleanup */
        app_save_presets_to_file(app->state, PRESETS_FILENAME);
        
        app_state_destroy(app->state);
        app->state = NULL;
    }
    
    if (app->proto) {
        sdr_protocol_destroy(app->proto);
        app->proto = NULL;
    }
    
    if (app->tcp) {
        tcp_client_destroy(app->tcp);
        app->tcp = NULL;
    }
    
    /* Cleanup TCP subsystem */
    tcp_client_cleanup();
}

/*
 * Handle UI actions
 */
static void app_handle_actions(app_context_t* app, const ui_actions_t* actions)
{
    if (!app || !actions) return;
    
    /* Connection actions */
    if (actions->connect_clicked) {
        app_connect(app);
    }
    
    if (actions->disconnect_clicked) {
        app_disconnect(app);
    }
    
    /* Only process other actions if connected */
    if (!sdr_is_connected(app->proto)) {
        return;
    }
    
    /* Streaming control */
    if (actions->start_clicked) {
        if (sdr_start(app->proto)) {
            app->state->streaming = true;
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Streaming started");
            LOG_INFO("Streaming started");
        } else {
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Start failed: %s", sdr_get_error_msg(app->proto));
            LOG_ERROR("Failed to start streaming: %s", sdr_get_error_msg(app->proto));
        }
    }
    
    if (actions->stop_clicked) {
        if (sdr_stop(app->proto)) {
            app->state->streaming = false;
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Streaming stopped");
            LOG_INFO("Streaming stopped");
        } else {
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Stop failed: %s", sdr_get_error_msg(app->proto));
        }
    }
    
    /* Frequency control */
    if (actions->freq_changed) {
        /* Apply DC offset when sending to server if enabled */
        int64_t actual_freq = actions->new_frequency + 
                              (app->state->dc_offset_enabled ? DC_OFFSET_HZ : 0);
        if (sdr_set_freq(app->proto, actual_freq)) {
            app->state->frequency = actions->new_frequency;
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Frequency: %s", app_format_frequency(actions->new_frequency));
        }
    }
    
    if (actions->freq_up) {
        int64_t new_display_freq = app->state->frequency + (int64_t)app->state->tuning_step;
        int64_t actual_freq = new_display_freq + 
                              (app->state->dc_offset_enabled ? DC_OFFSET_HZ : 0);
        if (new_display_freq <= FREQ_MAX && sdr_set_freq(app->proto, actual_freq)) {
            app->state->frequency = new_display_freq;
        }
    }
    
    if (actions->freq_down) {
        int64_t new_display_freq = app->state->frequency - (int64_t)app->state->tuning_step;
        int64_t actual_freq = new_display_freq + 
                              (app->state->dc_offset_enabled ? DC_OFFSET_HZ : 0);
        if (new_display_freq >= FREQ_MIN && sdr_set_freq(app->proto, actual_freq)) {
            app->state->frequency = new_display_freq;
        }
    }
    
    /* Tuning step control (local only, doesn't send to server) */
    if (actions->step_up) {
        app->state->tuning_step = app_next_step(app->state->tuning_step);
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "Step: %s", app_get_step_string(app->state->tuning_step));
    }
    
    if (actions->step_down) {
        app->state->tuning_step = app_prev_step(app->state->tuning_step);
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "Step: %s", app_get_step_string(app->state->tuning_step));
    }
    
    /* DC Offset toggle (local only, doesn't require connection) */
    if (actions->dc_offset_toggled) {
        app->state->dc_offset_enabled = !app->state->dc_offset_enabled;
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "DC Offset: %s (%+d Hz)", 
                 app->state->dc_offset_enabled ? "ON" : "OFF",
                 DC_OFFSET_HZ);
        LOG_INFO("DC Offset toggled: %s", app->state->dc_offset_enabled ? "ON" : "OFF");
    }
    
    /* WWV frequency shortcuts */
    if (actions->wwv_clicked) {
        int64_t display_freq = actions->wwv_frequency;
        /* Apply DC offset when sending to server if enabled */
        int64_t actual_freq = display_freq + (app->state->dc_offset_enabled ? DC_OFFSET_HZ : 0);
        
        if (sdr_is_connected(app->proto)) {
            if (sdr_set_freq(app->proto, actual_freq)) {
                app->state->frequency = display_freq;
                snprintf(app->state->status_message, sizeof(app->state->status_message),
                         "Tuned to WWV %s", app_format_frequency(display_freq));
                LOG_INFO("WWV tune: display=%lld Hz, actual=%lld Hz", 
                         (long long)display_freq, (long long)actual_freq);
            }
        } else {
            /* Not connected - just update local display */
            app->state->frequency = display_freq;
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "WWV preset: %s (not connected)", app_format_frequency(display_freq));
        }
    }
    
    /* Memory preset handling */
    if (actions->preset_clicked) {
        int slot = actions->preset_index;
        if (actions->preset_save) {
            /* Ctrl+click = Save current settings to preset */
            app_save_preset(app->state, slot);
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Saved M%d: %s", slot + 1, app_format_frequency(app->state->frequency));
        } else {
            /* Click = Recall preset */
            if (app_recall_preset(app->state, slot)) {
                /* Apply preset settings to server if connected */
                if (sdr_is_connected(app->proto)) {
                    /* Apply DC offset to frequency when sending */
                    int64_t actual_freq = app->state->frequency + 
                                          (app->state->dc_offset_enabled ? DC_OFFSET_HZ : 0);
                    sdr_set_freq(app->proto, actual_freq);
                    sdr_set_gain(app->proto, app->state->gain);
                    sdr_set_lna(app->proto, app->state->lna);
                    sdr_set_agc(app->proto, app->state->agc);
                    if (!app->state->streaming) {
                        sdr_set_srate(app->proto, app->state->sample_rate);
                        sdr_set_bw(app->proto, app->state->bandwidth);
                    }
                    sdr_set_antenna(app->proto, app->state->antenna);
                    sdr_set_notch(app->proto, app->state->notch);
                }
                snprintf(app->state->status_message, sizeof(app->state->status_message),
                         "Recalled M%d: %s", slot + 1, app_format_frequency(app->state->frequency));
            } else {
                snprintf(app->state->status_message, sizeof(app->state->status_message),
                         "M%d is empty (Ctrl+click to save)", slot + 1);
            }
        }
    }
    
    /* External process control */
    if (actions->server_toggled) {
        bool running = process_manager_toggle(&app->proc_mgr, PROC_SDR_SERVER);
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "SDR Server %s", running ? "started" : "stopped");
    }
    
    if (actions->waterfall_toggled) {
        bool running = process_manager_toggle(&app->proc_mgr, PROC_WATERFALL);
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "Waterfall %s", running ? "started" : "stopped");
    }
    
    /* Gain control - always update local state, send to server if connected */
    if (actions->gain_changed) {
        app->state->gain = actions->new_gain;
        if (sdr_is_connected(app->proto)) {
            sdr_set_gain(app->proto, actions->new_gain);
        }
    }
    
    if (actions->lna_changed) {
        app->state->lna = actions->new_lna;
        if (sdr_is_connected(app->proto)) {
            sdr_set_lna(app->proto, actions->new_lna);
        }
    }
    
    /* AGC control */
    if (actions->agc_changed) {
        app->state->agc = actions->new_agc;
        if (sdr_is_connected(app->proto)) {
            sdr_set_agc(app->proto, actions->new_agc);
        }
    }
    
    /* Sample rate and bandwidth (only when not streaming) */
    if (actions->srate_changed && !app->state->streaming) {
        app->state->sample_rate = actions->new_srate;
        if (sdr_is_connected(app->proto)) {
            sdr_set_srate(app->proto, actions->new_srate);
        }
    }
    
    if (actions->bw_changed && !app->state->streaming) {
        app->state->bandwidth = actions->new_bw;
        if (sdr_is_connected(app->proto)) {
            sdr_set_bw(app->proto, actions->new_bw);
        }
    }
    
    /* Antenna control */
    if (actions->antenna_changed) {
        app->state->antenna = actions->new_antenna;
        LOG_INFO("Antenna changed to %d", actions->new_antenna);
        
        /* Hi-Z antenna has reduced LNA states (0-4 vs 0-8 for A/B) */
        if (actions->new_antenna == ANTENNA_HIZ) {
            if (app->state->lna > LNA_MAX_HIZ) {
                app->state->lna = LNA_MAX_HIZ;
                LOG_INFO("LNA clamped to %d for Hi-Z antenna", LNA_MAX_HIZ);
                if (sdr_is_connected(app->proto)) {
                    sdr_set_lna(app->proto, app->state->lna);
                }
            }
        }
        
        if (sdr_is_connected(app->proto)) {
            sdr_set_antenna(app->proto, actions->new_antenna);
        }
    }
    
    /* Hardware toggles */
    if (actions->biast_changed) {
        app->state->bias_t = actions->new_biast;
        if (actions->new_biast) {
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "WARNING: Bias-T enabled - DC voltage on antenna!");
            LOG_WARN("Bias-T enabled");
        }
        if (sdr_is_connected(app->proto)) {
            sdr_set_biast(app->proto, actions->new_biast);
        }
    }
    
    if (actions->notch_changed) {
        app->state->notch = actions->new_notch;
        if (sdr_is_connected(app->proto)) {
            sdr_set_notch(app->proto, actions->new_notch);
        }
    }
}

/*
 * Periodic tasks
 */
static void app_periodic_tasks(app_context_t* app)
{
    if (!app || !app->state) return;
    
    uint32_t now = ui_get_ticks();
    
    if (sdr_is_connected(app->proto)) {
        /* Status polling */
        if (now - app->state->last_status_update >= STATUS_POLL_INTERVAL_MS) {
            app->state->last_status_update = now;
            
            if (sdr_get_status(app->proto)) {
                app_state_update_from_sdr(app->state, &app->proto->status);
            }
        }
        
        /* Keepalive ping (when not actively polling) */
        if (!app->state->streaming && 
            now - app->state->last_keepalive >= KEEPALIVE_INTERVAL_MS) {
            app->state->last_keepalive = now;
            
            if (!sdr_ping(app->proto)) {
                LOG_WARN("Keepalive ping failed");
                app->state->conn_state = CONN_ERROR;
                snprintf(app->state->status_message, sizeof(app->state->status_message),
                         "Connection lost");
            }
        }
    }
}

/*
 * Connect to SDR server
 */
static void app_connect(app_context_t* app)
{
    if (!app) return;
    
    snprintf(app->state->status_message, sizeof(app->state->status_message),
             "Connecting to %s:%d...", app->state->server_host, app->state->server_port);
    
    if (sdr_connect(app->proto, app->state->server_host, app->state->server_port)) {
        app->state->conn_state = CONN_CONNECTED;
        
        /* Get version info */
        if (sdr_get_version(app->proto)) {
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Connected - Phoenix SDR v%s", app->proto->version.phoenix_version);
        } else {
            snprintf(app->state->status_message, sizeof(app->state->status_message),
                     "Connected");
        }
        
        /* Get initial status and sync UI to server state */
        if (sdr_get_status(app->proto)) {
            app_state_update_from_sdr(app->state, &app->proto->status);
        }
        
        /* UI now reflects server state - no need to push settings back */
        
        app->state->last_status_update = ui_get_ticks();
        app->state->last_keepalive = ui_get_ticks();
        
        LOG_INFO("Connected to %s:%d", app->state->server_host, app->state->server_port);
    } else {
        app->state->conn_state = CONN_ERROR;
        snprintf(app->state->status_message, sizeof(app->state->status_message),
                 "Connection failed: %s", tcp_client_get_error(app->tcp));
        LOG_ERROR("Connection failed: %s", tcp_client_get_error(app->tcp));
    }
}

/*
 * Disconnect from SDR server
 */
static void app_disconnect(app_context_t* app)
{
    if (!app) return;
    
    sdr_disconnect(app->proto);
    app->state->conn_state = CONN_DISCONNECTED;
    app->state->streaming = false;
    app->state->overload = false;
    
    snprintf(app->state->status_message, sizeof(app->state->status_message),
             "Disconnected");
    
    LOG_INFO("Disconnected");
}
