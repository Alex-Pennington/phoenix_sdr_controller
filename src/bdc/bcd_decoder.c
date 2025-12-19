/**
 * @file bcd_decoder.c
 * @brief WWV BCD Frame Assembler and Decoder Implementation (Controller Side)
 *
 * Receives pre-detected symbols from modem with timestamps.
 * Uses minute sync timing from sync_detector to calculate frame position.
 * Assembles 60-symbol frames and decodes BCD time code.
 */

#include "bcd_decoder.h"
#include "../include/common.h"  /* For LOG_INFO etc */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*============================================================================
 * Internal Constants
 *============================================================================*/

/* Position marker seconds within frame */
static const int P_MARKER_POSITIONS[] = {0, 9, 19, 29, 39, 49, 59};
#define NUM_P_MARKERS 7

/*============================================================================
 * Internal State Structure
 *============================================================================*/

struct bcd_decoder {
    /* Sync state */
    bcd_sync_state_t sync_state;
    int frame_position;             /* Current position (0-59) or -1 */
    int last_frame_position;        /* Previous position for wrap detection */
    
    /* Frame buffer */
    bcd_symbol_t frame[BCD_FRAME_LENGTH];
    int symbols_in_frame;
    int p_markers_in_frame;
    
    /* Last symbol info */
    bcd_symbol_t last_symbol;
    float last_symbol_width_ms;
    float last_symbol_confidence;
    
    /* Sync tracking */
    sync_state_t last_sync_state;
    
    /* Decoded time */
    bcd_time_t last_time;
    
    /* Statistics */
    uint32_t frames_decoded;
    uint32_t frames_failed;
    uint32_t total_symbols;
};

/*============================================================================
 * Internal Helpers
 *============================================================================*/

/**
 * Convert char symbol to enum
 */
static bcd_symbol_t char_to_symbol(char c) {
    switch (c) {
        case '0': return BCD_SYMBOL_ZERO;
        case '1': return BCD_SYMBOL_ONE;
        case 'P': return BCD_SYMBOL_MARKER;
        default:  return BCD_SYMBOL_NONE;
    }
}

/**
 * Check if a position is a P marker location
 */
static bool is_p_marker_position(int pos) {
    for (int i = 0; i < NUM_P_MARKERS; i++) {
        if (P_MARKER_POSITIONS[i] == pos) return true;
    }
    return false;
}

/**
 * Decode BCD value from symbol array
 */
static int decode_bcd_field(bcd_symbol_t *symbols, const int *weights, int count) {
    int value = 0;
    for (int i = 0; i < count; i++) {
        if (symbols[i] == BCD_SYMBOL_ONE) {
            value += weights[i];
        } else if (symbols[i] != BCD_SYMBOL_ZERO) {
            return -1;  /* Invalid symbol in data position */
        }
    }
    return value;
}

/**
 * Decode a complete frame into time values
 */
