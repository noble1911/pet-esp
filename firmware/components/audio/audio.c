// audio implementation — skeleton.

#include "audio.h"
#include "esp_log.h"

static const char *TAG = "audio";

void audio_init(void)
{
    // TODO: codec bring-up; defer sound design (architecture §11).
    ESP_LOGI(TAG, "init (skeleton)");
}

void audio_play(sfx_id_t sfx)
{
    (void)sfx;
    // TODO(build-order:12): non-blocking playback.
}
