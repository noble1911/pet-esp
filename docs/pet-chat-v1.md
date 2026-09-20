# Pet chat and speech bubbles

**Play → Multiplayer** now has **Ball game** and **Pet chat** choices. Pick a
friend and send a chat invitation. The other player explicitly accepts before
any dialogue is generated. Both devices must have the Pet chat firmware.
Older devices continue to support ball games and are clearly marked as needing
an update if selected for chat.

Two pets share the existing room, showing their saved names and current complete
characters. They speak eight short alternating lines. Both screens show the same
caption; only the speaking pet's device receives and plays audio. The speaking
character talks while the other listens. The gateway waits for playback completion
before starting the next voice. The normal volume/mute controls still apply.

**Go home**, Back or PWR stops a chat. Losing either connection ends it without
an automatic replay; both players must opt in again. **Chat again** requires both
players after a completed conversation. A failed model, synthesis or playback
turn shows a retry message. Chat has no care rewards and never triggers microphone
recording. Existing ball-game rewards and reconnect behaviour are unchanged.

## Characters, owners and cost

The dedicated Butler route generates the entire dialogue with **one pet-scoped
Haiku request**, up to 700 output tokens. Kokoro then synthesizes each line with
the current pet voice settings. There are no per-line model calls or automatic
model retries. Both pets' character style, interests, saved personality and current
care stats are supplied, so Larry and Osono can sound like themselves.

Owner talk refers to “my human” or “my player” and the supplied game-care facts.
It does not retrieve private owner memories, conversation history, names or
biographies. The prompt prohibits invented owner preferences, comparisons,
private information or claims to observe the real world. Neither pet can use
memory, music, household or other tools during a chat. The dialogue is ephemeral
and isn't saved as either pet's personal conversation history.

The new endpoint `/api/voice/pet/playdate-chat` accepts internal gateway calls
only and checks that both named device accounts are virtual-pet accounts. The
existing gateway enforces scoped device tokens, household membership and an
accepted invitation. Character capabilities prevent old firmware receiving chat
audio. Eight short plain-text lines are validated before speech starts.

## Bubble rendering

Home, care celebrations and Pet chat use the same bubble component. Its complete
body, border and stepped tail draw together, with all geometry inside its bounds.
Changing a caption invalidates the entire component rather than only the text.
Text and decoration have one visibility parent. Captions now wrap across lines;
extra-long text uses an ellipsis inside a fixed text area instead of scrolling
horizontally across the border. The entire reply still plays aloud.

The host renderer now supports the device's 40-row partial refresh mode as well
as direct rendering. Tests change short/long captions while sprites animate,
check the top border/background pixels, and compare incremental bubble output
with a complete redraw. Both modes pass. Physical panel verification remains
pending the next USB flash.

[Home bubble](previews/pet-chat-v1/home-speaking.png) ·
[Chat invitation](previews/pet-chat-v1/pet-chat-invitation.png) ·
[Two pets chatting](previews/pet-chat-v1/pet-chat-listening.png)

## Verification and deployment

- Full host gameplay/UI tests pass in direct and 40-row partial rendering modes.
- 70 gateway tests pass, including mutual invitations, eight ordered turns,
  speaker-only binary audio on real WebSockets, stale acknowledgements,
  cancellation, failures, old-device compatibility and joint replay.
- 22 backend tests pass, including internal-only access, virtual-pet account
  checks, no memory/history access, Haiku isolation and malformed dialogue.
- Firmware builds with ESP-IDF: image `0x29c7c0`, `0x63840` bytes (about 398 KiB,
  13%) free. **Not flashed in this change.** Osono and the preceding changes are
  included in this build.

Butler `6c26a5b` and gateway `a57cad4` are deployed on Ron's Mac mini.
`integrations/scripts/check_pet_playdate_chat.py` runs real Haiku and Kokoro
through two isolated simulated pet sockets, using temporary virtual-pet accounts,
and removes those accounts afterwards. The live check passed: four spoken turns per device, the correct alternating speaker, and no care rewards or conversation history. It never contacts the physical pets or
reads their data. Results are saved in
[the live dialogue record](previews/pet-chat-v1/live-chat.json).

Restore firmware from `osono-v1` (`7e52967`) without erasing NVS. The previous
backend commit is `1f55d3f`, and the previous gateway commit is `382552b`.
The prior gateway image is `esp-gateway:before-pet-chat`; its three changed source
files are backed up under `~/pet-chat-backup` on the mini. Keep the existing
playdate SQLite volume when changing versions.

Logs: `/tmp/pet-chat-build.log`, `/tmp/pet-chat-host-test.log`,
`/tmp/pet-chat-partial-test.log`, `/tmp/pet-chat-gateway-tests.log`,
`/tmp/pet-chat-backend-tests.log`, `/tmp/pet-chat-live-v2.log`.