static bool decode_frame(bcd_decoder_t *dec, bcd_time_t *time_out,
                         bcd_frame_quality_t *quality_out) {
    bcd_symbol_t *f = dec->frame;
    
    /* Count position markers */
    int markers_found = 0;
    int markers_correct = 0;
    for (int i = 0; i < NUM_P_MARKERS; i++) {
        int pos = P_MARKER_POSITIONS[i];
        if (f[pos] == BCD_SYMBOL_MARKER) {
            markers_found++;
            markers_correct++;
        }
    }
    
    /* Also count P markers at wrong positions */
    for (int i = 0; i < BCD_FRAME_LENGTH; i++) {
        if (f[i] == BCD_SYMBOL_MARKER && !is_p_marker_position(i)) {
            markers_found++;
        }
    }
    
    if (quality_out) {
        quality_out->symbols_received = dec->symbols_in_frame;
        quality_out->markers_found = markers_found;
        quality_out->markers_correct = markers_correct;
        quality_out->frame_coverage = (float)dec->symbols_in_frame / BCD_FRAME_LENGTH * 100.0f;
    }
    
    /* Need at least 5 of 7 P markers at correct positions */
    if (markers_correct < 5) {
        LOG_WARN("[BCD] Frame decode failed: only %d/%d P markers correct (found %d total)",
               markers_correct, NUM_P_MARKERS, markers_found);
        
        /* Dump frame for diagnosis */
        char frame_str[180];
        char* p = frame_str;
        for (int i = 0; i < BCD_FRAME_LENGTH; i++) {
            if (i > 0 && i % 10 == 0) *p++ = ' ';
            switch (f[i]) {
                case BCD_SYMBOL_ZERO:   *p++ = '0'; break;
                case BCD_SYMBOL_ONE:    *p++ = '1'; break;
                case BCD_SYMBOL_MARKER: *p++ = 'P'; break;
                default:                *p++ = '.'; break;
            }
        }
        *p = '\0';
        LOG_INFO("[BCD] Frame: %s", frame_str);
        LOG_INFO("[BCD] Expect P at: 0,9,19,29,39,49,59");
        
        /* Show where P markers actually are */
        char p_pos_str[64] = "P found at:";
        for (int i = 0; i < BCD_FRAME_LENGTH; i++) {
            if (f[i] == BCD_SYMBOL_MARKER) {
                char tmp[8];
                snprintf(tmp, sizeof(tmp), " %d", i);
                strcat(p_pos_str, tmp);
            }
        }
        LOG_INFO("[BCD] %s", p_pos_str);
        
        return false;
    }
    
    /* Decode minutes: seconds 1-8 */
    static const int min_weights[] = {40, 20, 10, 0, 8, 4, 2, 1};
    int minutes = decode_bcd_field(&f[1], min_weights, 8);
    if (minutes < 0 || minutes > 59) {
        LOG_WARN("[BCD] Invalid minutes: %d (fields 1-8: %d%d%d%d %d%d%d%d)",
                minutes, f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
        return false;
    }
    
    /* Decode hours: seconds 12-18 */
    static const int hour_weights[] = {20, 10, 0, 8, 4, 2, 1};
    int hours = decode_bcd_field(&f[12], hour_weights, 7);
    if (hours < 0 || hours > 23) {
        LOG_WARN("[BCD] Invalid hours: %d (fields 12-18: %d%d%d %d%d%d%d)",
                hours, f[12], f[13], f[14], f[15], f[16], f[17], f[18]);
        return false;
    }
    
    /* Decode day of year: seconds 22-28 (hundreds/tens), 30-33 (units) */
    static const int doy_ht_weights[] = {200, 100, 0, 80, 40, 20, 10};
    int doy_ht = decode_bcd_field(&f[22], doy_ht_weights, 7);
    
    static const int doy_u_weights[] = {8, 4, 2, 1};
    int doy_u = decode_bcd_field(&f[30], doy_u_weights, 4);
    
    int day_of_year = (doy_ht >= 0 && doy_u >= 0) ? doy_ht + doy_u : -1;
    if (day_of_year < 1 || day_of_year > 366) {
        LOG_WARN("[BCD] Invalid DOY: %d (ht=%d, u=%d)", day_of_year, doy_ht, doy_u);
        return false;
    }
    
    /* Decode DUT1 sign: seconds 35-37 */
    int dut1_sign = 0;
    if (f[35] == BCD_SYMBOL_ONE || f[36] == BCD_SYMBOL_ONE) {
        dut1_sign = 1;
    }
    if (f[37] == BCD_SYMBOL_ONE) {
        dut1_sign = -1;
    }
    
    /* Decode DUT1 magnitude: seconds 40-43 */
    float dut1_value = 0.0f;
    if (f[40] == BCD_SYMBOL_ONE) dut1_value += 0.8f;
    if (f[41] == BCD_SYMBOL_ONE) dut1_value += 0.4f;
    if (f[42] == BCD_SYMBOL_ONE) dut1_value += 0.2f;
    if (f[43] == BCD_SYMBOL_ONE) dut1_value += 0.1f;
    
    /* Decode year: seconds 51-58 */
    static const int year_weights[] = {80, 40, 20, 10, 8, 4, 2, 1};
    int year = decode_bcd_field(&f[51], year_weights, 8);
    if (year < 0 || year > 99) {
        year = -1;  /* Invalid but don't fail decode */
    }
    
    /* Fill output */
    time_out->valid = true;
    time_out->hours = hours;
    time_out->minutes = minutes;
    time_out->day_of_year = day_of_year;
    time_out->year = year;
    time_out->dut1_sign = dut1_sign;
    time_out->dut1_value = dut1_value;
    time_out->leap_second_pending = false;
    time_out->dst_pending = false;
    time_out->decode_timestamp_ms = 0;  /* Timestamp not used with frame_second mode */
    
    return true;
}

/**
 * Clear frame buffer for new frame
 */
static void clear_frame(bcd_decoder_t *dec) {
    for (int i = 0; i < BCD_FRAME_LENGTH; i++) {
        dec->frame[i] = BCD_SYMBOL_NONE;
    }
    dec->symbols_in_frame = 0;
    dec->p_markers_in_frame = 0;
}

/*============================================================================
 * Public API Implementation
 *============================================================================*/

bcd_decoder_t *bcd_decoder_create(void) {
    bcd_decoder_t *dec = calloc(1, sizeof(bcd_decoder_t));
    if (!dec) return NULL;
    
    dec->sync_state = BCD_SYNC_WAITING;
    dec->frame_position = -1;
    dec->last_frame_position = -1;
    dec->last_symbol = BCD_SYMBOL_NONE;
    dec->last_sync_state = SYNC_ACQUIRING;
    
    clear_frame(dec);
    
    LOG_INFO("[BCD] Frame assembler created, frame position from modem");
    return dec;
}

void bcd_decoder_destroy(bcd_decoder_t *dec) {
    if (dec) {
        if (dec->frames_decoded > 0 || dec->frames_failed > 0) {
            LOG_INFO("[BCD] Final stats: %u decoded, %u failed, %u symbols",
                   dec->frames_decoded, dec->frames_failed, dec->total_symbols);
        }
        free(dec);
    }
}

void bcd_decoder_process_symbol(bcd_decoder_t *dec,
                                char symbol_char,
                                int frame_second,
                                float width_ms,
                                float confidence,
                                sync_state_t sync_state) {
    if (!dec) return;
    
    bcd_symbol_t symbol = char_to_symbol(symbol_char);
    if (symbol == BCD_SYMBOL_NONE) return;
    
    dec->total_symbols++;
    dec->last_symbol = symbol;
    dec->last_symbol_width_ms = width_ms;
    dec->last_symbol_confidence = confidence;
    
    /* Track sync state changes */
    if (dec->last_sync_state != sync_state) {
        const char *state_names[] = {"ACQUIRING", "TENTATIVE", "LOCKED", "RECOVERING"};
        LOG_INFO("[BCD] Sync state change: %s -> %s",
                 state_names[dec->last_sync_state], state_names[sync_state]);
        dec->last_sync_state = sync_state;
        
        /* Clear frame on state change to avoid stale data */
        clear_frame(dec);
    }
    
    /* Only process symbols when not in ACQUIRING state */
    if (sync_state == SYNC_ACQUIRING) {
        if (dec->sync_state != BCD_SYNC_WAITING) {
            LOG_INFO("[BCD] Lost sync (ACQUIRING), clearing frame");
            dec->sync_state = BCD_SYNC_WAITING;
            clear_frame(dec);
        }
        return;
    }
    
    /* Log low-confidence symbols */
    if (confidence < 0.5f) {
        LOG_WARN("[BCD] Low confidence symbol: %c at second %d (%.2f)",
                symbol_char, frame_second, confidence);
    }
    
    /* Validate frame position */
    if (frame_second < 0 || frame_second >= BCD_FRAME_LENGTH) {
        LOG_WARN("[BCD] Invalid frame position: %d", frame_second);
        return;
    }
    
    /* Detect frame boundary (second 0 or wrap from 59->0) */
    bool new_frame = false;
    if (frame_second == 0 || (dec->last_frame_position == 59 && frame_second == 0)) {
        new_frame = true;
        
        /* Check if we should attempt decode */
        if (dec->sync_state == BCD_SYNC_ACTIVE && dec->symbols_in_frame > 0) {
            LOG_DEBUG("[BCD] Frame complete: %d symbols, %d P-markers",
                      dec->symbols_in_frame, dec->p_markers_in_frame);
            
            /* Attempt decode (only in LOCKED state for now) */
            if (sync_state == SYNC_LOCKED) {
                bcd_time_t decoded_time;
                bcd_frame_quality_t quality;
                if (decode_frame(dec, &decoded_time, &quality)) {
                    dec->last_time = decoded_time;
                    dec->frames_decoded++;
                    LOG_INFO("[BCD] Decoded time: %04d-%02d-%02d %02d:%02d",
                             decoded_time.year, decoded_time.day_of_year / 100, decoded_time.day_of_year % 100,
                             decoded_time.hours, decoded_time.minutes);
                } else {
                    dec->frames_failed++;
                    LOG_WARN("[BCD] Frame decode failed");
                }
            }
        }
        
        /* Clear for next frame */
        clear_frame(dec);
    }
    
    /* Update frame position */
    dec->frame_position = frame_second;
    dec->last_frame_position = frame_second;
    
    /* Activate on first valid symbol */
    if (dec->sync_state == BCD_SYNC_WAITING && sync_state != SYNC_ACQUIRING) {
        dec->sync_state = BCD_SYNC_ACTIVE;
        LOG_INFO("[BCD] Activated at frame position %d", frame_second);
    }
    
    /* Accumulate symbol into frame */
    if (dec->sync_state == BCD_SYNC_ACTIVE) {
        /* Store symbol */
        dec->frame[frame_second] = symbol;
        dec->symbols_in_frame++;
        
        /* Count P markers */
        if (symbol == BCD_SYMBOL_MARKER) {
            dec->p_markers_in_frame++;
        }
    }
}

void bcd_decoder_reset(bcd_decoder_t *dec) {
    if (!dec) return;
    
    dec->sync_state = BCD_SYNC_WAITING;
    dec->frame_position = -1;
    dec->last_frame_position = -1;
    dec->last_symbol = BCD_SYMBOL_NONE;
    clear_frame(dec);
    
    LOG_INFO("[BCD] Reset, waiting for minute sync");
}

bcd_sync_state_t bcd_decoder_get_sync_state(bcd_decoder_t *dec) {
    return dec ? dec->sync_state : BCD_SYNC_WAITING;
}

int bcd_decoder_get_frame_position(bcd_decoder_t *dec) {
    return dec ? dec->frame_position : -1;
}

const bcd_time_t *bcd_decoder_get_last_time(bcd_decoder_t *dec) {
    return dec ? &dec->last_time : NULL;
}

void bcd_decoder_get_stats(bcd_decoder_t *dec,
                           uint32_t *frames_decoded,
                           uint32_t *frames_failed,
                           uint32_t *total_symbols) {
    if (!dec) return;
    if (frames_decoded) *frames_decoded = dec->frames_decoded;
    if (frames_failed) *frames_failed = dec->frames_failed;
    if (total_symbols) *total_symbols = dec->total_symbols;
}

void bcd_decoder_get_ui_status(bcd_decoder_t *dec, bcd_ui_status_t *status) {
    if (!status) return;
    memset(status, 0, sizeof(*status));
    
    if (!dec) {
        status->sync_state = BCD_SYNC_WAITING;
        status->frame_position = -1;
        status->last_symbol = BCD_SYMBOL_NONE;
        return;
    }
    
    status->sync_state = dec->sync_state;
    status->frame_position = dec->frame_position;
    status->last_symbol = dec->last_symbol;
    status->last_symbol_width_ms = dec->last_symbol_width_ms;
    status->last_symbol_timestamp_ms = 0.0f;  /* Legacy field - no longer used */
    status->symbols_in_frame = dec->symbols_in_frame;
    status->p_markers_found = dec->p_markers_in_frame;
    status->frames_decoded = dec->frames_decoded;
    status->frames_failed = dec->frames_failed;
    status->total_symbols = dec->total_symbols;
    
    status->time_valid = dec->last_time.valid;
    if (dec->last_time.valid) {
        status->current_time = dec->last_time;
    }
}
