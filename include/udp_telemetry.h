/**
 * Phoenix SDR Controller - UDP Telemetry Receiver
 * 
 * Receives WWV signal statistics broadcast on UDP port 3005.
 * 
 * Telemetry channels:
 *   CHAN - Channel quality (carrier, SNR, tones, noise, quality)
 *   CARR - Carrier frequency tracking (offset Hz/ppm)
 *   SUBC - Subcarrier detection (500/600 Hz, match status)
 *   T500 - 500 Hz tone tracking
 *   T600 - 600 Hz tone tracking
 */

#ifndef UDP_TELEMETRY_H
#define UDP_TELEMETRY_H

#include "common.h"

/* Default telemetry port */
#define TELEMETRY_UDP_PORT 3005

/* Maximum packet size */
#define TELEMETRY_MAX_PACKET 512

/* Telemetry channel types */
typedef enum {
    TELEM_NONE = 0,
    TELEM_CHANNEL,      /* CHAN - overall channel quality */
    TELEM_CARRIER,      /* CARR - carrier frequency tracking */
    TELEM_SUBCARRIER,   /* SUBC - subcarrier detection */
    TELEM_TONE500,      /* T500 - 500 Hz tone tracking */
    TELEM_TONE600,      /* T600 - 600 Hz tone tracking */
    TELEM_MARKER,       /* MARK - minute marker events */
    TELEM_SYNC          /* SYNC - synchronization state */
} telemetry_type_t;

/* Channel quality levels */
typedef enum {
    QUALITY_NONE = 0,   /* SNR < 3 dB */
    QUALITY_POOR,       /* SNR 3-8 dB */
    QUALITY_FAIR,       /* SNR 8-15 dB */
    QUALITY_GOOD        /* SNR > 15 dB */
} signal_quality_t;

/* Expected subcarrier per WWV schedule */
typedef enum {
    SUBCAR_NONE = 0,    /* Silent minute */
    SUBCAR_500HZ,       /* 500 Hz subcarrier */
    SUBCAR_600HZ        /* 600 Hz subcarrier */
} subcarrier_t;

/* CHAN - Channel quality data */
typedef struct {
    float carrier_db;       /* DC/carrier power in dB */
    float snr_db;           /* Signal-to-noise ratio */
    float sub500_db;        /* 500 Hz subcarrier power */
    float sub600_db;        /* 600 Hz subcarrier power */
    float tone1000_db;      /* 1000 Hz tick tone power */
    float noise_db;         /* Noise floor */
    signal_quality_t quality;
    bool valid;             /* Data received at least once */
    uint32_t last_update;   /* Timestamp of last update (ms) */
} telem_channel_t;

/* CARR - Carrier tracking data */
typedef struct {
    float measured_hz;      /* Measured frequency offset */
    float offset_hz;        /* Same as measured_hz */
    float offset_ppm;       /* Offset in parts per million */
    float snr_db;           /* SNR of carrier */
    bool measurement_valid; /* YES if reliable measurement */
    bool valid;             /* Data received */
    uint32_t last_update;
} telem_carrier_t;

/* SUBC - Subcarrier detection data */
typedef struct {
    int minute;             /* Current minute (0-59) */
    subcarrier_t expected;  /* Expected per WWV schedule */
    float sub500_db;        /* Measured 500 Hz power */
    float sub600_db;        /* Measured 600 Hz power */
    float delta_db;         /* sub500_db - sub600_db */
    subcarrier_t detected;  /* Which tone detected */
    bool match;             /* Does detected match expected? */
    bool valid;
    uint32_t last_update;
} telem_subcarrier_t;

/* T500/T600 - Tone tracking data */
typedef struct {
    float measured_hz;      /* Actual measured frequency */
    float offset_hz;        /* Deviation from nominal */
    float offset_ppm;       /* Offset in ppm */
    float snr_db;           /* SNR of tone */
    bool measurement_valid; /* YES if tone detected */
    bool valid;
    uint32_t last_update;
} telem_tone_t;

/* Sync state */
typedef enum {
    SYNC_SEARCHING = 0,
    SYNC_ACQUIRING,
    SYNC_LOCKED
} sync_state_t;

