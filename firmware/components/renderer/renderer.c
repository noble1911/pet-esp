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
static palette_t s_pal_body;   // loaded once at boot; lives for process life
static palette_t s_pal_eye;

#define ASSETS_BASE   "/assets"
#define ASSETS_LABEL  "assets"
#define ANCHOR_MAGIC  "PANC"
#define PALETTE_MAGIC "PPAL"
#define MAX_SHAPES    16     // shapes per (stage,layer); gene clamps into this
// Big enough for GCC's worst-case format-truncation analysis: a parent dir
// path plus a NAME_MAX (255) filename plus ".bin" can theoretically be 512+.
// In practice every path we build is short ("/assets/baby/body/round/idle.bin"
// ≈ 35 chars); buffer is over-sized so -Werror=format-truncation accepts it.
#define PATH_MAX_LEN  512

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

// Decode a "PPAL" tint table into PSRAM. File is little-endian RGB565,
// which is native on the (little-endian) ESP32-S3 — read straight in.
static bool load_palette(const char *path, palette_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    uint8_t hdr[8];
    uint16_t *buf = NULL;
    bool ok = false;
    uint8_t entries = 0, ramp = 0;
    size_t n = 0;
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        goto done;
    }
    if (memcmp(hdr, PALETTE_MAGIC, 4) != 0 || hdr[4] != 1
        || hdr[5] == 0 || hdr[6] == 0) {
        ESP_LOGW(TAG, "%s: bad palette header", path);
        goto done;
    }
    entries = hdr[5];
    ramp = hdr[6];
    n = (size_t)entries * ramp;
    buf = heap_caps_malloc(n * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "%s: PSRAM alloc failed", path);
        goto done;
    }
    if (fread(buf, sizeof(uint16_t), n, f) != n) {
        ESP_LOGW(TAG, "%s: short read", path);
        heap_caps_free(buf);
        buf = NULL;
        goto done;
    }
    out->entries = entries;
    out->ramp = ramp;
    out->rgb565 = buf;
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

        if (!load_palette(ASSETS_BASE "/palettes/body.pal", &s_pal_body)) {
            ESP_LOGW(TAG, "body palette missing — body layers untinted");
        }
        if (!load_palette(ASSETS_BASE "/palettes/eye.pal", &s_pal_eye)) {
            ESP_LOGW(TAG, "eye palette missing — eyes untinted");
        }
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

const palette_t *renderer_palette_body(void)
{
    return s_pal_body.rgb565 ? &s_pal_body : NULL;
}

const palette_t *renderer_palette_eye(void)
{
    return s_pal_eye.rgb565 ? &s_pal_eye : NULL;
}

uint16_t renderer_gray_rgb565(uint8_t g)
{
    // Cast to uint32_t first: avoids signed-int promotion in the shifts and
    // makes byte-parity with forge_common.rgb565() explicit/portable.
    uint32_t v = g;
    return (uint16_t)(((v & 0xF8u) << 8) | ((v & 0xFCu) << 3) | (v >> 3));
}

uint16_t renderer_tint_rgb565(const palette_t *pal, uint8_t entry,
                              uint8_t gray)
{
    if (pal == NULL || pal->rgb565 == NULL
        || pal->entries == 0 || pal->ramp == 0) {
        return renderer_gray_rgb565(gray);
    }
    uint8_t shade = (uint8_t)(((uint32_t)gray * pal->ramp) >> 8);
    if (shade >= pal->ramp) {
        shade = pal->ramp - 1;
    }
    return pal->rgb565[(entry % pal->entries) * pal->ramp + shade];
}

// ---- Composition (build-order step 5 phase ④) ------------------------
//
// Back-to-front src-over alpha blend of the loaded, gene-tinted layers
// into an ARGB8888 buffer (LVGL needs per-pixel alpha). Frame 0 only —
// the ~5 fps animation timer is a later build step; §5 permits a static
// placeholder. Layers are placed body-relative: body & pattern at (0,0),
// upper layers at their PANC anchor. Out-of-canvas pixels are clipped.

