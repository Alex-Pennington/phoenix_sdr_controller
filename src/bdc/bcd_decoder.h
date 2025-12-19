/**
 * @file bcd_decoder.h
 * @brief WWV BCD Frame Assembler and Decoder (Controller Side)
 *
 * Receives pre-detected symbols from modem with timestamps.
 * Uses minute sync timing to calculate frame position.
 * Assembles 60-symbol frames and decodes BCD time.
 *
 * Input: Symbols from BCDS,SYM telemetry + sync timing from SYNC telemetry
 * Output: Decoded UTC time
 */

#ifndef BCD_DECODER_H
#define BCD_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"  /* For sync_state_t */

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Configuration
 *============================================================================*/

/* Frame parameters */
#define BCD_FRAME_LENGTH            60      /* Symbols per frame */

/* Position marker locations within frame */
#define BCD_P0_SECOND               0
#define BCD_P1_SECOND               9
#define BCD_P2_SECOND               19
#define BCD_P3_SECOND               29
#define BCD_P4_SECOND               39
#define BCD_P5_SECOND               49
#define BCD_P6_SECOND               59      /* Also next minute's P0 */

/*============================================================================
 * Types
 *============================================================================*/

/** Symbol types (must match modem) */
typedef enum {
    BCD_SYMBOL_NONE = -1,       /* No symbol yet */
    BCD_SYMBOL_ZERO = 0,        /* Binary 0 (~200ms pulse) */
    BCD_SYMBOL_ONE = 1,         /* Binary 1 (~500ms pulse) */
    BCD_SYMBOL_MARKER = 2       /* Position marker (~800ms pulse) */
} bcd_symbol_t;

/** Decoder sync state */
typedef enum {
    BCD_SYNC_WAITING,           /* Waiting for minute sync lock */
    BCD_SYNC_ACTIVE,            /* Minute sync locked, assembling frame */
    BCD_SYNC_LOCKED             /* Frame validated, decoding */
} bcd_sync_state_t;

/** Decoded time structure */
typedef struct {
    bool valid;                 /* Decode successful */
    int hours;                  /* 0-23 UTC */
    int minutes;                /* 0-59 */
    int day_of_year;            /* 1-366 */
    int year;                   /* 2-digit year (0-99) */
    int dut1_sign;              /* +1, 0, or -1 */
    float dut1_value;           /* 0.0 to 0.9 seconds */
    bool leap_second_pending;   /* Leap second warning */
    bool dst_pending;           /* DST change warning */
    uint64_t decode_timestamp_ms; /* When this was decoded */
} bcd_time_t;

/** Frame quality metrics */
typedef struct {
    int symbols_received;       /* Total symbols in frame */
    int markers_found;          /* Position markers detected */
    int markers_correct;        /* P markers at correct positions */
    float frame_coverage;       /* % of frame positions filled */
} bcd_frame_quality_t;

/** Comprehensive status structure for UI */
typedef struct {
    bcd_sync_state_t sync_state;    /* Current sync state */
    int frame_position;             /* Current position (0-59), or -1 */
    
    /* Last symbol info */
    bcd_symbol_t last_symbol;       /* Most recent symbol */
    float last_symbol_width_ms;     /* Width of last symbol */
    float last_symbol_timestamp_ms; /* When last symbol arrived */
    
    /* Frame stats */
    int symbols_in_frame;           /* Symbols received this frame */
    int p_markers_found;            /* P markers in current frame */
    
    /* Decode stats */
    uint32_t frames_decoded;        /* Successfully decoded */
    uint32_t frames_failed;         /* Failed decode */
    uint32_t total_symbols;         /* All symbols received */
    
    /* Current time (if valid) */
    bool time_valid;
    bcd_time_t current_time;
} bcd_ui_status_t;

/** Opaque decoder state */
typedef struct bcd_decoder bcd_decoder_t;

/*============================================================================
 * Public API
 *============================================================================*/

/**
 * Create a new BCD frame assembler
 */
bcd_decoder_t *bcd_decoder_create(void);

/**
 * Destroy a BCD frame assembler
 */
void bcd_decoder_destroy(bcd_decoder_t *dec);

/**
 * Process a symbol from the modem
 *
 * @param dec               Decoder instance
 * @param symbol            Symbol type ('0', '1', 'P')
 * @param frame_second      Frame position (0-59) from modem
 * @param width_ms          Pulse width in milliseconds
 * @param confidence        Symbol confidence 0.0-1.0
 * @param sync_state        Current sync state from modem
 */
void bcd_decoder_process_symbol(bcd_decoder_t *dec,
                                char symbol,
                                int frame_second,
                                float width_ms,
                                float confidence,
                                sync_state_t sync_state);

/**
 * Reset decoder state
 */
void bcd_decoder_reset(bcd_decoder_t *dec);

/**
 * Get current sync state
 */
bcd_sync_state_t bcd_decoder_get_sync_state(bcd_decoder_t *dec);

/**
 * Get current frame position (0-59, or -1 if not synced)
 */
int bcd_decoder_get_frame_position(bcd_decoder_t *dec);

/**
 * Get most recent decoded time
 */
const bcd_time_t *bcd_decoder_get_last_time(bcd_decoder_t *dec);

/**
 * Get decode statistics
 */
void bcd_decoder_get_stats(bcd_decoder_t *dec,
                           uint32_t *frames_decoded,
                           uint32_t *frames_failed,
                           uint32_t *total_symbols);

/**
 * Get comprehensive UI status
 */
void bcd_decoder_get_ui_status(bcd_decoder_t *dec, bcd_ui_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* BCD_DECODER_H */
