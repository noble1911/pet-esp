#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/components/power/power.c"
static uint8_t regs[256];
static int read_fail=-1,write_fail=-1;
static bool bus_ok=true;
static unsigned off_writes;
i2c_master_bus_handle_t bsp_i2c_get_handle(void) {return bus_ok?(void*)1:NULL;}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus,const i2c_device_config_t *cfg,i2c_master_dev_handle_t *out)
{assert(bus && cfg->device_address==0x34);*out=(void*)2;return ESP_OK;}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t dev,const uint8_t *reg,size_t n,uint8_t *out,size_t len,int ms)
{assert(dev && n==1 && len==1 && ms==50);if(*reg==read_fail)return 1;*out=regs[*reg];return ESP_OK;}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev,const uint8_t *data,size_t n,int ms)
{
    assert(dev && n==2 && ms==50);if(data[0]==write_fail)return 1;
    if(data[0]==REG_IRQ_STATUS1)regs[data[0]]&=~data[1];
    else regs[data[0]]=data[1];
    if(data[0]==REG_COMMON)off_writes++;
    return ESP_OK;
}
const char *esp_err_to_name(esp_err_t e) {(void)e;return "mock failure";}
int main(void)
{
    assert(!power_take_short_press() && !power_request_off());
    bus_ok=false;assert(!power_init());bus_ok=true;
    regs[REG_IRQ_ENABLE1]=0x95;regs[REG_IRQ_STATUS1]=0xef;regs[REG_KEY_TIMING]=0x1f;
    assert(power_init());
    assert(regs[REG_IRQ_ENABLE1]==0x9d && regs[REG_IRQ_STATUS1]==0xe7);
    assert(regs[REG_KEY_TIMING]==0x1c); // off-level and IRQ-level untouched
    assert(!power_take_short_press());
    regs[REG_IRQ_STATUS1]|=8;assert(power_take_short_press());assert(!power_take_short_press());
    assert(regs[REG_IRQ_STATUS1]==0xe7); // charger/long-press flags untouched
    regs[REG_IRQ_STATUS1]|=8;read_fail=REG_IRQ_STATUS1;
    assert(!power_take_short_press() && regs[REG_IRQ_STATUS1]==0xef);read_fail=-1;
    write_fail=REG_IRQ_STATUS1;assert(!power_take_short_press());write_fail=-1;
    assert(power_take_short_press() && !power_take_short_press());
    regs[REG_COMMON]=0xa4;read_fail=REG_COMMON;assert(!power_request_off() && !off_writes);read_fail=-1;
    write_fail=REG_COMMON;assert(!power_request_off() && !off_writes);write_fail=-1;
    assert(power_request_off() && off_writes==1 && regs[REG_COMMON]==0xa5);
    write_fail=REG_IRQ_ENABLE1;assert(power_init());assert(!s_key_ready);
    assert(!power_take_short_press() && !power_request_off());
    puts("PASS: PWR short press once, boot clear, unrelated IRQ preservation, short wake timing, power-off RMW, I2C failure handling");
}
