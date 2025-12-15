/**
 * Phoenix SDR Controller - SDR Protocol Implementation
 * 
 * Implements the Phoenix SDR TCP command protocol.
 */

#include "sdr_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Response buffer size */
#define RESPONSE_BUF_SIZE 1024

/* Helper: parse error code from response */
static error_code_t parse_error_code(const char* err_str)
{
    if (!err_str) return ERR_UNKNOWN;
    
    if (strncmp(err_str, "SYNTAX", 6) == 0) return ERR_SYNTAX;
    if (strncmp(err_str, "UNKNOWN", 7) == 0) return ERR_UNKNOWN;
    if (strncmp(err_str, "PARAM", 5) == 0) return ERR_PARAM;
    if (strncmp(err_str, "RANGE", 5) == 0) return ERR_RANGE;
    if (strncmp(err_str, "STATE", 5) == 0) return ERR_STATE;
    if (strncmp(err_str, "BUSY", 4) == 0) return ERR_BUSY;
    if (strncmp(err_str, "HARDWARE", 8) == 0) return ERR_HARDWARE;
    if (strncmp(err_str, "TIMEOUT", 7) == 0) return ERR_TIMEOUT;
    
    return ERR_UNKNOWN;
}

/* Helper: check if response is OK */
static bool is_response_ok(const char* response)
{
    return response && (strncmp(response, "OK", 2) == 0);
}

/* Helper: check if response is error */
static bool is_response_error(const char* response)
{
    return response && (strncmp(response, "ERR", 3) == 0);
}

/* Helper: set error from response */
static void set_error_from_response(sdr_protocol_t* proto, const char* response)
{
    if (!proto || !response) return;
    
    if (strncmp(response, "ERR ", 4) == 0) {
        const char* err_start = response + 4;
        proto->last_error = parse_error_code(err_start);
        
        /* Find and copy error message */
        const char* msg = strchr(err_start, ' ');
        if (msg) {
            strncpy(proto->last_error_msg, msg + 1, sizeof(proto->last_error_msg) - 1);
        } else {
            strncpy(proto->last_error_msg, err_start, sizeof(proto->last_error_msg) - 1);
        }
    } else {
        proto->last_error = ERR_UNKNOWN;
        strncpy(proto->last_error_msg, response, sizeof(proto->last_error_msg) - 1);
    }
    proto->last_error_msg[sizeof(proto->last_error_msg) - 1] = '\0';
}

/* Helper: parse key=value from status response */
static bool parse_status_value(const char* response, const char* key, char* value, size_t value_size)
{
    if (!response || !key || !value || value_size == 0) return false;
    
    /* Find key in response */
    const char* ptr = strstr(response, key);
    if (!ptr) return false;
    
    /* Move past key and = */
    ptr += strlen(key);
    if (*ptr != '=') return false;
    ptr++;
    
    /* Copy value until space or end */
    size_t i = 0;
    while (*ptr && *ptr != ' ' && *ptr != '\n' && i < value_size - 1) {
        value[i++] = *ptr++;
    }
    value[i] = '\0';
    
    return i > 0;
}

/* Helper: parse integer from status */
static bool parse_status_int(const char* response, const char* key, int* result)
{
    char value[32];
    if (parse_status_value(response, key, value, sizeof(value))) {
        *result = atoi(value);
        return true;
    }
    return false;
}

/* Helper: parse int64 from status */
static bool parse_status_int64(const char* response, const char* key, int64_t* result)
{
    char value[32];
    if (parse_status_value(response, key, value, sizeof(value))) {
        *result = strtoll(value, NULL, 10);
        return true;
    }
    return false;
}

/*
 * AGC mode to string
 */
const char* agc_mode_to_string(agc_mode_t mode)
{
    switch (mode) {
        case AGC_OFF:   return "OFF";
        case AGC_5HZ:   return "5HZ";
        case AGC_50HZ:  return "50HZ";
        case AGC_100HZ: return "100HZ";
        default:        return "OFF";
    }
}

