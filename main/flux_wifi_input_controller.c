#include "flux_wifi_input_controller.h"
#include "flux_serialize_utils.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#define FLUX_WIFI_CONNECT_MAGIC_NUMBER  0x32F8B108
#define FLUX_WIFI_SEND_BUFFER_SIZE      256
#define FLUX_WIFI_PING_FREQUENCY        100 // 100ms
#define FLUX_WIFI_WORK_BUFFER_SIZE      1024

char work_buffer[FLUX_WIFI_WORK_BUFFER_SIZE];

typedef enum
{
        FLUX_WIFI_IDLE,
        FLUX_WIFI_CONNECTING_WIFI,
        FLUX_WIFI_LISTEN_BROADCAST,
        FLUX_WIFI_CONTROLLER_CONNECTED,
        FLUX_WIFI_ERROR
} flux_wifi_controller_status_t;

typedef struct
{
        flux_input_controller_t                 base;
        flux_wifi_input_controller_config_t     config;

        struct  sockaddr_in                     remote_addr;
        int                                     socket;
        flux_wifi_controller_status_t           status;
        clock_t                                 last_ping;
} flux_wifi_input_controller_t;

///////////////////////////////////
// UDP Messages Handle Functions //
///////////////////////////////////
int    flux_handle_ping_message(flux_wifi_input_controller_t* controller, char* buffer, unsigned int length)
{
        return flux_serializeint32(FLUX_WIFI_CONNECT_MAGIC_NUMBER, buffer, length);
}

void    flux_handle_controller_input_message(flux_wifi_input_controller_t* controller, char* buffer, unsigned int length)
{
}

///////////////////////////////////
// UDP Messages ///////////////////
///////////////////////////////////
typedef enum
{
        FLUX_UDP_MSG_PING,
        FLUX_UDP_MSG_CONTROLLER_INPUT,
        FLUX_UDP_MSG_MAX
} flux_udp_message_type_t;

typedef int (*flux_udp_msg_sender_fn_t)(flux_wifi_input_controller_t*, char*, unsigned int);
typedef void (*flux_udp_msg_handler_fn_t)(flux_wifi_input_controller_t*, char*, unsigned int);

typedef struct
{
        flux_udp_msg_sender_fn_t                sender;
        flux_udp_msg_handler_fn_t               handler;
} flux_udp_msg_handler_t;

flux_udp_msg_handler_t                          message_handlers[FLUX_UDP_MSG_MAX];

// Register Message Handlers
void    flux_register_message_handlers()
{
        message_handlers[FLUX_UDP_MSG_PING].sender              = flux_handle_ping_message;
        message_handlers[FLUX_UDP_MSG_PING].handler             = NULL;
        message_handlers[FLUX_UDP_MSG_CONTROLLER_INPUT].handler = flux_handle_controller_input_message;
        message_handlers[FLUX_UDP_MSG_CONTROLLER_INPUT].sender  = NULL;
}

int     flux_send_udp_message(flux_wifi_input_controller_t* controller, flux_udp_message_type_t type)
{
        assert(message_handlers[type].sender);

        char    buffer[FLUX_WIFI_SEND_BUFFER_SIZE];
        int     offset = 0;
        offset = flux_serializeint32(type, buffer, FLUX_WIFI_SEND_BUFFER_SIZE);

        int send_size = message_handlers[type].sender(controller, buffer + offset, FLUX_WIFI_SEND_BUFFER_SIZE);
        if(send_size > 0)
        {
                return sendto(controller->socket, buffer, send_size + offset, 0, (struct sockaddr*)&controller->remote_addr, sizeof(controller->remote_addr));
        }

        return -1;
}

////////////////////////////////
// Activate UDP Listner ////////
////////////////////////////////
void    flux_close_socket(flux_wifi_input_controller_t* controller)
{
        shutdown(controller->socket, SHUT_RDWR);
        close(controller->socket);

        controller->socket = 0;
}

