/**
 * Phoenix SDR Controller - SDR Protocol Handler
 * 
 * Implements the Phoenix SDR TCP command protocol.
 */

#ifndef SDR_PROTOCOL_H
#define SDR_PROTOCOL_H

#include "common.h"
#include "tcp_client.h"

/* SDR Capabilities structure */
typedef struct {
    int64_t freq_min;
    int64_t freq_max;
    int gain_min;
    int gain_max;
    int lna_states;
    int srate_min;
    int srate_max;
    int bandwidths[16];
    int bandwidth_count;
    char antennas[8][8];
    int antenna_count;
    char agc_modes[8][16];
    int agc_mode_count;
} sdr_capabilities_t;

/* SDR Status structure */
typedef struct {
    bool streaming;
    int64_t frequency;
    int gain;
    int lna;
    agc_mode_t agc;
    int sample_rate;
    int bandwidth;
    bool overload;
    antenna_port_t antenna;
    bool bias_t;
    bool notch;
} sdr_status_t;

/* SDR Version info */
typedef struct {
    char phoenix_version[32];
    char protocol_version[16];
    char api_version[16];
} sdr_version_t;

/* Protocol handler context */
typedef struct {
    tcp_client_t* client;
    sdr_status_t status;
    sdr_capabilities_t caps;
    sdr_version_t version;
    error_code_t last_error;
    char last_error_msg[256];
    bool caps_loaded;
    bool version_loaded;
} sdr_protocol_t;

/* Create protocol handler */
sdr_protocol_t* sdr_protocol_create(tcp_client_t* client);

/* Destroy protocol handler */
void sdr_protocol_destroy(sdr_protocol_t* proto);

/* Connection commands */
bool sdr_connect(sdr_protocol_t* proto, const char* host, int port);
void sdr_disconnect(sdr_protocol_t* proto);
bool sdr_is_connected(sdr_protocol_t* proto);

/* Utility commands */
bool sdr_ping(sdr_protocol_t* proto);
bool sdr_get_version(sdr_protocol_t* proto);
bool sdr_get_caps(sdr_protocol_t* proto);
bool sdr_get_status(sdr_protocol_t* proto);
bool sdr_quit(sdr_protocol_t* proto);

/* Frequency commands */
bool sdr_set_freq(sdr_protocol_t* proto, int64_t freq_hz);
bool sdr_get_freq(sdr_protocol_t* proto, int64_t* freq_hz);

/* Gain commands */
bool sdr_set_gain(sdr_protocol_t* proto, int gain_db);
bool sdr_get_gain(sdr_protocol_t* proto, int* gain_db);
bool sdr_set_lna(sdr_protocol_t* proto, int lna_state);
bool sdr_get_lna(sdr_protocol_t* proto, int* lna_state);
bool sdr_set_agc(sdr_protocol_t* proto, agc_mode_t mode);
bool sdr_get_agc(sdr_protocol_t* proto, agc_mode_t* mode);

/* Sample rate and bandwidth */
bool sdr_set_srate(sdr_protocol_t* proto, int srate_hz);
bool sdr_get_srate(sdr_protocol_t* proto, int* srate_hz);
bool sdr_set_bw(sdr_protocol_t* proto, int bw_khz);
bool sdr_get_bw(sdr_protocol_t* proto, int* bw_khz);

/* Hardware configuration */
bool sdr_set_antenna(sdr_protocol_t* proto, antenna_port_t port);
bool sdr_get_antenna(sdr_protocol_t* proto, antenna_port_t* port);
bool sdr_set_biast(sdr_protocol_t* proto, bool enable);
bool sdr_set_notch(sdr_protocol_t* proto, bool enable);

/* Streaming control */
bool sdr_start(sdr_protocol_t* proto);
bool sdr_stop(sdr_protocol_t* proto);

/* Process async notifications */
bool sdr_process_async(sdr_protocol_t* proto);

/* Get last error */
error_code_t sdr_get_error(sdr_protocol_t* proto);
const char* sdr_get_error_msg(sdr_protocol_t* proto);

/* Helper functions */
const char* agc_mode_to_string(agc_mode_t mode);
agc_mode_t string_to_agc_mode(const char* str);
const char* antenna_to_string(antenna_port_t port);
antenna_port_t string_to_antenna(const char* str);

#endif /* SDR_PROTOCOL_H */
