#include "flux_serialize_utils.h"

int     flux_serializeint32(int32_t value, void* buffer, int32_t buffer_length)
{
        if(buffer_length < sizeof(int32_t))
        {
                return -1;
        }

        *(int32_t*)buffer = value;
        return sizeof(int32_t);
}

int     flux_deserializeint32(int32_t* value, void* buffer, int32_t buffer_length)
{
        if(buffer_length < sizeof(int32_t))
        {
                return -1;
        }

        *value = *(uint8_t*)buffer; buffer+=sizeof(uint8_t);
        *value |= *(uint8_t*)buffer << 8; buffer+=sizeof(uint8_t);
        *value |= *(uint8_t*)buffer << 16; buffer+=sizeof(uint8_t);
        *value |= *(uint8_t*)buffer << 24;
        return sizeof(int32_t);
}
