# Playdates: multiplayer proposal

Status: multiplayer remains a proposal. Play → Multiplayer now has a coming-soon
page; no discovery, invitations or shared games are implemented.
User preference: home Wi-Fi first. Two physical ESP devices, two independent pets.
Assume the second device is the same supported Waveshare board; verify before flashing.

## Recommended experience

Use **Play → Multiplayer**, with a large two-pet icon. Each device shows its pet's
name and portrait. Open Multiplayer to become available; choose the other household
device and send an invitation. The other player sees a big **Play together**
button and Back/Not now. Both players explicitly join. No account entry or
network addresses on the child's screens.

The pets wave hello and appear together in a small playdate meadow. Start with
**Pass the ball**. The ball appears on one player's device with a large tap area.
A tap makes the pet reach and sends the ball in a visible arc off the edge;
it appears on the other device, ready for that player. Ten passes (five each)
fill a shared progress strip and trigger a celebration on both screens.
There is no timer, missed-turn penalty, score competition or losing. Waiting
gets a gentle idle animation; either player can leave at any time. **Again**
starts another round. Existing complete wave/reach/happy sprites work for v1.

Side-by-side devices make the pass especially fun, but holding the devices
separately must also work. A continuous room spanning the physical screens is
a later visual improvement, not required to understand whose turn it is.

A successfully completed round awards one care star to each pet through the
usual local care reward rules. Repeated taps, retries and reconnects cannot
award the same round twice. Cancelled unfinished rounds do not award stars.
The first completed meeting with a distinct pet adds a friendship entry and
an interactive friendship sticker. Replaying with the same pet is unlimited;
there is no one-hour lockout on invitations or games. Further shared milestones
and gifts follow after the basic game is proven on both devices.

## Why revise the original plan

`docs/architecture.md` section 7 and `docs/radio_protocol.md` describe ESP-NOW
discovery, a continuous shared canvas, emotes, trading, dancing and breeding.
`firmware/components/radio/radio.c` is entirely a stub; none of this is already
working. The old fixed-channel/no-Wi-Fi-STA assumption predates the current
voice networking. Its 400-pixel display halves also predate the actual
368 × 448 display layout. Breeding no longer fits the seven independent whole
characters and should not be part of this implementation.

The current pet already connects to the local ESP gateway on Ron's Mac mini.
Use that gateway for home playdates. Multiplayer itself makes no model, speech
recognition or speech synthesis calls, so it needs no additional AI credits.
It requires the home Wi-Fi and Mac mini/gateway to be running; internet access
is not required for the game itself. Voice retains its existing internet
requirements. Do not advertise offline-away-from-home support in this version.

## Technical approach

- Add a small two-player game coordinator to the local ESP gateway project
  (`claude-esp/gateway`), separate from Butler's AI conversation logic. The
  coordinator owns invitations, room membership, turn order and round outcomes.
- Reuse existing device authentication and Wi-Fi configuration. Prefer a
  dedicated lightweight gameplay WebSocket endpoint so game events cannot be
  queued behind streamed voice audio. Keep existing voice clients compatible.
- Device identity is not pet identity: a physical device has a unique registered
  account; its current pet has its own persistent ID and personal name. The
  current `PET_USER_ID` in `voice.c` is hardcoded to the first device. Provision
  the second device separately and make selection/configuration explicit before
  any two-device test. Never clone the first pet's NVS, credentials or memories.
  Preserve the first device's current account and saved pet.
- Availability is scoped to the household's registered devices. No public lobby.
  Publish presence only while the Multiplayer view/playdate is active. An invite
  cannot interrupt eating, reset confirmation or a game without acceptance.
- Exchange only the required appearance snapshot (pet ID, personal name,
  character index/artwork version and stage) and game actions. Both firmware
  builds already contain all seven sprites; no art streaming is needed. Keep
  separate render buffers for both complete characters. Different supported
  firmware versions get an understandable update message rather than a crash.
- Validate message types, sizes, membership and turn ownership. Every room and
  round has a fresh ID; every action has a sequence ID. Stale or duplicate
  actions are ignored, and a full authoritative room snapshot restores clients.
  Animate locally between sparse game events instead of streaming frames.
- The firmware UI receives queued events on its normal LVGL thread. Networking
  must never mutate LVGL objects directly. Share network lifecycle ownership
  cleanly with voice, whose component currently initialises Wi-Fi itself.
- Pause automatic pet chatter during multiplayer rounds. Use existing local
  sound effects for passes, arrival and celebration. Do not start two AI pets
  talking to one another or stream one player's microphone to the other toy.
- Heartbeats detect a lost peer. Show a gentle waiting message, then return
  both pets home when the session expires. Power-off, Back and Start fresh
  leave the room cleanly. A reconnect may resume the same round only when the
  coordinator still has it; a server restart ends an unfinished round safely.
- Store completed-round receipts durably and apply each local award once,
  with recovery for power loss between receiving a result and saving it.
  Retain unacknowledged results on the server for delivery after reconnect.
  Keep multiplayer progress versioned; preserve and migrate existing saves.
  Test power-loss recovery rather than treating two device writes as atomic.

## Build sequence and exit checks

1. **Two real devices online.** Identify the second board; provision its own
   device account and pet; verify both on the gateway at once. Confirm separate
   names, saves and voice memory identities. Add compatible gameplay capability
   negotiation and a controllable test client before UI work.
2. **Meet and leave.** Build Multiplayer, presence, invitation/accept/decline,
   two-character rendering and hello/goodbye. Test simultaneous invitations,
   already-busy devices, identical character choices, duplicate names, Back,
   PWR, Wi-Fi loss and reconnection. This is the first on-device milestone.
3. **Pass the ball.** Add alternating turns, visible travel, shared ten-pass
   progress, celebration and replay. Test actual touch latency on both boards,
   input spam, duplicated/delayed messages and cancellation at every transition.
4. **Friendship and rewards.** Add first-friend sticker, friendship counter and
   durable round awards. Test failed NVS saves, reboot before/after award,
   lost acknowledgements and server restart. No duplicate stars or lost
   already-earned rewards; replaying with the same friend always stays possible.
5. **Polish and release.** Check that voice returns normally after play, measure
   gameplay responsiveness alongside voice traffic, test every character pair
   in the host renderer and representative pairs on hardware. Keep a rollback
   tag before each firmware/backend release; never erase the existing pet.

The first release is steps 1–5: discovery/invites, greeting, one cooperative
game, graceful leaving/reconnect and a first friendship reward. Hardware
verification needs both physical devices available, not just two simulated
clients or one board reflashed twice.

## Follow-on playdates

- **Picnic:** choose unlocked snacks and eat together; both choose their own
  food. Sharing means a pretend treat/copy, not removing earned inventory.
- **Dance duet:** alternate large drum/note buttons to build a short shared
  melody using the local music engine. No timing test or AI composition required.
- **Friendship milestones:** earn a buddy biscuit recipe, paired wall sticker,
  framed picture or friendship bunting after completed shared activities.
  Use cumulative progress, never streaks or penalties for not visiting.
- **Room visits:** choose whose room to visit, see their placed gifts, tap a
  guest-friendly interaction. Later add the original walk-between-screens
  effect with new complete walking frames for all seven characters.
- **Away-from-home play:** introduce an ESP-NOW transport after home play is
  solid. Keep the gameplay event model portable, but design Wi-Fi channel
  coordination and offline session authority explicitly when building it.

Defer breeding, destructive item trades, leaderboards, chat messaging and
unrestricted online matchmaking. They add little to the two-person playdate
and would expand the reliability and UI work substantially.