/*
 * String to AGC mode
 */
agc_mode_t string_to_agc_mode(const char* str)
{
    if (!str) return AGC_OFF;
    
    if (strcmp(str, "OFF") == 0) return AGC_OFF;
    if (strcmp(str, "5HZ") == 0) return AGC_5HZ;
    if (strcmp(str, "50HZ") == 0) return AGC_50HZ;
    if (strcmp(str, "100HZ") == 0) return AGC_100HZ;
    
    return AGC_OFF;
}

/*
 * Antenna to string
 */
const char* antenna_to_string(antenna_port_t port)
{
    switch (port) {
        case ANTENNA_A:   return "A";
        case ANTENNA_B:   return "B";
        case ANTENNA_HIZ: return "HIZ";
        default:          return "A";
    }
}

/*
 * String to antenna
 */
antenna_port_t string_to_antenna(const char* str)
{
    if (!str) return ANTENNA_A;
    
    if (strcmp(str, "A") == 0) return ANTENNA_A;
    if (strcmp(str, "B") == 0) return ANTENNA_B;
    if (strcmp(str, "HIZ") == 0) return ANTENNA_HIZ;
    
    return ANTENNA_A;
}

/*
 * Create protocol handler
 */
sdr_protocol_t* sdr_protocol_create(tcp_client_t* client)
{
    sdr_protocol_t* proto = (sdr_protocol_t*)calloc(1, sizeof(sdr_protocol_t));
    if (!proto) {
        LOG_ERROR("Failed to allocate sdr_protocol_t");
        return NULL;
    }
    
    proto->client = client;
    proto->last_error = ERR_NONE;
    proto->last_error_msg[0] = '\0';
    proto->caps_loaded = false;
    proto->version_loaded = false;
    
    /* Initialize status with defaults */
    proto->status.streaming = false;
    proto->status.frequency = 15000000;
    proto->status.gain = 40;
    proto->status.lna = 4;
    proto->status.agc = AGC_OFF;
    proto->status.sample_rate = 2000000;
    proto->status.bandwidth = 200;
    proto->status.overload = false;
    proto->status.antenna = ANTENNA_A;
    proto->status.bias_t = false;
    proto->status.notch = false;
    
    return proto;
}

/*
 * Destroy protocol handler
 */
void sdr_protocol_destroy(sdr_protocol_t* proto)
{
    if (proto) {
        free(proto);
    }
}

/*
 * Connect to SDR server
 */
bool sdr_connect(sdr_protocol_t* proto, const char* host, int port)
{
    if (!proto || !proto->client) {
        return false;
    }
    
    return tcp_client_connect(proto->client, host, port);
}

/*
 * Disconnect from SDR server
 */
void sdr_disconnect(sdr_protocol_t* proto)
{
    if (proto && proto->client) {
        /* Try to send QUIT first */
        sdr_quit(proto);
        tcp_client_disconnect(proto->client);
    }
}

/*
 * Check if connected
 */
bool sdr_is_connected(sdr_protocol_t* proto)
{
    return proto && proto->client && tcp_client_is_connected(proto->client);
}

/*
 * PING - Connection keepalive
 */
bool sdr_ping(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "PING", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    return (strcmp(response, "PONG") == 0);
}

/*
 * VER - Get version information
 */
bool sdr_get_version(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "VER", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse: OK PHOENIX_SDR=x.x.x PROTOCOL=x.x API=x.xx */
    parse_status_value(response, "PHOENIX_SDR", proto->version.phoenix_version, 
                       sizeof(proto->version.phoenix_version));
    parse_status_value(response, "PROTOCOL", proto->version.protocol_version,
                       sizeof(proto->version.protocol_version));
    parse_status_value(response, "API", proto->version.api_version,
                       sizeof(proto->version.api_version));
    
    proto->version_loaded = true;
    LOG_INFO("SDR Version: %s, Protocol: %s, API: %s",
             proto->version.phoenix_version,
             proto->version.protocol_version,
             proto->version.api_version);
    
    return true;
}