int    flux_init_socket_on_port(flux_wifi_input_controller_t* controller, int32_t port)
{
        struct sockaddr_in local_addr = {};
        // Create the Broadcast Socket
        if ((controller->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
                ESP_LOGE("FluxWiFiInputController", "Socket Creation Error" );
                controller->status = FLUX_WIFI_ERROR;
                return -1;
        }

        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(port);
        local_addr.sin_addr.s_addr = INADDR_ANY;

        if(bind(controller->socket, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1)
        {
                
                ESP_LOGE("FluxWiFiInputController", "Socket Bind Error" );
                controller->status = FLUX_WIFI_ERROR;
                return -1;
        }

        if(fcntl(controller->socket, F_SETFL, O_NONBLOCK) == -1)
        {
                ESP_LOGE("FluxWiFiInputController", "Error Setting Socket to Non-Block" );
                controller->status = FLUX_WIFI_ERROR;
                return -1;
        }

        return 0;
}

void    flux_listen_udp_broadcast(flux_wifi_input_controller_t* controller)
{
        int recv_size = 0;
        socklen_t addr_len = sizeof(controller->remote_addr);
        recv_size = recvfrom(controller->socket, work_buffer, FLUX_WIFI_WORK_BUFFER_SIZE, 0, (struct sockaddr*)&controller->remote_addr, &addr_len);
        if(recv_size >= (int)sizeof(int))

        {
                int magic_number = 0;
                flux_deserializeint32(&magic_number, work_buffer, recv_size);
                ESP_LOGI("FluxWiFiInputController", "Magic Number : %d == %d", magic_number, FLUX_WIFI_CONNECT_MAGIC_NUMBER);
                if(magic_number == FLUX_WIFI_CONNECT_MAGIC_NUMBER)
                {
                        ESP_LOGI("FluxWiFiInputController", "Received a Connection Request from %s", inet_ntoa(controller->remote_addr.sin_addr.s_addr));

                        flux_close_socket(controller);
                        if(flux_init_socket_on_port(controller, controller->config.communication_port) == 0)
                        {
                                // Just change the communication port of the address
                                controller->status = FLUX_WIFI_CONTROLLER_CONNECTED;
                                controller->remote_addr.sin_port = htons(controller->config.communication_port);
                        }
                                
                }
        }
        else if(recv_size == -1)
        {
                if(errno != EWOULDBLOCK)
                {
                        ESP_LOGE("FluxWiFiInputController", "Error on Socket %d", errno);
                        controller->status = FLUX_WIFI_ERROR;
                }
        }
}

void    flux_check_incoming_messages(flux_wifi_input_controller_t* controller)
{
        int recv_size = 0;
        socklen_t addr_len = sizeof(controller->remote_addr);
        recv_size = recvfrom(controller->socket, work_buffer, FLUX_WIFI_WORK_BUFFER_SIZE, 0, (struct sockaddr*)&controller->remote_addr, &addr_len);
        if(recv_size > (int)sizeof(int32_t))
        {
        }
        else if(recv_size == -1)
        {
                if(errno != EWOULDBLOCK)
                {
                        ESP_LOGE("FluxWiFiInputController", "Error on Socket %d", errno);
                        controller->status = FLUX_WIFI_ERROR;
                }
        }

}

void    flux_udp_connected(flux_wifi_input_controller_t* controller)
{
        flux_check_incoming_messages(controller);

        clock_t current_time = clock() / (CLOCKS_PER_SEC / 1000);
        if((current_time - controller->last_ping) > FLUX_WIFI_PING_FREQUENCY)
        {
                ESP_LOGI("FluxWiFiInputController", "Last Ping: %ld, Current : %ld", controller->last_ping, current_time );
                controller->last_ping = current_time;
                flux_send_udp_message(controller, FLUX_UDP_MSG_PING);
        }
}

////////////////////////////////
// Initialize WiFi /////////////
////////////////////////////////
void    flux_wifi_controller_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    flux_wifi_input_controller_t* controller = (flux_wifi_input_controller_t*)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        ESP_LOGI("FluxWiFiInputController", "Connecting to WiFi..." );
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        ESP_LOGI("FluxWiFiInputController", "Connecting to WiFi..." );
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("FluxWiFiInputController", "Connected: Local IP :" IPSTR, IP2STR(&event->ip_info.ip));

        if(flux_init_socket_on_port(controller, controller->config.broadcast_port) == 0)
        {
                ESP_LOGI("FluxWiFiInputController", "Listening to Broadcasts on Port %d", controller->config.broadcast_port);
                controller->status = FLUX_WIFI_LISTEN_BROADCAST;
        }
    } 
}

void    wifi_input_controller_initialise(void* _this)
{
        ESP_LOGI("FluxWiFiInputController", "Initializing WiFi..." );

        flux_wifi_input_controller_t* controller = (flux_wifi_input_controller_t*)_this;

        esp_netif_create_default_wifi_sta();

        wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&init_config));

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &flux_wifi_controller_event_handler,
                                                        _this,
                                                        &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &flux_wifi_controller_event_handler,
                                                        _this,
                                                        &instance_got_ip));

        wifi_config_t wifi_config = {};
        memcpy(wifi_config.sta.ssid, controller->config.ssid, sizeof(controller->config.ssid));
        memcpy(wifi_config.sta.password, controller->config.password, sizeof(controller->config.password));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
}

//////////////////////////////////////////
// Controller Functions WiFi /////////////
//////////////////////////////////////////

void    wifi_input_controller_update(void* _this)
{
        flux_wifi_input_controller_t* controller = (flux_wifi_input_controller_t*)_this;

        if(controller->status == FLUX_WIFI_LISTEN_BROADCAST)
        {
                flux_listen_udp_broadcast(controller);
        }
        else if(controller->status == FLUX_WIFI_CONTROLLER_CONNECTED)
        {
                flux_udp_connected(controller);
        }
        else if(controller->status == FLUX_WIFI_ERROR)
        {
                ESP_LOGI("FluxWiFiInputController", "Restarting Socket");
                flux_close_socket(controller);

                if(flux_init_socket_on_port(controller, controller->config.broadcast_port) == 0)
                {
                        controller->status = FLUX_WIFI_LISTEN_BROADCAST;
                }
        }
}

void    wifi_input_controller_dealloc(void* _this)
{
        assert(_this);
        free(_this);
}


flux_input_controller_t*       flux_create_wifi_input_controller(flux_wifi_input_controller_config_t* config)
{
        flux_register_message_handlers();

        flux_wifi_input_controller_t* controller = malloc(sizeof(flux_wifi_input_controller_t));
        assert(controller);

        memset(controller, 0, sizeof(flux_wifi_input_controller_config_t));
        memcpy(&controller->config, config, sizeof(flux_wifi_input_controller_config_t));

        controller->base.initialize = wifi_input_controller_initialise;
        controller->base.update = wifi_input_controller_update;
        controller->base.dealloc = wifi_input_controller_dealloc;
        controller->base._this = controller;

        return &controller->base;
}