static uint32_t *s_canvas;          // ARGB8888 (0xAARRGGBB), PSRAM
static int       s_canvas_w, s_canvas_h;
static lv_image_dsc_t s_pet_img;
static lv_obj_t *s_pet_obj;
static lv_obj_t *s_placeholder_obj;   // shown only on load/compose failure

static void anchor_for(const pet_sprites_t *ps, int layer,
                       int *ox, int *oy)
{
    *ox = 0;
    *oy = 0;
    switch (layer) {
    case LAYER_EYES:  *ox = ps->anchors.eyes[0];  *oy = ps->anchors.eyes[1];  break;
    case LAYER_MOUTH: *ox = ps->anchors.mouth[0]; *oy = ps->anchors.mouth[1]; break;
    case LAYER_EARS:  *ox = ps->anchors.ears[0];  *oy = ps->anchors.ears[1];  break;
    case LAYER_TAIL:  *ox = ps->anchors.tail[0];  *oy = ps->anchors.tail[1];  break;
    case LAYER_ACCESSORY:
        *ox = ps->anchors.accessory[0]; *oy = ps->anchors.accessory[1]; break;
    default: break;  // BODY, PATTERN -> (0,0)
    }
}

// Per architecture §5.1: body/tail/ears/pattern use the body_color tint;
// eyes use eye_color; mouth/accessory are untinted (gray).
static const palette_t *tint_for(int layer, const Pet *pet, uint8_t *entry)
{
    switch (layer) {
    case LAYER_EYES:
        *entry = pet->genes[GENE_EYE_COLOR];
        return renderer_palette_eye();
    case LAYER_BODY:
    case LAYER_TAIL:
    case LAYER_EARS:
    case LAYER_PATTERN:
        *entry = pet->genes[GENE_BODY_COLOR];
        return renderer_palette_body();
    default:                  // mouth, accessory
        *entry = 0;
        return NULL;
    }
}

static bool compose_pet(const Pet *pet)
{
    const pet_sprites_t *ps = renderer_pet_sprites();
    if (!ps || !ps->layer[LAYER_BODY].pixels) {
        return false;
    }
    const int W = ps->layer[LAYER_BODY].hdr.width;
    const int H = ps->layer[LAYER_BODY].hdr.height;
    const size_t npx = (size_t)W * H;

    if (s_canvas == NULL || s_canvas_w != W || s_canvas_h != H) {
        if (s_canvas) {
            heap_caps_free(s_canvas);
            // The old buffer is gone — make sure no stale LVGL render can
            // dereference it via s_pet_img if we fail below.
            s_pet_img.data = NULL;
            s_pet_img.data_size = 0;
        }
        s_canvas = heap_caps_malloc(npx * 4, MALLOC_CAP_SPIRAM);
        if (!s_canvas) {
            s_canvas_w = s_canvas_h = 0;
            return false;
        }
        s_canvas_w = W;
        s_canvas_h = H;
    }
    memset(s_canvas, 0, npx * 4);   // fully transparent

    for (int L = 0; L < LAYER_COUNT; L++) {       // back -> front
        const sprite_t *s = &ps->layer[L];
        if (!s->pixels) {
            continue;
        }
        int ox, oy;
        anchor_for(ps, L, &ox, &oy);
        uint8_t entry;
        const palette_t *pal = tint_for(L, pet, &entry);
        const int sw = s->hdr.width, sh = s->hdr.height;
        const uint8_t *fr = s->pixels;            // frame 0, [gray,alpha]

        for (int yy = 0; yy < sh; yy++) {
            int cy = oy + yy;
            if (cy < 0 || cy >= H) {
                continue;
            }
            for (int xx = 0; xx < sw; xx++) {
                int cx = ox + xx;
                if (cx < 0 || cx >= W) {
                    continue;
                }
                const uint8_t *p = fr + ((size_t)yy * sw + xx) * 2;
                uint8_t gray = p[0], a = p[1];
                if (a == 0) {
                    continue;
                }
                uint16_t c = pal ? renderer_tint_rgb565(pal, entry, gray)
                                 : renderer_gray_rgb565(gray);
                uint8_t r = (c >> 8) & 0xF8; r |= r >> 5;
                uint8_t g = (c >> 3) & 0xFC; g |= g >> 6;
                uint8_t b = (c << 3) & 0xF8; b |= b >> 5;

                uint32_t *dst = &s_canvas[(size_t)cy * W + cx];
                uint32_t d = *dst;
                uint8_t da = (d >> 24) & 0xFF;
                if (a == 255 || da == 0) {
                    *dst = ((uint32_t)a << 24) | ((uint32_t)r << 16)
                         | ((uint32_t)g << 8) | b;
                } else {
                    // Straight-alpha src-over with a semi-transparent dst:
                    //   oa = a + da*(1-a)              (<=255 for valid a)
                    //   out = (src*a + dst*da*(1-a)) / oa
                    // Weighting dst by da and dividing by oa (not 255) is
                    // required, else colours go too dark at AA edges.
                    uint32_t ia = 255u - a;
                    uint8_t dr = (d >> 16) & 0xFF;
                    uint8_t dg = (d >> 8) & 0xFF;
                    uint8_t db = d & 0xFF;
                    uint32_t dw = (uint32_t)da * ia / 255u;   // dst weight
                    uint32_t oa = (uint32_t)a + dw;
                    if (oa == 0) {
                        *dst = 0;
                    } else {
                        uint8_t orr = (uint8_t)(((uint32_t)r * a + dr * dw) / oa);
                        uint8_t og  = (uint8_t)(((uint32_t)g * a + dg * dw) / oa);
                        uint8_t ob  = (uint8_t)(((uint32_t)b * a + db * dw) / oa);
                        *dst = (oa << 24) | ((uint32_t)orr << 16)
                             | ((uint32_t)og << 8) | ob;
                    }
                }
            }
        }
    }
    return true;
}

