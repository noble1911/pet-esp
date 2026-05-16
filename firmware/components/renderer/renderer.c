// renderer implementation — display + LVGL bring-up via the Waveshare BSP,
// and the LittleFS -> PSRAM sprite loader (build-order step 5, phase ②).
// Gene-palette tinting (③) and layered composition (④) are still stubbed:
// renderer_draw_pet() remains the step-3 placeholder circle until ④.

#include "renderer.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_heap_caps.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "renderer";
static lv_display_t *s_disp;
static bool s_fs_ok;
static pet_sprites_t s_sprites;

#define ASSETS_BASE   "/assets"
#define ASSETS_LABEL  "assets"
#define ANCHOR_MAGIC  "PANC"
#define MAX_SHAPES    16     // shapes per (stage,layer); gene clamps into this
#define PATH_MAX_LEN  192

// pet_stage_t -> directory name (mirrors forge_common.STAGES / architecture).
static const char *const STAGE_DIR[] = {
    "egg", "baby", "child", "teen", "adult", "elder",
};

// sprite_layer_t -> directory name (mirrors forge_common.LAYERS).
static const char *const LAYER_DIR[LAYER_COUNT] = {
    [LAYER_BODY] = "body",   [LAYER_TAIL]    = "tail",
    [LAYER_EARS] = "ears",   [LAYER_MOUTH]   = "mouth",
    [LAYER_EYES] = "eyes",   [LAYER_PATTERN] = "pattern",
    [LAYER_ACCESSORY] = "accessory",
};

// Resting animation per layer for step 5 (architecture §5.5). If the file
// is absent the loader falls back to the first *.bin in the shape dir.
static const char *const LAYER_ANIM[LAYER_COUNT] = {
    [LAYER_BODY] = "idle",   [LAYER_TAIL]    = "idle",
    [LAYER_EARS] = "idle",   [LAYER_MOUTH]   = "neutral",
    [LAYER_EYES] = "normal", [LAYER_PATTERN] = "idle",
    [LAYER_ACCESSORY] = "idle",
};

// Which gene picks each layer's shape. Tail follows the body shape (no tail
// gene exists in gene_spec); accessory has no gene yet -> index 0.
// ASSUMPTION (confirm): tail==body_shape, accessory==slot 0.
static uint8_t layer_gene_value(const Pet *pet, sprite_layer_t layer)
{
    switch (layer) {
    case LAYER_BODY:
    case LAYER_TAIL:      return pet->genes[GENE_BODY_SHAPE];
    case LAYER_EARS:      return pet->genes[GENE_EAR_SHAPE];
    case LAYER_MOUTH:     return pet->genes[GENE_MOUTH_SHAPE];
    case LAYER_EYES:      return pet->genes[GENE_EYE_SHAPE];
    case LAYER_PATTERN:   return pet->genes[GENE_PATTERN];
    case LAYER_ACCESSORY: return 0;
    default:              return 0;
    }
}

