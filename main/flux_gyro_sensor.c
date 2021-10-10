#include "driver/i2c.h"

#define ACK_CHECK_EN 0x1

void configure_gyro_sensor()
{
        int i2c_master_port = 0;
        i2c_config_t config_master = 
        {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = 21,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_io_num = 22,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master.clk_speed = 400000,
        };

        ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &config_master));
        ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, config_master.mode, 0, 0, 0));

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (0x68 << 1) | I2C_MASTER_WRITE, 0);
        i2c_master_stop(cmd);
        esp_err_t i2c_ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);


        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (0x68 << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x6B, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        i2c_ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        /*
        cmd = i2c_cmd_link_create();
        uint8_t data[6];
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (0x68 << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x43, ACK_CHECK_EN);
        i2c_master_read(cmd, data, 6, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        i2c_ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        */
        if (i2c_ret == ESP_OK)
        {
        printf("I2C Write OK\n");
        /*
                for(int i=0; i < 6; i=i+2)
                {
                        printf("X : %d)\n", data[i] << 8 | data[i+1]);
                }
                */
        }
        if (i2c_ret == ESP_FAIL)
        {
        printf("I2C_fail\n");
        }
        if (i2c_ret == ESP_ERR_INVALID_ARG)
        {
        printf("wrong parameter\n");
        }
        if (i2c_ret == ESP_ERR_INVALID_STATE)
        {
        printf("driver_error\n");
        }
        if (i2c_ret == ESP_ERR_TIMEOUT)
        {
        printf("timeout\n");
        }
 
        i2c_driver_delete(i2c_master_port);
}