void renderer_draw_pet(const Pet *pet, int x, int y)
{
    if (pet == NULL) {
        return;
    }
    if (renderer_pet_sprites() == NULL) {
        renderer_load_pet_sprites(pet);
    }
    if (!compose_pet(pet)) {
        // Load/compose failed (e.g. assets not flashed yet). Show ONE
        // reusable placeholder — never leak a new lv_obj per call.
        if (s_pet_obj) {
            lv_obj_del(s_pet_obj);
            s_pet_obj = NULL;
        }
        if (s_placeholder_obj == NULL) {
            s_placeholder_obj = lv_obj_create(lv_screen_active());
            lv_obj_set_size(s_placeholder_obj, 128, 128);
            lv_obj_set_style_radius(s_placeholder_obj, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(s_placeholder_obj,
                                      lv_color_hex(0xf5d76e), 0);
            lv_obj_set_style_border_width(s_placeholder_obj, 0, 0);
        }
        lv_obj_set_pos(s_placeholder_obj, x, y);
        return;
    }
    if (s_placeholder_obj) {            // compose succeeded — drop fallback
        lv_obj_del(s_placeholder_obj);
        s_placeholder_obj = NULL;
    }

    s_pet_img.header.magic  = LV_IMAGE_HEADER_MAGIC;
    s_pet_img.header.cf     = LV_COLOR_FORMAT_ARGB8888;
    s_pet_img.header.w      = s_canvas_w;
    s_pet_img.header.h      = s_canvas_h;
    s_pet_img.header.stride = s_canvas_w * 4;
    s_pet_img.data          = (const uint8_t *)s_canvas;
    s_pet_img.data_size     = (uint32_t)s_canvas_w * s_canvas_h * 4;

    if (s_pet_obj == NULL) {
        s_pet_obj = lv_image_create(lv_screen_active());
        lv_image_set_antialias(s_pet_obj, false);  // crisp pixel art
    }
    lv_image_set_src(s_pet_obj, &s_pet_img);
    if (s_canvas_w > 0) {                           // ~64 -> ~128 (§5.4)
        lv_image_set_scale(s_pet_obj, (256 * 128) / s_canvas_w);
    }
    lv_obj_set_pos(s_pet_obj, x, y);
}
