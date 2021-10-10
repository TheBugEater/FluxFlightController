#pragma once
#include <stdint.h>

int     flux_serializeint32(int32_t value, void* buffer, int32_t buffer_length);
int     flux_deserializeint32(int32_t* value, void* buffer, int32_t buffer_length);
