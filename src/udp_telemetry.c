/**
 * Phoenix SDR Controller - UDP Telemetry Receiver Implementation
 * 
 * Receives and parses WWV signal statistics broadcast on UDP.
 */

#include "udp_telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#endif

/* Helper: Get current time in ms */
static uint32_t get_time_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

/* Helper: Parse quality string */
static signal_quality_t parse_quality(const char* str)
{
    if (!str) return QUALITY_NONE;
    if (strcmp(str, "GOOD") == 0) return QUALITY_GOOD;
    if (strcmp(str, "FAIR") == 0) return QUALITY_FAIR;
    if (strcmp(str, "POOR") == 0) return QUALITY_POOR;
    return QUALITY_NONE;
}

/* Helper: Parse subcarrier string */
static subcarrier_t parse_subcarrier(const char* str)
{
    if (!str) return SUBCAR_NONE;
    if (strcmp(str, "500Hz") == 0 || strcmp(str, "500") == 0) return SUBCAR_500HZ;
    if (strcmp(str, "600Hz") == 0 || strcmp(str, "600") == 0) return SUBCAR_600HZ;
    return SUBCAR_NONE;
}

/* Helper: Parse YES/NO string */
static bool parse_yes_no(const char* str)
{
    if (!str) return false;
    return (strcmp(str, "YES") == 0 || strcmp(str, "1") == 0);
}

/*
 * Create telemetry receiver
 */
udp_telemetry_t* udp_telemetry_create(int port)
{
    udp_telemetry_t* telem = (udp_telemetry_t*)calloc(1, sizeof(udp_telemetry_t));
    if (!telem) {
        LOG_ERROR("Failed to allocate udp_telemetry_t");
        return NULL;
    }
    
    telem->socket = INVALID_SOCK;
    telem->port = (port > 0) ? port : TELEMETRY_UDP_PORT;
    telem->bound = false;
    
    LOG_INFO("UDP telemetry receiver created for port %d", telem->port);
    return telem;
}

/*
 * Destroy telemetry receiver
 */
void udp_telemetry_destroy(udp_telemetry_t* telem)
{
    if (!telem) return;
    
    udp_telemetry_stop(telem);
    free(telem);
    
    LOG_INFO("UDP telemetry receiver destroyed");
}

/*
 * Start listening
 */
bool udp_telemetry_start(udp_telemetry_t* telem)
{
    if (!telem) return false;
    
    if (telem->bound) {
        LOG_WARN("Telemetry already listening");
        return true;
    }
    
    /* Create UDP socket */
    telem->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (telem->socket == INVALID_SOCK) {
        LOG_ERROR("Failed to create UDP socket: %d", SOCKET_ERROR_CODE);
        return false;
    }
    
    /* Allow address reuse */
    int reuse = 1;
    setsockopt(telem->socket, SOL_SOCKET, SO_REUSEADDR, 
               (const char*)&reuse, sizeof(reuse));
    
    /* Bind to port */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)telem->port);
    
    if (bind(telem->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind UDP socket to port %d: %d", 
                  telem->port, SOCKET_ERROR_CODE);
        CLOSE_SOCKET(telem->socket);
        telem->socket = INVALID_SOCK;
        return false;
    }
    
    /* Set non-blocking mode */
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(telem->socket, FIONBIO, &mode);
#else
    int flags = fcntl(telem->socket, F_GETFL, 0);
    fcntl(telem->socket, F_SETFL, flags | O_NONBLOCK);
#endif
    
    telem->bound = true;
    LOG_INFO("UDP telemetry listening on port %d", telem->port);
    return true;
}

/*
 * Stop listening
 */
void udp_telemetry_stop(udp_telemetry_t* telem)
{
    if (!telem) return;
    
    if (telem->socket != INVALID_SOCK) {
        CLOSE_SOCKET(telem->socket);
        telem->socket = INVALID_SOCK;
    }
    
    telem->bound = false;
    LOG_INFO("UDP telemetry stopped");
}

/*
 * Poll for incoming packets
 */
int udp_telemetry_poll(udp_telemetry_t* telem)
{
    if (!telem || !telem->bound) return 0;
    
    char buffer[TELEMETRY_MAX_PACKET];
    struct sockaddr_in sender;
    int sender_len = sizeof(sender);
    int packets = 0;
    
    /* Read all available packets */
    while (1) {
        int len = recvfrom(telem->socket, buffer, sizeof(buffer) - 1, 0,
                          (struct sockaddr*)&sender, &sender_len);
        
        if (len <= 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                LOG_DEBUG("recvfrom error: %d", err);
            }
#endif
            break;
        }
        
        buffer[len] = '\0';
        
        /* Remove trailing newline if present */
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len-1] == '\r') {
            buffer[len-1] = '\0';
            len--;
        }
        
        /* Parse the packet */
        telemetry_type_t type = udp_telemetry_parse(telem, buffer);
        if (type != TELEM_NONE) {
            telem->packets_received++;
            packets++;
        } else {
            telem->parse_errors++;
            LOG_DEBUG("Failed to parse telemetry: %s", buffer);
        }
    }
    
    return packets;
}

/*
 * Parse a telemetry packet
 * Format: PREFIX,field1,field2,...
 */
