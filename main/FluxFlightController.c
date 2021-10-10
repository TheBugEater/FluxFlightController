#include <stdio.h>
#include "flux_engine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "flux_wifi_input_controller.h"

void app_main(void)
{
        flux_engine_config_t config;


        flux_wifi_input_controller_config_t wifi_controller_config =
        {
                .ssid = "TheBug's WiFi",
                .password = "Fello@0123456",
                .broadcast_port = 32353,
                .communication_port = 43295,
        };

        flux_input_controller_t* wifi_controller = flux_create_wifi_input_controller(&wifi_controller_config);
        flux_engine_add_input_controller(wifi_controller);

        flux_engine_start(&config);
}
