/**
 * Phoenix SDR Controller - TCP Client Implementation
 * 
 * Winsock-based TCP socket handling for Phoenix SDR server communication.
 */

#include "tcp_client.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/select.h>
    #include <fcntl.h>
    #include <netdb.h>
#endif

/* Static flag for Winsock initialization */
static bool g_winsock_initialized = false;

/*
 * Initialize TCP client subsystem (call once at startup)
 */
bool tcp_client_init(void)
{
#ifdef _WIN32
    if (!g_winsock_initialized) {
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            LOG_ERROR("WSAStartup failed: %d", result);
            return false;
        }
        g_winsock_initialized = true;
        LOG_INFO("Winsock initialized");
    }
#endif
    return true;
}

/*
 * Cleanup TCP client subsystem (call at shutdown)
 */
void tcp_client_cleanup(void)
{
#ifdef _WIN32
    if (g_winsock_initialized) {
        WSACleanup();
        g_winsock_initialized = false;
        LOG_INFO("Winsock cleaned up");
    }
#endif
}

/*
 * Create a new TCP client context
 */
tcp_client_t* tcp_client_create(void)
{
    tcp_client_t* client = (tcp_client_t*)calloc(1, sizeof(tcp_client_t));
    if (!client) {
        LOG_ERROR("Failed to allocate tcp_client_t");
        return NULL;
    }
    
    client->socket = INVALID_SOCK;
    client->state = CONN_DISCONNECTED;
    client->port = DEFAULT_PORT;
    client->host[0] = '\0';
    client->last_error[0] = '\0';
    client->last_activity_ms = 0;
    client->has_pending_data = false;
    
    return client;
}

/*
 * Destroy TCP client context
 */
void tcp_client_destroy(tcp_client_t* client)
{
    if (client) {
        tcp_client_disconnect(client);
        free(client);
    }
}

/*
 * Connect to server
 */
bool tcp_client_connect(tcp_client_t* client, const char* host, int port)
{
    if (!client) {
        return false;
    }
    
    /* Disconnect if already connected */
    if (client->socket != INVALID_SOCK) {
        tcp_client_disconnect(client);
    }
    
    client->state = CONN_CONNECTING;
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->host[sizeof(client->host) - 1] = '\0';
    client->port = port;
    
    LOG_INFO("Connecting to %s:%d", host, port);
    
    /* Resolve host address */
    struct addrinfo hints = {0};
    struct addrinfo* result = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    int res = getaddrinfo(host, port_str, &hints, &result);
    if (res != 0) {
        snprintf(client->last_error, sizeof(client->last_error), 
                 "getaddrinfo failed: %d", res);
        LOG_ERROR("%s", client->last_error);
        client->state = CONN_ERROR;
        return false;
    }
    
    /* Create socket */
    client->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (client->socket == INVALID_SOCK) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "socket() failed: %d", SOCKET_ERROR_CODE);
        LOG_ERROR("%s", client->last_error);
        freeaddrinfo(result);
        client->state = CONN_ERROR;
        return false;
    }
    
    /* Set socket timeout for recv operations */
#ifdef _WIN32
    DWORD timeout = SOCKET_TIMEOUT_MS;
    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(client->socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = SOCKET_TIMEOUT_MS / 1000;
    tv.tv_usec = (SOCKET_TIMEOUT_MS % 1000) * 1000;
    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(client->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    
    /* Connect to server */
    res = connect(client->socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    
    if (res != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "connect() failed: %d", SOCKET_ERROR_CODE);
        LOG_ERROR("%s", client->last_error);
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCK;
        client->state = CONN_ERROR;
        return false;
    }
    
    client->state = CONN_CONNECTED;
    client->last_activity_ms = 0; /* Will be set by caller with SDL_GetTicks() */
    LOG_INFO("Connected to %s:%d", host, port);
    
    return true;
}

/*
 * Disconnect from server
 */
void tcp_client_disconnect(tcp_client_t* client)
{
    if (!client) {
        return;
    }
    
    if (client->socket != INVALID_SOCK) {
        /* Attempt graceful shutdown */
#ifdef _WIN32
        shutdown(client->socket, SD_BOTH);
#else
        shutdown(client->socket, SHUT_RDWR);
#endif
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCK;
        LOG_INFO("Disconnected from %s:%d", client->host, client->port);
    }
    
    client->state = CONN_DISCONNECTED;
    client->has_pending_data = false;
}

/*
 * Check if connected
 */
bool tcp_client_is_connected(tcp_client_t* client)
{
    return client && client->state == CONN_CONNECTED && client->socket != INVALID_SOCK;
}

/*
 * Send a command (adds newline automatically)
 */
bool tcp_client_send(tcp_client_t* client, const char* command)
{
    if (!tcp_client_is_connected(client)) {
        snprintf(client->last_error, sizeof(client->last_error), "Not connected");
        return false;
    }
    
    if (!command) {
        snprintf(client->last_error, sizeof(client->last_error), "NULL command");
        return false;
    }
    
    /* Build command with newline */
    char buffer[MAX_CMD_LENGTH];
    int len = snprintf(buffer, sizeof(buffer), "%s\n", command);
    
    if (len <= 0 || len >= (int)sizeof(buffer)) {
        snprintf(client->last_error, sizeof(client->last_error), "Command too long");
        return false;
    }
    
    /* Send data */
    int sent = send(client->socket, buffer, len, 0);
    if (sent != len) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "send() failed: %d", SOCKET_ERROR_CODE);
        LOG_ERROR("%s", client->last_error);
        client->state = CONN_ERROR;
        return false;
    }
    
    LOG_DEBUG("Sent: %s", command);
    return true;
}

