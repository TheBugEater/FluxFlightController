#pragma once

#define FLUX_ENGINE_CORE_TASK_AFFINITY          0
#define FLUX_ENGINE_CORE_TASK_DELAY             10
#define FLUX_ENGINE_CORE_TASK_PRIORITY          10

#define FLUX_ENGINE_DEFAULT_STACK_SIZE          4096

#define FLUX_ENGINE_SECONDARY_TASK_AFINITY      1
#define FLUX_ENGINE_SECONDARY_TASK_DELAY        15
#define FLUX_ENGINE_SECONDARY_TASK_PRIORITY     10

#define FLUX_MAX_INPUT_CONTROLLERS              2
#define FLUX_MAX_SENSORS                        32

typedef void (*initialize_fn_t)(void* _this);
typedef void (*update_fn_t)(void* _this);
typedef void (*dealloc_fn_t)(void* _this);

typedef struct
{
        void*                   _this;

        initialize_fn_t         initialize;
        update_fn_t             update;
        dealloc_fn_t            dealloc;
} flux_sensor_t;

typedef struct
{
        void*                   _this;

        initialize_fn_t         initialize;
        update_fn_t             update;
        dealloc_fn_t            dealloc;
} flux_input_controller_t;

typedef struct
{
        unsigned int            thrust;
        float                   pitch;
        float                   yaw;
        float                   roll;
} flux_input_params_t;

// Configuration to Setup the Flight Engine
typedef struct
{
} flux_engine_config_t;
