# Sprout's voice — first version

Sprout is the default name. Hold the speech bubble on the home screen, speak, and release. Holding **BOOT** records while held without leaving the current screen or interrupting a care activity. Recording stops after 20 seconds. Do not hold BOOT while powering on: that is the ESP's download-mode shortcut. PWR retains its existing hardware function.

**Options → Pet name** changes the saved name without changing identity or memories. Names use 1–15 letters/spaces/apostrophes/hyphens and start with a letter. Existing saves are migrated without resetting progress.

**Options → Wi-Fi & voice connection** shows Wi-Fi connectivity, network name, IP, signal strength and gateway readiness. **Check now** sends a ping and reports a reply or timeout. This checks the gateway connection, not every upstream AI service. Existing Wi-Fi credentials are reused. There is no network picker/password editor in this version. Normal pet care works offline.

Voice and sound effects share one speaker worker and the existing volume/mute controls. The microphone is gated by push-to-talk, never sent continuously. Reply text appears in the home speech bubble and the mouth animates during playback. The room and care controls remain visible; there is no separate chat screen. A network interruption stops capture/playback and reconnects automatically.

## Connection and identity

- ESP → LAN `ws://192.168.1.117:8770/ws` → existing `esp-gateway` on Ron's Mac mini.
- Groq STT → Butler `/api/voice/pet/stream` → Kokoro `af_sky` (Brighter Sprout), speed 1.05 and +3 semitones with duration compensation; PCM16 mono 16 kHz.
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

Final device verification: firmware source `d89bc64`, flashed and reset successfully. A 20-second boot capture showed both codecs, display/touch and authenticated pet gateway ready, with no error/panic lines. Saved pet ID, stage and needs were retained. Firmware SHA-256: `698e35d4c5b806a8d10df1b0159046aa1e5d425153e0a22afec0fc02091e8db6`.

## Inline voice / microphone fix

A user test exposed a real device allocation failure that the earlier backend tests did not cover: `xStreamBufferCreate(96*1024)` allocates internal RAM, regardless of general PSRAM settings. The device had only 49,152 bytes in its largest free internal block. Allocation failed and the old early return silently skipped creating both audio tasks, despite successful codec initialization.

Speech storage now explicitly uses PSRAM with `xStreamBufferCreateStatic`; its control structure stays internal. Buffer/task failures are logged and surfaced before listening. Boot confirms both tasks running. Audio start/end diagnostics include microphone frame counts. Recording intent is separate from network status, so a delayed listening acknowledgement cannot swallow release. Pet sessions report empty/quiet captures as retry messages instead of silently going idle.

Voice now stays in the normal room, with a holdable speech bubble, listening level feedback, thinking/reply text, and mouth animation. BOOT also keeps other care screens in place. Host checks cover touch hold/release, BOOT press/release, the 20-second recording limit, and preserving an active bath. All 28 gateway tests pass, including empty microphone and silence feedback.

Live device retest after the fix: one 1.20-second hold sent 38,400 PCM bytes; Groq transcribed “Hello.”, the dedicated pet endpoint returned HTTP 200, and Kokoro generated the reply. Device logs show microphone frame counts increasing from 3,078 to 3,138 during that recording.

## Haiku and occasional little chats

Only the pet uses `claude-haiku-4-5-20251001`, including memory-tool follow-ups. The shared streaming helper accepts a request-local model override; default Butler and routing settings are untouched. Pet replies cap output at 300 tokens, spontaneous remarks at 100. No web-search tools or fallback to a larger model are enabled for the pet. Recent conversation context is limited to six messages; deliberate conversations retain scoped vector memory.

With **Options → Little chats: on**, the pet can volunteer a brief, state-aware comment in its home room. The first opportunity is about 90 seconds after boot; subsequent opportunities are randomly spaced 4–7 minutes apart. Manual conversations reset that cooldown. Automatic remarks are skipped while muted, at zero volume, during care activities or an ongoing conversation, offline, and after ten minutes without touch/BOOT interaction. The setting persists across restarts. Automatic remarks do not record or transmit microphone audio; hold-to-talk still interrupts it normally.

Spontaneous remarks get an explicit automatic-turn instruction, no memory-writing tools or semantic lookup, and are stored only as the pet's own assistant messages (not invented child speech). This keeps them short and avoids paying for unnecessary background memory work.

The shared helper change is preserved as `integrations/butler/llm_override.patch`; deploy it with the updated pet route. Backend isolation tests cover ordinary routing before/after tool use and pet override persistence across tool rounds.

Live spontaneous-turn check passed: the ESP requested a remark at uptime 91.967 seconds, the pet route returned successfully, Kokoro generated speech, and the saved assistant message carried `proactive: true`. The running backend confirmed ordinary Butler `claude-opus-5` and pet `claude-haiku-4-5-20251001`. Gateway: 29 tests; backend: 6 tests; host gameplay/chatter guards passed.

## Brighter Sprout voice

The pet gateway applies `af_sky`, speed 1.05, and a three-semitone pitch/formant
lift to pet sessions only. ffmpeg compensates the duration and emits mono PCM16
at the device playback rate. Normal Butler sessions retain their configured
voice and unprocessed synthesis path. Firmware and the Haiku model are unchanged.

The selected audition and reproducible settings are in `art/voice-auditions/`.
Gateway image now includes ffmpeg; processing is asynchronous, times out after
30 seconds, and terminates its subprocess when a voice turn is cancelled.
Pre-change deployment files are preserved on the Mac mini in
`~/pet-voice-backup/pre-brighter-sprout/`.
