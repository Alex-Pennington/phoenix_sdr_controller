/**
 * Phoenix SDR Controller - TCP Client
 * 
 * Handles TCP socket connection to the Phoenix SDR server.
 */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "common.h"

/* TCP Client context */
typedef struct {
    socket_t socket;
    connection_state_t state;
    char host[256];
    int port;
    char last_error[256];
    uint32_t last_activity_ms;
    bool has_pending_data;
} tcp_client_t;

/* Initialize TCP client (call once at startup) */
bool tcp_client_init(void);

/* Cleanup TCP client (call at shutdown) */
void tcp_client_cleanup(void);

/* Create a new client context */
tcp_client_t* tcp_client_create(void);

/* Destroy client context */
void tcp_client_destroy(tcp_client_t* client);

/* Connect to server */
bool tcp_client_connect(tcp_client_t* client, const char* host, int port);

/* Disconnect from server */
void tcp_client_disconnect(tcp_client_t* client);

/* Check if connected */
bool tcp_client_is_connected(tcp_client_t* client);

/* Send a command (adds newline automatically) */
bool tcp_client_send(tcp_client_t* client, const char* command);

/* Receive response (blocking with timeout) */
bool tcp_client_receive(tcp_client_t* client, char* buffer, size_t buffer_size, int timeout_ms);

/* Send command and receive response */
bool tcp_client_send_receive(tcp_client_t* client, const char* command, 
                             char* response, size_t response_size, int timeout_ms);

/* Check for async notifications (non-blocking) */
bool tcp_client_check_async(tcp_client_t* client, char* buffer, size_t buffer_size);

/* Get last error message */
const char* tcp_client_get_error(tcp_client_t* client);

/* Get connection state */
connection_state_t tcp_client_get_state(tcp_client_t* client);

#endif /* TCP_CLIENT_H */