/* MARK - Minute marker event data */
typedef struct {
    char marker_num[8];     /* Marker label (M1, M2, etc.) */
    int wwv_sec;            /* WWV second position (0-59) */
    char expected[32];      /* Expected WWV event */
    float accum_energy;     /* Accumulated energy during marker */
    float duration_ms;      /* Marker pulse duration (ms) */
    float since_last_sec;   /* Time since last marker (seconds) */
    float baseline;         /* Energy baseline level */
    float threshold;        /* Detection threshold */
    bool valid;
    uint32_t last_update;
} telem_marker_t;

/* SYNC - Synchronization state data */
typedef struct {
    int marker_num;         /* Confirmed marker count */
    sync_state_t state;     /* SEARCHING, ACQUIRING, LOCKED */
    float interval_sec;     /* Interval between markers (seconds) */
    float delta_ms;         /* Timing error (ms) from expected */
    float tick_dur_ms;      /* Last tick pulse duration (ms) */
    float marker_dur_ms;    /* Last marker pulse duration (ms) */
    bool valid;
    uint32_t last_update;
} telem_sync_t;

/* Complete telemetry state */
typedef struct {
    telem_channel_t channel;
    telem_carrier_t carrier;
    telem_subcarrier_t subcarrier;
    telem_tone_t tone500;
    telem_tone_t tone600;
    telem_marker_t marker;
    telem_sync_t sync;
    
    /* Connection state */
    socket_t socket;
    bool bound;
    int port;
    
    /* Statistics */
    uint32_t packets_received;
    uint32_t parse_errors;
} udp_telemetry_t;

/* Initialize telemetry receiver */
udp_telemetry_t* udp_telemetry_create(int port);

/* Destroy telemetry receiver */
void udp_telemetry_destroy(udp_telemetry_t* telem);

/* Start listening (bind socket) */
bool udp_telemetry_start(udp_telemetry_t* telem);

/* Stop listening */
void udp_telemetry_stop(udp_telemetry_t* telem);

/* Check for and process incoming packets (non-blocking) 
 * Returns number of packets processed */
int udp_telemetry_poll(udp_telemetry_t* telem);

/* Parse a telemetry packet line
 * Returns the type of data parsed, or TELEM_NONE on error */
telemetry_type_t udp_telemetry_parse(udp_telemetry_t* telem, const char* packet);

/* Check if telemetry is stale (no updates in timeout_ms) */
bool udp_telemetry_is_stale(const udp_telemetry_t* telem, uint32_t timeout_ms);

/* Get quality as string */
const char* udp_telemetry_quality_str(signal_quality_t quality);

/* Get subcarrier as string */
const char* udp_telemetry_subcarrier_str(subcarrier_t sub);

/* Get sync state as string */
const char* udp_telemetry_sync_state_str(sync_state_t state);

/* WWV/WWVH tone schedule - which tone each station broadcasts at a given minute */
typedef enum {
    TONE_NONE = 0,
    TONE_500HZ,
    TONE_600HZ,
    TONE_SPECIAL     /* Voice, alerts, HamSCI test, etc. */
} wwv_tone_t;

/* Special broadcast types */
typedef enum {
    SPECIAL_NONE = 0,
    SPECIAL_VOICE,           /* Voice announcement */
    SPECIAL_GEO_ALERT,       /* Geophysical alert (NOAA) */
    SPECIAL_HAMSCI           /* HamSCI test signal */
} wwv_special_t;

/* Get the tone WWV broadcasts at a given minute (0-59) */
wwv_tone_t wwv_get_tone(int minute);

/* Get the tone WWVH broadcasts at a given minute (0-59) */
wwv_tone_t wwvh_get_tone(int minute);

/* Get special broadcast type for WWV at a given minute */
wwv_special_t wwv_get_special(int minute);

/* Get special broadcast type for WWVH at a given minute */
wwv_special_t wwvh_get_special(int minute);

/* Get tone as short string ("500", "600", "---") */
const char* wwv_tone_str(wwv_tone_t tone);

/* Get special broadcast as string */
const char* wwv_special_str(wwv_special_t special);

#endif /* UDP_TELEMETRY_H */
