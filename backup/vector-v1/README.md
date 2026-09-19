# Vector-art v1 rollback

These local firmware binaries preserve the working shaded-bunny version before the pixel-art pivot. Restore with `./backup/vector-v1/restore.sh /dev/cu.usbmodem101` from the project root. This restores the application/display assets but does not overwrite NVS, so pet identity, stars and needs are retained. The pre-pivot source is saved by Git tag `vector-art-v1`.

The binary bundle is local-only (gitignored); keep this folder if moving machines. `SHA256SUMS` verifies the four binaries. The older full-device backup in `backup/2026-09-19/` is a different project and is not the rollback for this graphics change.