static uint8_t format_bpp(uint8_t fmt)
{
    switch (fmt) {
    case SPRITE_FMT_GRAY_ALPHA: return 2;
    case SPRITE_FMT_PALETTIZED: return 1;
    case SPRITE_FMT_RGB565:     return 2;
    default:                    return 0;
    }
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

// Resolve a gene value to a shape directory name within {base}.
// Shapes are the sorted subdirectory list; gene is taken modulo the count
// (gene_spec.md: out-of-range clamps to table size, never rejects).
// Returns false if the layer has no shapes (caller skips the layer).
static bool resolve_shape(const char *base, uint8_t gene,
                          char out[NAME_MAX + 1])
{
    DIR *d = opendir(base);
    if (!d) {
        return false;
    }
    static char names[MAX_SHAPES][NAME_MAX + 1];
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && n < MAX_SHAPES) {
        if (e->d_name[0] == '.') {
            continue;
        }
        char p[PATH_MAX_LEN];
        struct stat st;
        snprintf(p, sizeof(p), "%s/%s", base, e->d_name);
        if (stat(p, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(names[n], e->d_name, NAME_MAX);
            names[n][NAME_MAX] = '\0';
            n++;
        }
    }
    closedir(d);
    if (n == 0) {
        return false;
    }
    qsort(names, n, sizeof(names[0]), cmp_str);
    strcpy(out, names[gene % n]);
    return true;
}

// Pick {shape_dir}/{anim}.bin, else the alphabetically-first *.bin in
// {shape_dir}. The fallback MUST sort: LittleFS readdir order is hash/
// insertion order, so taking the raw-first entry would load a
// boot-dependent, non-deterministic frame once a shape has >1 animation.
static bool resolve_sprite_file(const char *shape_dir, const char *anim,
                                char out[PATH_MAX_LEN])
{
    struct stat st;
    snprintf(out, PATH_MAX_LEN, "%s/%s.bin", shape_dir, anim);
    if (stat(out, &st) == 0) {
        return true;
    }
    DIR *d = opendir(shape_dir);
    if (!d) {
        return false;
    }
    static char bins[MAX_SHAPES][NAME_MAX + 1];
    int nb = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && nb < MAX_SHAPES) {
        const char *dot = strrchr(e->d_name, '.');
        if (dot && strcmp(dot, ".bin") == 0
            && strcmp(e->d_name, "anchors.bin") != 0) {
            strncpy(bins[nb], e->d_name, NAME_MAX);
            bins[nb][NAME_MAX] = '\0';
            nb++;
        }
    }
    closedir(d);
    if (nb == 0) {
        return false;
    }
    qsort(bins, nb, sizeof(bins[0]), cmp_str);
    snprintf(out, PATH_MAX_LEN, "%s/%s", shape_dir, bins[0]);
    return true;
}

// Read a PSPR sheet into a freshly PSRAM-allocated buffer.
static bool load_sprite(const char *path, sprite_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    sprite_header_t hdr;
    bool ok = false;
    uint8_t *buf = NULL;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        goto done;
    }
    if (memcmp(hdr.magic, SPRITE_MAGIC, 4) != 0) {
        ESP_LOGW(TAG, "%s: bad magic", path);
        goto done;
    }
    uint8_t bpp = format_bpp(hdr.format);
    if (bpp == 0 || hdr.width == 0 || hdr.height == 0
        || hdr.num_frames == 0) {
        ESP_LOGW(TAG, "%s: bad header", path);
        goto done;
    }
    size_t sz = (size_t)hdr.width * hdr.height * hdr.num_frames * bpp;
    buf = heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "%s: PSRAM alloc %u failed", path, (unsigned)sz);
        goto done;
    }
    if (fread(buf, 1, sz, f) != sz) {
        ESP_LOGW(TAG, "%s: short read", path);
        heap_caps_free(buf);
        buf = NULL;
        goto done;
    }
    out->hdr = hdr;
    out->pixels = buf;
    ok = true;
done:
    fclose(f);
    return ok;
}

// Decode anchors.bin ("PANC", u8 ver=1, u8 count=5, count*(i16 x,i16 y)).
static bool load_anchors(const char *path, sprite_anchors_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    uint8_t hdr[6];
    bool ok = false;
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        goto done;
    }
    if (memcmp(hdr, ANCHOR_MAGIC, 4) != 0 || hdr[4] != 1 || hdr[5] != 5) {
        ESP_LOGW(TAG, "%s: bad anchors header", path);
        goto done;
    }
    int16_t pts[10];
    if (fread(pts, 1, sizeof(pts), f) != sizeof(pts)) {
        goto done;
    }
    // Order: eyes, mouth, ears, tail, accessory (build.py ANCHOR_KEYS).
    memcpy(out->eyes,      &pts[0], 4);
    memcpy(out->mouth,     &pts[2], 4);
    memcpy(out->ears,      &pts[4], 4);
    memcpy(out->tail,      &pts[6], 4);
    memcpy(out->accessory, &pts[8], 4);
    ok = true;
done:
    fclose(f);
    return ok;
}

static void free_sprites(void)
{
    for (int i = 0; i < LAYER_COUNT; i++) {
        if (s_sprites.layer[i].pixels) {
            heap_caps_free(s_sprites.layer[i].pixels);
        }
    }
    memset(&s_sprites, 0, sizeof(s_sprites));
}