/*
 * Receive response (blocking with timeout)
 */
bool tcp_client_receive(tcp_client_t* client, char* buffer, size_t buffer_size, int timeout_ms)
{
    if (!tcp_client_is_connected(client)) {
        snprintf(client->last_error, sizeof(client->last_error), "Not connected");
        return false;
    }
    
    if (!buffer || buffer_size == 0) {
        snprintf(client->last_error, sizeof(client->last_error), "Invalid buffer");
        return false;
    }
    
    /* Use select for timeout */
    fd_set read_fds;
    struct timeval tv;
    
    FD_ZERO(&read_fds);
    FD_SET(client->socket, &read_fds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int select_result = select((int)client->socket + 1, &read_fds, NULL, NULL, &tv);
    
    if (select_result < 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                 "select() failed: %d", SOCKET_ERROR_CODE);
        LOG_ERROR("%s", client->last_error);
        client->state = CONN_ERROR;
        return false;
    }
    
    if (select_result == 0) {
        snprintf(client->last_error, sizeof(client->last_error), "Timeout");
        return false;
    }
    
    /* Read data until newline or buffer full */
    size_t total_received = 0;
    while (total_received < buffer_size - 1) {
        int received = recv(client->socket, buffer + total_received, 1, 0);
        
        if (received < 0) {
            snprintf(client->last_error, sizeof(client->last_error),
                     "recv() failed: %d", SOCKET_ERROR_CODE);
            LOG_ERROR("%s", client->last_error);
            client->state = CONN_ERROR;
            return false;
        }
        
        if (received == 0) {
            snprintf(client->last_error, sizeof(client->last_error), "Connection closed");
            client->state = CONN_DISCONNECTED;
            client->socket = INVALID_SOCK;
            return false;
        }
        
        total_received++;
        
        /* Check for newline (end of response) */
        if (buffer[total_received - 1] == '\n') {
            break;
        }
    }
    
    /* Null-terminate and trim trailing newline/carriage return */
    buffer[total_received] = '\0';
    while (total_received > 0 && (buffer[total_received - 1] == '\n' || buffer[total_received - 1] == '\r')) {
        buffer[--total_received] = '\0';
    }
    
    LOG_DEBUG("Received: %s", buffer);
    return true;
}

/*
 * Send command and receive response
 */
bool tcp_client_send_receive(tcp_client_t* client, const char* command,
                             char* response, size_t response_size, int timeout_ms)
{
    if (!tcp_client_send(client, command)) {
        return false;
    }
    
    return tcp_client_receive(client, response, response_size, timeout_ms);
}

/*
 * Check for async notifications (non-blocking)
 */
bool tcp_client_check_async(tcp_client_t* client, char* buffer, size_t buffer_size)
{
    if (!tcp_client_is_connected(client)) {
        return false;
    }
    
    /* Non-blocking check using select with zero timeout */
    fd_set read_fds;
    struct timeval tv = {0, 0};
    
    FD_ZERO(&read_fds);
    FD_SET(client->socket, &read_fds);
    
    int select_result = select((int)client->socket + 1, &read_fds, NULL, NULL, &tv);
    
    if (select_result <= 0) {
        client->has_pending_data = false;
        return false;
    }
    
    client->has_pending_data = true;
    
    if (buffer && buffer_size > 0) {
        /* Read the pending data */
        return tcp_client_receive(client, buffer, buffer_size, 100);
    }
    
    return true;
}

/*
 * Get last error message
 */
const char* tcp_client_get_error(tcp_client_t* client)
{
    if (!client) {
        return "NULL client";
    }
    return client->last_error;
}

/*
 * Get connection state
 */
connection_state_t tcp_client_get_state(tcp_client_t* client)
{
    if (!client) {
        return CONN_DISCONNECTED;
    }
    return client->state;
}
