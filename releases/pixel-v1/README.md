# Pixel-art v1 firmware bundle

Local binaries built using ESP-IDF 5.3.5 for the original Waveshare ESP32-S3-Touch-AMOLED-1.8. Install from the project root:

```sh
./releases/pixel-v1/flash.sh /dev/cu.usbmodem101
```

Return to the shaded bunny:

```sh
./backup/vector-v1/restore.sh /dev/cu.usbmodem101
```

Both commands leave the NVS save partition untouched. Device port may change after reconnecting (`ls /dev/cu.*`). Binaries are gitignored and local-only; copy the entire bundle when moving machines. Check `SHA256SUMS` before using archived binaries. Normal source builds remain available through `./scripts/device.sh build`.

This bundle has now been flashed to the ESP with verified data hashes. Boot checking confirmed saved-pet loading and display/touch/audio initialization without an observed crash during 20 seconds. Build, host interaction, visual and memory-safety checks also passed.
