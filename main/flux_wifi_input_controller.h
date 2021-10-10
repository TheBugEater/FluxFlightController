#pragma once
#include "flux_types.h"

typedef struct
{
        char            ssid[32]; 
        char            password[32];
        unsigned short  broadcast_port;
        unsigned short  communication_port;
} flux_wifi_input_controller_config_t;

flux_input_controller_t*       flux_create_wifi_input_controller(flux_wifi_input_controller_config_t* config);
