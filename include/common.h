/**
 * Phoenix SDR Controller - Common Definitions
 * 
 * Shared types, constants, and macros used throughout the application.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define INVALID_SOCK INVALID_SOCKET
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    typedef int socket_t;
    #define INVALID_SOCK (-1)
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET(s) close(s)
#endif

/* Application version */
#define APP_VERSION "0.3.0"
#define APP_NAME "Phoenix SDR Controller"

/* Protocol constants */
#define DEFAULT_PORT 4535
#define MAX_CMD_LENGTH 256
#define MAX_RESPONSE_LENGTH 1024
#define KEEPALIVE_INTERVAL_MS 60000
#define SOCKET_TIMEOUT_MS 5000

/* SDR limits (RSP2 Pro) */
#define FREQ_MIN 1000LL
#define FREQ_MAX 2000000000LL
#define GAIN_MIN 20
#define GAIN_MAX 59
#define LNA_MIN 0
#define LNA_MAX 9
#define SRATE_MIN 2000000
#define SRATE_MAX 10000000

/* DC offset for tuning (Hz) */
#define DC_OFFSET_HZ 450

/* WWV frequencies (Hz) */
#define WWV_2_5_MHZ  2500000LL
#define WWV_5_MHZ    5000000LL
#define WWV_10_MHZ  10000000LL
#define WWV_15_MHZ  15000000LL
#define WWV_20_MHZ  20000000LL
#define WWV_25_MHZ  25000000LL
#define WWV_30_MHZ  30000000LL

/* AGC modes */
typedef enum {
    AGC_OFF = 0,
    AGC_5HZ,
    AGC_50HZ,
    AGC_100HZ
} agc_mode_t;

/* Antenna ports */
typedef enum {
    ANTENNA_A = 0,
    ANTENNA_B,
    ANTENNA_HIZ
} antenna_port_t;

/* Connection state */
typedef enum {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_ERROR
} connection_state_t;

/* Error codes matching protocol */
typedef enum {
    ERR_NONE = 0,
    ERR_SYNTAX,
    ERR_UNKNOWN,
    ERR_PARAM,
    ERR_RANGE,
    ERR_STATE,
    ERR_BUSY,
    ERR_HARDWARE,
    ERR_TIMEOUT
} error_code_t;

/* UI Colors (SDRuno-inspired dark theme) */
#define COLOR_BG_DARK       0x1A1A2EFF
#define COLOR_BG_PANEL      0x16213EFF
#define COLOR_BG_WIDGET     0x0F3460FF
#define COLOR_ACCENT        0x00D9FFFF
#define COLOR_ACCENT_DIM    0x007799FF
#define COLOR_TEXT          0xE8E8E8FF
#define COLOR_TEXT_DIM      0x888888FF
#define COLOR_GREEN         0x00FF88FF
#define COLOR_RED           0xFF4444FF
#define COLOR_ORANGE        0xFFA500FF
#define COLOR_YELLOW        0xFFFF00FF
#define COLOR_FREQ_DISPLAY  0x00FFAAFF
#define COLOR_BUTTON        0x2D4A7CFF
#define COLOR_BUTTON_HOVER  0x3D5A8CFF
#define COLOR_BUTTON_ACTIVE 0x4D6A9CFF
#define COLOR_SLIDER_BG     0x333355FF
#define COLOR_SLIDER_FG     0x00AAFFFF

/* Utility macros */
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Logging macros */
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#ifdef _DEBUG
    #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
#endif

#endif /* COMMON_H */
