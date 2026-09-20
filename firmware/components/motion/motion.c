// QMI8658C register map: Waveshare QMI8658C datasheet sections 5.4 and 5.7.
#include "motion.h"
#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static i2c_master_dev_handle_t dev;
static bool write_reg(uint8_t r,uint8_t v) {uint8_t b[]={r,v};return i2c_master_transmit(dev,b,2,20)==ESP_OK;}
bool motion_init(void)
{
    i2c_master_bus_handle_t bus=bsp_i2c_get_handle();if(!bus)return false;
    for(unsigned addr=0x6a;addr<=0x6b;addr++){
        if(i2c_master_probe(bus,addr,20)!=ESP_OK)continue;
        i2c_device_config_t c={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=addr,.scl_speed_hz=400000};
        if(i2c_master_bus_add_device(bus,&c,&dev)!=ESP_OK)continue;
        uint8_t reg=0,id=0;
        if(i2c_master_transmit_receive(dev,&reg,1,&id,1,20)==ESP_OK && id==5 &&
           write_reg(8,0) && write_reg(2,0x40) && write_reg(3,6) && write_reg(6,3) && write_reg(8,1)){
            ESP_LOGI("motion","QMI8658 ready at 0x%02x: +/-2g, 125 Hz",addr);
            vTaskDelay(pdMS_TO_TICKS(30));
            uint8_t r=0x35,b[6];
            if(i2c_master_transmit_receive(dev,&r,1,b,6,20)==ESP_OK)
                ESP_LOGI("motion","Gravity sample: x=%.3fg y=%.3fg z=%.3fg",(int16_t)(b[0]|b[1]<<8)/16384.f,(int16_t)(b[2]|b[3]<<8)/16384.f,(int16_t)(b[4]|b[5]<<8)/16384.f);
            return true;
        }
        i2c_master_bus_rm_device(dev);dev=NULL;
    }
    ESP_LOGW("motion","Tilt unavailable; touch control remains available");return false;
}
bool motion_read(float *x,float *y)
{
    if(!dev)return false;
    uint8_t r=0x35,b[6];if(i2c_master_transmit_receive(dev,&r,1,b,6,12)!=ESP_OK)return false;
    // Same portrait mapping as Waveshare's original Gravitysphere app: -Y, +X.
    *x=-(int16_t)(b[2]|b[3]<<8)/16384.f;*y=(int16_t)(b[0]|b[1]<<8)/16384.f;
    return true;
}
