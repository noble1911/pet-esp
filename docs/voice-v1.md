# Sprout's voice — first version

Sprout is the default name. Tap its name at home to open the talking screen, then hold **Talk**, speak, and release. Holding **BOOT** opens the same screen and records while held. Recording stops after 20 seconds. Do not hold BOOT while powering on: that is the ESP's download-mode shortcut. PWR retains its existing hardware function.

**Options → Pet name** changes the saved name without changing identity or memories. Names use 1–15 letters/spaces/apostrophes/hyphens and start with a letter. Existing saves are migrated without resetting progress.

**Options → Wi-Fi & voice connection** shows Wi-Fi connectivity, network name, IP, signal strength and gateway readiness. **Check now** sends a ping and reports a reply or timeout. This checks the gateway connection, not every upstream AI service. Existing Wi-Fi credentials are reused. There is no network picker/password editor in this version. Normal pet care works offline.

Voice and sound effects share one speaker worker and the existing volume/mute controls. The microphone is gated by push-to-talk, never sent continuously. Reply text appears on the talk screen and the mouth animates during playback. Leaving that screen cancels the voice turn. A network interruption stops capture/playback and reconnects automatically.

## Connection and identity

- ESP → LAN `ws://192.168.1.117:8770/ws` → existing `esp-gateway` on Ron's Mac mini.
- Groq STT → Butler `/api/voice/pet/stream` → Kokoro `bf_emma`, PCM16 mono 16 kHz.
- Account: `pet-meadow-3cdc756e3104`, a separate `virtual_pet` profile. The scoped device token cannot switch to adult accounts.
- The first successful turn binds the account to the saved pet ID. Renaming retains memory; a future reset to a new pet ID requires a new account/rebinding, to prevent accidental memory inheritance.
- Each start/end of speech includes name, pet ID, stage, fullness, happiness, energy, cleanliness, care stars, all genes, generation, inventory, friends met and activity. Snapshot is copied on the UI thread; network work never blocks LVGL.
- Pet replies use a dedicated child-oriented system prompt, not the adult Butler persona. Only remember/recall tools are exposed, with user IDs forced by code. No household, email, calendar or display tools are available.
- Conversation history and facts reuse Butler's PostgreSQL/pgvector stack, isolated by account. Fresh device stats override historical statements; spoken requests cannot directly alter game state.

Credentials live only in ignored `firmware/components/voice/secrets.h`; it contains the Wi-Fi macros and the pet-only token. The backend API key stays on the Mac mini. LAN WebSocket transport is intended for the trusted home network.

## Sources and deployment

- Firmware: `firmware/components/voice/`, shared audio worker and LVGL UI.
- Gateway: `/Users/ron/random/claude-esp/gateway/`, branch `codex/pet-voice-v1`; deployed to `~/esp-gateway` on the mini.
- Butler: `~/home-server/butler/api/routes/pet.py`, mounted from `voice.py`. Exact module, tests and provisioning script are also preserved in this repo under `integrations/`.
- Dependencies are pinned/locked; LVGL 9.4 and 40-row display buffers avoid unexpected upgrades and the board's PSRAM/SPI transfer limit.

Build/flash: `./scripts/device.sh -p /dev/cu.usbmodem101 build flash`.

## Restore points

`pre-voice-v1` is the previous pixel-art firmware with volume control, commit `7017e70`. Use a separate worktree to inspect/build that source without losing current work. The earlier `vector-art-v1` and `pixel-art-v1` restore points are also retained.

A full 16 MB device backup, including the pre-voice NVS state, is stored locally in ignored `backup/pre-voice-v1/device.bin`, with a checksum. Restoring this full image would also rewind saved progress. To preserve current progress, build/flash only the prior firmware from its Git restore point or use the existing vector/pixel binary restore scripts.

Mac mini backups: `~/pet-voice-backup/{voice.py,session.py,butler.py,gateway.env}` and Docker image `esp-gateway:pre-pet-voice`. Remove the pet router import from `voice.py` (or restore the saved file if no subsequent changes), restart `butler-api`, and restore/rebuild the old gateway sources to roll back backend code. The isolated pet account can remain without affecting other users. Do not restore old files over newer unrelated backend edits.

## Verification

- Host simulator: existing care, growth, persistence, navigation, mute and volume checks; new name validation/persistence, naming UI and hold/release interaction. New screen previews in `docs/previews/voice-v1/`.
- Gateway: 26 tests passed, including fresh snapshots, locked identity, pet route selection and recovery from STT failure.
- Butler: 5 tests passed for state validation, account checks, pet-ID binding, memory scoping and restricted tools.
- Live Mac mini test: synthesized speech → real Groq transcription → Claude pet response → Kokoro audio (253,776 PCM bytes). Reconnected session recalled a stored fact; database confirmed a vector embedding. Changed fullness was reflected in the next reply. Temporary test account and its memories were deleted afterwards.
- Device boot verified: saved stage/needs retained, display/touch initialized, both codecs opened, LAN IP `192.168.1.58`, gateway authenticated as the pet. The initial display SPI error was resolved with the 40-row buffer.
- Physical button feel, microphone pickup with a child's voice and perceived speaker loudness still need the owner's hands-on check.

Backend commits: HomeServer `a1ff4a2` (pushed on `codex/pet-voice-v1`), claude-esp `62c12b9`.