void renderer_init(void)
{
    ESP_ERROR_CHECK(bsp_i2c_init());
    s_disp = bsp_display_start();
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return;
    }
    bsp_display_brightness_set(80);
    ESP_LOGI(TAG, "display + touch + LVGL up (%" PRId32 "x%" PRId32 ")",
             lv_display_get_horizontal_resolution(s_disp),
             lv_display_get_vertical_resolution(s_disp));

    // Mount the flash-resident sprite library read-only (architecture §2/§5).
    esp_vfs_littlefs_conf_t conf = {
        .base_path = ASSETS_BASE,
        .partition_label = ASSETS_LABEL,
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    esp_err_t e = esp_vfs_littlefs_register(&conf);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "assets LittleFS mount failed: %s",
                 esp_err_to_name(e));
        s_fs_ok = false;
    } else {
        size_t total = 0, used = 0;
        esp_littlefs_info(ASSETS_LABEL, &total, &used);
        ESP_LOGI(TAG, "assets mounted at %s (%u/%u bytes)",
                 ASSETS_BASE, (unsigned)used, (unsigned)total);
        s_fs_ok = true;
    }
}

bool renderer_lock(uint32_t timeout_ms)
{
    return bsp_display_lock(timeout_ms);
}

void renderer_unlock(void)
{
    bsp_display_unlock();
}

bool renderer_load_pet_sprites(const Pet *pet)
{
    if (!s_fs_ok || pet == NULL) {
        return false;
    }
    if (pet->stage >= (sizeof(STAGE_DIR) / sizeof(STAGE_DIR[0]))) {
        ESP_LOGW(TAG, "stage %u out of range", pet->stage);
        return false;
    }
    free_sprites();
    const char *stage = STAGE_DIR[pet->stage];

    for (int L = 0; L < LAYER_COUNT; L++) {
        char layer_base[PATH_MAX_LEN];
        char shape[NAME_MAX + 1];
        char shape_dir[PATH_MAX_LEN];
        char sprite_path[PATH_MAX_LEN];

        snprintf(layer_base, sizeof(layer_base), "%s/%s/%s",
                 ASSETS_BASE, stage, LAYER_DIR[L]);
        if (!resolve_shape(layer_base, layer_gene_value(pet, L), shape)) {
            continue;  // no art for this layer — fine, skip
        }
        snprintf(shape_dir, sizeof(shape_dir), "%s/%s", layer_base, shape);
        if (!resolve_sprite_file(shape_dir, LAYER_ANIM[L], sprite_path)) {
            continue;
        }
        if (!load_sprite(sprite_path, &s_sprites.layer[L])) {
            ESP_LOGW(TAG, "layer %s: load %s failed",
                     LAYER_DIR[L], sprite_path);
            continue;
        }
        ESP_LOGI(TAG, "layer %s: %s (%ux%u x%u)", LAYER_DIR[L], sprite_path,
                 s_sprites.layer[L].hdr.width,
                 s_sprites.layer[L].hdr.height,
                 s_sprites.layer[L].hdr.num_frames);

        if (L == LAYER_BODY) {
            char anchors_path[PATH_MAX_LEN];
            snprintf(anchors_path, sizeof(anchors_path),
                     "%s/anchors.bin", shape_dir);
            if (!load_anchors(anchors_path, &s_sprites.anchors)) {
                ESP_LOGW(TAG, "body %s: no/invalid anchors.bin", shape);
            }
        }
    }

    if (s_sprites.layer[LAYER_BODY].pixels == NULL) {
        ESP_LOGE(TAG, "no body sprite for stage %s — nothing to draw",
                 stage);
        free_sprites();
        return false;
    }
    s_sprites.valid = true;
    return true;
}

const pet_sprites_t *renderer_pet_sprites(void)
{
    return s_sprites.valid ? &s_sprites : NULL;
}

void renderer_draw_pet(const Pet *pet, int x, int y)
{
    (void)pet;  // genes/colors/animations honored at step 5 phase ④
    lv_obj_t *body = lv_obj_create(lv_screen_active());
    lv_obj_set_size(body, 128, 128);
    lv_obj_set_pos(body, x, y);
    lv_obj_set_style_radius(body, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xf5d76e), 0);
    lv_obj_set_style_border_width(body, 0, 0);
}
