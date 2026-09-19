# Start fresh

Open the cog → **Start fresh...** → **Yes, start fresh**. **Keep my pet** or the back arrow cancels without changing the save.

Confirmation replaces the pet with a newly randomized baby named Sprout, four needs at 100%, zero care stars/growth, empty stickers/inventory/room gifts and no friend history. The device then reboots to clear old speech, captions and activity state. A failed save shows an error and keeps the current in-memory pet; the existing NVS blob is never erased before replacement. There is no on-device undo.

Wi-Fi credentials and the saved Little chats setting remain. As with any reboot, sound starts enabled at the existing 100% default. Normal play and reset work offline.

The pet account on the voice server explicitly enables new pet identities. Its original pet keeps its existing account; each later pet gets a deterministic, device-scoped account on its first voice turn. Facts, vector memories, conversation history and model/tool calls all use that new account. Previous memories remain archived on the server and are not supplied to the fresh pet. This is a fresh start, not a server data-erasure feature. Household Butler accounts, model choices and the gateway's device token remain unchanged.

## Verification

- Production LVGL pointer tests: opening/cancelling/back are harmless; simulated save failure preserves the pet and persisted state; confirmed reset saves once, reboots, clears rewards and survives reload; decay starts fresh. Existing game/music/volume tests also pass.
- Reviewed actual 368×448 renders: [Options](previews/start-fresh-v1/settings.png), [confirmation](previews/start-fresh-v1/start-fresh.png), [save failure](previews/start-fresh-v1/start-fresh-error.png), [new pet](previews/start-fresh-v1/fresh-pet.png).
- Twelve Butler tests cover authorization, opted-in device accounts, stable new identities, separate devices, memory/model binding and account conflict rejection.
- `integrations/scripts/test_pet_reset_accounts.py` checks the real database using temporary accounts within an always-rolled-back transaction: original memories survive, two subsequent pets each start empty and reconnects retain identity. No model calls are required.
- ESP-IDF build and USB flash preserve the real pet save. Boot verified the same Sprout identity (`dffaa588e4bc0d90`), saved stage 3, display/touch, speaker/microphone, Wi-Fi and gateway ready; 43% of the app partition remains free. The destructive action is exercised only in host tests; the real pet is left for the owner to reset.

## Deployment and rollback

Backend commit: `5d61313` in HomeServer; gateway unchanged. Run `integrations/scripts/provision_pet.py` inside `butler-api` to idempotently enable new generations for this device only. It never replaces an existing pet binding or memories.

Firmware checkpoint before this change: `pre-start-fresh-v1` (`f8f8a37`). Build/flash that tag in a separate worktree to remove the reset control. Reverting firmware does not recover an already-reset pet; it continues with the saved new baby. Keep the updated backend for its voice account support. The pre-change server route is backed up at `~/pet-voice-backup/pet-pre-start-fresh.py` on the Mac mini.
