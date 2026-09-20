#include "arcade.h"
#include "nvs.h"
#include <string.h>
// Separate namespace: old Pet blobs and firmware rollback remain byte-compatible.
typedef struct {uint64_t pet;uint32_t version,best[ARCADE_COUNT];} Records;
static Records load(uint64_t pet)
{
    Records r={0};nvs_handle_t h;size_t n=sizeof r;
    if(nvs_open("pet_arcade",NVS_READONLY,&h)==ESP_OK){
        if(nvs_get_blob(h,"records",&r,&n)!=ESP_OK || n!=sizeof r)memset(&r,0,sizeof r);
        nvs_close(h);
    }
    if(r.pet!=pet || r.version!=1){memset(&r,0,sizeof r);r.pet=pet;r.version=1;}
    return r;
}
unsigned arcade_best(uint64_t pet,arcade_kind_t kind) {return (unsigned)kind<ARCADE_COUNT?load(pet).best[kind]:0;}
bool arcade_save_best(uint64_t pet,arcade_kind_t kind,unsigned score)
{
    if((unsigned)kind>=ARCADE_COUNT)return false;
    Records r=load(pet);if(score<=r.best[kind])return true;r.best[kind]=score;
    nvs_handle_t h;if(nvs_open("pet_arcade",NVS_READWRITE,&h)!=ESP_OK)return false;
    bool ok=nvs_set_blob(h,"records",&r,sizeof r)==ESP_OK && nvs_commit(h)==ESP_OK;
    nvs_close(h);return ok;
}
