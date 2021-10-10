#include "flux_types.h"
#include "flux_engine.h"
#include "flux_flight_controller.h"
#include "flux_gyro_sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"

TaskHandle_t                    flux_engine_core_task_handle = NULL;
TaskHandle_t                    flux_engine_secondary_task_handle = NULL;
flux_engine_config_t            flux_engine_config;                    


// Input Controllers
flux_input_controller_t*        input_controllers[FLUX_MAX_INPUT_CONTROLLERS];
unsigned short                  current_input_controllers = 0;

// Sensors
flux_sensor_t*                  sensors[FLUX_MAX_SENSORS];
unsigned short                  current_sensors = 0;

void    flux_engine_add_input_controller(flux_input_controller_t* controller)
{
        assert(current_input_controllers < FLUX_MAX_INPUT_CONTROLLERS || controller);

        input_controllers[current_input_controllers++] = controller;
}

void    flux_engine_add_sensor(flux_sensor_t*  sensor)
{
        assert(current_sensors < FLUX_MAX_SENSORS);

        sensors[current_sensors++] = sensor;
}

void    flux_init_subsystems()
{
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void    flux_engine_secondary_task()
{
        configure_gyro_sensor();
        // Initialize
        for(int i = 0; i < current_input_controllers; i++)
        {
                input_controllers[i]->initialize(input_controllers[i]->_this);
        }

        while(1)
        {
                for(int i = 0; i < current_input_controllers; i++)
                {
                        input_controllers[i]->update(input_controllers[i]->_this);
                }

                vTaskDelay(FLUX_ENGINE_SECONDARY_TASK_DELAY/portTICK_PERIOD_MS);
        }

        for(int i = 0; i < current_input_controllers; i++)
        {
                input_controllers[i]->dealloc(input_controllers[i]->_this);
        }

}

void    flux_engine_core_task()
{
        flux_init_flight_controller();

        while(1)
        {
                flux_update_flight_controller();
                vTaskDelay(FLUX_ENGINE_CORE_TASK_DELAY/portTICK_PERIOD_MS);
        }
}

int     flux_engine_start(flux_engine_config_t* config)
{
        // Initialize ESP Subsystems
        flux_init_subsystems();

        flux_engine_config = *config;

        static int taskParameter = 0;
        xTaskCreatePinnedToCore(flux_engine_core_task, "FluxEngineTask", 
                        FLUX_ENGINE_DEFAULT_STACK_SIZE,
                        (void*)&taskParameter, FLUX_ENGINE_CORE_TASK_PRIORITY , &flux_engine_core_task_handle, FLUX_ENGINE_CORE_TASK_AFFINITY); 
        configASSERT(flux_engine_core_task_handle);

        xTaskCreatePinnedToCore(flux_engine_secondary_task, "FluxSecondaryTask", 
                        FLUX_ENGINE_DEFAULT_STACK_SIZE, 
                        (void*)&taskParameter, FLUX_ENGINE_SECONDARY_TASK_PRIORITY, &flux_engine_secondary_task_handle, FLUX_ENGINE_SECONDARY_TASK_AFINITY); 
        configASSERT(flux_engine_secondary_task_handle);

        return 1;
}

