# pet-esp

A Tamagotchi-Uni-inspired virtual pet for the **Waveshare ESP32-S3-Touch-AMOLED-1.8**.
Two devices can meet over ESP-NOW and share a play space across their screens.

> The design source of truth is **[docs/architecture.md](docs/architecture.md)**.
> Implementation follows those decisions unless the doc is explicitly revised.

## Layout

```
firmware/        ESP-IDF / PlatformIO firmware (the device)
  src/main.c     Entry point
  components/    pet_state, renderer, radio, ui, audio
  assets/        Built .bin sprites (gitignored — produced by sprite_forge)
sprite_forge/    Laptop-side Python tool: PNG art -> .bin sprite sheets
docs/            Architecture and sub-specs
```

## Status

Project skeleton only. See the **build order** in
[docs/architecture.md §10](docs/architecture.md) — each step ends with a
working, demoable artifact, and steps are done in order. Component sources
are stubs marked with `TODO(build-order:N)` against that list.

## Getting started

Firmware (PlatformIO + ESP-IDF):

```
cd firmware
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial log
```

Sprite forge (laptop, Python 3.10+):

```
cd sprite_forge
pip install -r requirements.txt
python build.py          # build all .bin assets into ../firmware/assets
python preview.py        # interactive preview window
python random_grid.py    # save a 4x4 PNG of random pets
```

## Sub-specs

- [docs/gene_spec.md](docs/gene_spec.md) — 8-byte gene vector + breeding
- [docs/radio_protocol.md](docs/radio_protocol.md) — ESP-NOW beacon / handshake / sync
- [docs/sprite_format.md](docs/sprite_format.md) — on-device `.bin` sprite format
