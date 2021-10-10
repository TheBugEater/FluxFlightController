#pragma once
#include "flux_types.h"

void    flux_engine_add_input_controller(flux_input_controller_t* controller);
void    flux_engine_add_sensor(flux_sensor_t*  sensor);
int     flux_engine_start(flux_engine_config_t* config);