telemetry_type_t udp_telemetry_parse(udp_telemetry_t* telem, const char* packet)
{
    if (!telem || !packet || !*packet) return TELEM_NONE;
    
    /* Skip comment lines */
    if (packet[0] == '#') return TELEM_NONE;
    
    /* Make a copy for tokenizing */
    char buf[TELEMETRY_MAX_PACKET];
    strncpy(buf, packet, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    
    /* Get prefix */
    char* token = strtok(buf, ",");
    if (!token) return TELEM_NONE;
    
    uint32_t now = get_time_ms();
    
    /* Parse based on prefix */
    if (strcmp(token, "CHAN") == 0) {
        /* CHAN,time,timestamp_ms,carrier_db,snr_db,sub500_db,sub600_db,tone1000_db,noise_db,quality */
        /* Skip time field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        /* Skip timestamp_ms field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        float values[6];
        for (int i = 0; i < 6; i++) {
            token = strtok(NULL, ",");
            if (!token) return TELEM_NONE;
            values[i] = (float)atof(token);
        }
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        telem->channel.carrier_db = values[0];
        telem->channel.snr_db = values[1];
        telem->channel.sub500_db = values[2];
        telem->channel.sub600_db = values[3];
        telem->channel.tone1000_db = values[4];
        telem->channel.noise_db = values[5];
        telem->channel.quality = parse_quality(token);
        telem->channel.valid = true;
        telem->channel.last_update = now;
        
        return TELEM_CHANNEL;
    }
    else if (strcmp(token, "CARR") == 0) {
        /* CARR,time,timestamp_ms,measured_hz,offset_hz,offset_ppm,snr_db */
        /* Skip time field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        /* Skip timestamp_ms field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        float values[4];
        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ",");
            if (!token) return TELEM_NONE;
            values[i] = (float)atof(token);
        }
        
        telem->carrier.measured_hz = values[0];
        telem->carrier.offset_hz = values[1];
        telem->carrier.offset_ppm = values[2];
        telem->carrier.snr_db = values[3];
        /* Check for optional valid field */
        token = strtok(NULL, ",");
        telem->carrier.measurement_valid = token ? parse_yes_no(token) : true;
        telem->carrier.valid = true;
        telem->carrier.last_update = now;
        
        return TELEM_CARRIER;
    }
    else if (strcmp(token, "SUBC") == 0) {
        /* SUBC,time,timestamp_ms,minute,expected,sub500_db,sub600_db,delta_db,detected,match */
        /* Skip time field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        /* Skip timestamp_ms field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        telem->subcarrier.minute = atoi(token);
        
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        telem->subcarrier.expected = parse_subcarrier(token);
        
        float values[3];
        for (int i = 0; i < 3; i++) {
            token = strtok(NULL, ",");
            if (!token) return TELEM_NONE;
            values[i] = (float)atof(token);
        }
        telem->subcarrier.sub500_db = values[0];
        telem->subcarrier.sub600_db = values[1];
        telem->subcarrier.delta_db = values[2];
        
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        telem->subcarrier.detected = parse_subcarrier(token);
        
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        telem->subcarrier.match = parse_yes_no(token);
        
        telem->subcarrier.valid = true;
        telem->subcarrier.last_update = now;
        
        return TELEM_SUBCARRIER;
    }
    else if (strcmp(token, "T500") == 0) {
        /* T500,time,timestamp_ms,measured_hz,offset_hz,offset_ppm,snr_db */
        /* Skip time field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        /* Skip timestamp_ms field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        float values[4];
        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ",");
            if (!token) return TELEM_NONE;
            values[i] = (float)atof(token);
        }
        
        telem->tone500.measured_hz = values[0];
        telem->tone500.offset_hz = values[1];
        telem->tone500.offset_ppm = values[2];
        telem->tone500.snr_db = values[3];
        token = strtok(NULL, ",");
        telem->tone500.measurement_valid = token ? parse_yes_no(token) : true;
        telem->tone500.valid = true;
        telem->tone500.last_update = now;
        
        return TELEM_TONE500;
    }
    else if (strcmp(token, "T600") == 0) {
        /* T600,time,timestamp_ms,measured_hz,offset_hz,offset_ppm,snr_db */
        /* Skip time field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        /* Skip timestamp_ms field */
        token = strtok(NULL, ",");
        if (!token) return TELEM_NONE;
        
        float values[4];
        for (int i = 0; i < 4; i++) {
            token = strtok(NULL, ",");
            if (!token) return TELEM_NONE;
            values[i] = (float)atof(token);
        }
        
        telem->tone600.measured_hz = values[0];
        telem->tone600.offset_hz = values[1];
        telem->tone600.offset_ppm = values[2];
        telem->tone600.snr_db = values[3];
        token = strtok(NULL, ",");
        telem->tone600.measurement_valid = token ? parse_yes_no(token) : true;
        telem->tone600.valid = true;
        telem->tone600.last_update = now;
        
        return TELEM_TONE600;
    }
    
    return TELEM_NONE;
}

/*
 * Check if telemetry is stale
 */
bool udp_telemetry_is_stale(const udp_telemetry_t* telem, uint32_t timeout_ms)
{
    if (!telem) return true;
    
    uint32_t now = get_time_ms();
    
    /* Check channel data (most frequently updated) */
    if (telem->channel.valid) {
        if ((now - telem->channel.last_update) < timeout_ms) {
            return false;
        }
    }
    
    /* Check carrier data */
    if (telem->carrier.valid) {
        if ((now - telem->carrier.last_update) < timeout_ms) {
            return false;
        }
    }
    
    return true;
}

/*
 * Get quality as string
 */
const char* udp_telemetry_quality_str(signal_quality_t quality)
{
    switch (quality) {
        case QUALITY_GOOD: return "GOOD";
        case QUALITY_FAIR: return "FAIR";
        case QUALITY_POOR: return "POOR";
        case QUALITY_NONE:
        default: return "NONE";
    }
}

/*
 * Get subcarrier as string
 */
const char* udp_telemetry_subcarrier_str(subcarrier_t sub)
{
    switch (sub) {
        case SUBCAR_500HZ: return "500Hz";
        case SUBCAR_600HZ: return "600Hz";
        case SUBCAR_NONE:
        default: return "NONE";
    }
}