/*
 * CAPS - Get capabilities (stub for Phase 2)
 */
bool sdr_get_caps(sdr_protocol_t* proto)
{
    /* TODO: Implement multi-line CAPS parsing in Phase 2 */
    if (!sdr_is_connected(proto)) return false;
    
    /* For now, use hardcoded RSP2 capabilities */
    proto->caps.freq_min = FREQ_MIN;
    proto->caps.freq_max = FREQ_MAX;
    proto->caps.gain_min = GAIN_MIN;
    proto->caps.gain_max = GAIN_MAX;
    proto->caps.lna_states = 9;
    proto->caps.srate_min = SRATE_MIN;
    proto->caps.srate_max = SRATE_MAX;
    
    proto->caps_loaded = true;
    return true;
}

/*
 * STATUS - Get streaming status
 */
bool sdr_get_status(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "STATUS", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse STATUS response: OK STREAMING=x FREQ=x GAIN=x LNA=x AGC=x SRATE=x BW=x [OVERLOAD=x] */
    int streaming = 0;
    parse_status_int(response, "STREAMING", &streaming);
    proto->status.streaming = (streaming != 0);
    
    parse_status_int64(response, "FREQ", &proto->status.frequency);
    parse_status_int(response, "GAIN", &proto->status.gain);
    parse_status_int(response, "LNA", &proto->status.lna);
    parse_status_int(response, "SRATE", &proto->status.sample_rate);
    parse_status_int(response, "BW", &proto->status.bandwidth);
    
    int overload = 0;
    if (parse_status_int(response, "OVERLOAD", &overload)) {
        proto->status.overload = (overload != 0);
    }
    
    char agc_str[16];
    if (parse_status_value(response, "AGC", agc_str, sizeof(agc_str))) {
        proto->status.agc = string_to_agc_mode(agc_str);
    }
    
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * QUIT - Graceful disconnect
 */
bool sdr_quit(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "QUIT", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    return (strcmp(response, "BYE") == 0);
}

/*
 * SET_FREQ - Set center frequency
 */
bool sdr_set_freq(sdr_protocol_t* proto, int64_t freq_hz)
{
    if (!sdr_is_connected(proto)) return false;
    
    /* Validate range */
    if (freq_hz < FREQ_MIN || freq_hz > FREQ_MAX) {
        proto->last_error = ERR_RANGE;
        snprintf(proto->last_error_msg, sizeof(proto->last_error_msg),
                 "Frequency out of range: %lld", freq_hz);
        return false;
    }
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_FREQ %lld", freq_hz);
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.frequency = freq_hz;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_FREQ - Get current frequency
 */
bool sdr_get_freq(sdr_protocol_t* proto, int64_t* freq_hz)
{
    if (!sdr_is_connected(proto) || !freq_hz) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_FREQ", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse: OK <freq> */
    if (strlen(response) > 3) {
        *freq_hz = strtoll(response + 3, NULL, 10);
        proto->status.frequency = *freq_hz;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_GAIN - Set gain reduction
 */
bool sdr_set_gain(sdr_protocol_t* proto, int gain_db)
{
    if (!sdr_is_connected(proto)) return false;
    
    /* Validate range */
    if (gain_db < GAIN_MIN || gain_db > GAIN_MAX) {
        proto->last_error = ERR_RANGE;
        snprintf(proto->last_error_msg, sizeof(proto->last_error_msg),
                 "Gain out of range: %d (must be %d-%d)", gain_db, GAIN_MIN, GAIN_MAX);
        return false;
    }
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_GAIN %d", gain_db);
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.gain = gain_db;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_GAIN - Get current gain reduction
 */
bool sdr_get_gain(sdr_protocol_t* proto, int* gain_db)
{
    if (!sdr_is_connected(proto) || !gain_db) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_GAIN", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse: OK <gain> */
    if (strlen(response) > 3) {
        *gain_db = atoi(response + 3);
        proto->status.gain = *gain_db;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_LNA - Set LNA state
 */
bool sdr_set_lna(sdr_protocol_t* proto, int lna_state)
{
    if (!sdr_is_connected(proto)) return false;
    
    /* Validate range */
    if (lna_state < LNA_MIN || lna_state > LNA_MAX) {
        proto->last_error = ERR_RANGE;
        snprintf(proto->last_error_msg, sizeof(proto->last_error_msg),
                 "LNA state out of range: %d (must be %d-%d)", lna_state, LNA_MIN, LNA_MAX);
        return false;
    }
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_LNA %d", lna_state);
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.lna = lna_state;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_LNA - Get current LNA state
 */
bool sdr_get_lna(sdr_protocol_t* proto, int* lna_state)
{
    if (!sdr_is_connected(proto) || !lna_state) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_LNA", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse: OK <lna> */
    if (strlen(response) > 3) {
        *lna_state = atoi(response + 3);
        proto->status.lna = *lna_state;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_AGC - Set AGC mode
 */
bool sdr_set_agc(sdr_protocol_t* proto, agc_mode_t mode)
{
    if (!sdr_is_connected(proto)) return false;
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_AGC %s", agc_mode_to_string(mode));
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.agc = mode;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_AGC - Get current AGC mode
 */
bool sdr_get_agc(sdr_protocol_t* proto, agc_mode_t* mode)
{
    if (!sdr_is_connected(proto) || !mode) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_AGC", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    /* Parse: OK <mode> */
    if (strlen(response) > 3) {
        *mode = string_to_agc_mode(response + 3);
        proto->status.agc = *mode;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_SRATE - Set sample rate
 */
bool sdr_set_srate(sdr_protocol_t* proto, int srate_hz)
{
    if (!sdr_is_connected(proto)) return false;
    
    /* Validate range */
    if (srate_hz < SRATE_MIN || srate_hz > SRATE_MAX) {
        proto->last_error = ERR_RANGE;
        snprintf(proto->last_error_msg, sizeof(proto->last_error_msg),
                 "Sample rate out of range: %d", srate_hz);
        return false;
    }
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_SRATE %d", srate_hz);
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.sample_rate = srate_hz;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_SRATE - Get current sample rate
 */
bool sdr_get_srate(sdr_protocol_t* proto, int* srate_hz)
{
    if (!sdr_is_connected(proto) || !srate_hz) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_SRATE", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    if (strlen(response) > 3) {
        *srate_hz = atoi(response + 3);
        proto->status.sample_rate = *srate_hz;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_BW - Set IF bandwidth
 */
bool sdr_set_bw(sdr_protocol_t* proto, int bw_khz)
{
    if (!sdr_is_connected(proto)) return false;
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_BW %d", bw_khz);
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.bandwidth = bw_khz;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_BW - Get current bandwidth
 */
bool sdr_get_bw(sdr_protocol_t* proto, int* bw_khz)
{
    if (!sdr_is_connected(proto) || !bw_khz) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_BW", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    if (strlen(response) > 3) {
        *bw_khz = atoi(response + 3);
        proto->status.bandwidth = *bw_khz;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_ANTENNA - Set antenna port
 */
bool sdr_set_antenna(sdr_protocol_t* proto, antenna_port_t port)
{
    if (!sdr_is_connected(proto)) return false;
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_ANTENNA %s", antenna_to_string(port));
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.antenna = port;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * GET_ANTENNA - Get current antenna
 */
bool sdr_get_antenna(sdr_protocol_t* proto, antenna_port_t* port)
{
    if (!sdr_is_connected(proto) || !port) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "GET_ANTENNA", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    if (strlen(response) > 3) {
        *port = string_to_antenna(response + 3);
        proto->status.antenna = *port;
        proto->last_error = ERR_NONE;
        return true;
    }
    
    return false;
}

/*
 * SET_BIAST - Enable/disable Bias-T (requires CONFIRM for ON)
 */
bool sdr_set_biast(sdr_protocol_t* proto, bool enable)
{
    if (!sdr_is_connected(proto)) return false;
    
    char cmd[MAX_CMD_LENGTH];
    if (enable) {
        snprintf(cmd, sizeof(cmd), "SET_BIAST ON CONFIRM");
    } else {
        snprintf(cmd, sizeof(cmd), "SET_BIAST OFF");
    }
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.bias_t = enable;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * SET_NOTCH - Enable/disable FM notch
 */
bool sdr_set_notch(sdr_protocol_t* proto, bool enable)
{
    if (!sdr_is_connected(proto)) return false;
    
    char cmd[MAX_CMD_LENGTH];
    snprintf(cmd, sizeof(cmd), "SET_NOTCH %s", enable ? "ON" : "OFF");
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, cmd, response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.notch = enable;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * START - Start streaming
 */
bool sdr_start(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "START", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.streaming = true;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * STOP - Stop streaming
 */
bool sdr_stop(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char response[RESPONSE_BUF_SIZE];
    if (!tcp_client_send_receive(proto->client, "STOP", response, sizeof(response), SOCKET_TIMEOUT_MS)) {
        return false;
    }
    
    if (!is_response_ok(response)) {
        set_error_from_response(proto, response);
        return false;
    }
    
    proto->status.streaming = false;
    proto->status.overload = false;
    proto->last_error = ERR_NONE;
    return true;
}

/*
 * Process async notifications (non-blocking)
 */
bool sdr_process_async(sdr_protocol_t* proto)
{
    if (!sdr_is_connected(proto)) return false;
    
    char buffer[RESPONSE_BUF_SIZE];
    
    /* Check for pending data */
    while (tcp_client_check_async(proto->client, buffer, sizeof(buffer))) {
        /* Check if it's a notification (starts with '!') */
        if (buffer[0] == '!') {
            LOG_DEBUG("Async notification: %s", buffer);
            
            /* Parse notification */
            if (strstr(buffer, "OVERLOAD DETECTED")) {
                proto->status.overload = true;
                LOG_WARN("ADC Overload detected!");
            } else if (strstr(buffer, "OVERLOAD CLEARED")) {
                proto->status.overload = false;
                LOG_INFO("ADC Overload cleared");
            } else if (strstr(buffer, "GAIN_CHANGE")) {
                /* Parse: ! GAIN_CHANGE GAIN=x LNA_GR=y 
                 * Note: LNA_GR is gain reduction in dB from AGC, not the LNA state index.
                 * We update status.gain but NOT status.lna (which is the user-set state 0-9).
                 */
                int gain, lna_gr;
                if (parse_status_int(buffer, "GAIN", &gain)) {
                    proto->status.gain = gain;
                }
                /* Parse LNA_GR for logging, but don't overwrite lna state */
                if (parse_status_int(buffer, "LNA_GR", &lna_gr)) {
                    LOG_INFO("AGC adjusted: GAIN=%d LNA_GR=%d dB", gain, lna_gr);
                } else {
                    LOG_INFO("AGC adjusted: GAIN=%d", proto->status.gain);
                }
            } else if (strstr(buffer, "DISCONNECT")) {
                LOG_WARN("Server disconnect notification: %s", buffer);
                /* Connection will be closed by server */
            }
        }
    }
    
    return true;
}

/*
 * Get last error code
 */
error_code_t sdr_get_error(sdr_protocol_t* proto)
{
    if (!proto) return ERR_UNKNOWN;
    return proto->last_error;
}

/*
 * Get last error message
 */
const char* sdr_get_error_msg(sdr_protocol_t* proto)
{
    if (!proto) return "NULL protocol";
    return proto->last_error_msg;
}
