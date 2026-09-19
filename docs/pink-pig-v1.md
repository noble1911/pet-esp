# Rosy pig and clear spoken names

Rosy pig is the sixth choice in **Options → My pet → Choose character**.
She is a Peppa-inspired pink pig in a red dress, with a rounded snout, rosy
cheeks, upright ears, dark shoes and a curly tail. All 26 poses are complete
illustrations, including fourteen hold/bite poses for seven foods. There are
no separately moved eyes or facial parts. The previous five characters are
unchanged. Selection marker 245 extends the existing schema without changing
name, personality, memory identity or progress.

[Actual chooser](previews/pink-pig-v1/choose-character-5.png) ·
[actual renderer animation](previews/pink-pig-v1/pig-face-loop.gif) ·
[source art and exact prompts](../art/characters-v1/pig/)

## Spoken name fix

The prompt now labels `CURRENT IDENTITY.personal_name` separately from
`character_type` and appearance. Name-only questions request exactly “I'm
<saved name>!” and stop, with no species, colour, snout or clothing added.
Conflicting old introductions cannot override the saved name. Looks are
available when asked about appearance. The base personality no longer calls
every character a sprout or encourages leaf descriptions for pets without leaves.
This changes no saved names and leaves the Haiku model and voice unchanged.

Validation: 17 backend tests pass, including old descriptive memories/history
and personal names differing from character type. Live Haiku smoke checks
using the production prompt and conflicting old history returned “I'm Olive!”
for both “What's your name?” and “Who are you?”, and “I'm Rosy Pig!” when that
was the saved name. An explicit appearance question still received a visual
description. Smoke checks made no pet account, memory or conversation writes.

The full host gameplay/UI suite passes. It covers all 156 frames, all six
choices, successful pig selection and save/reload, failed saves and unchanged
progress. Source crops and actual renderer previews were visually checked.
The six-character compressed atlas is 753,699 bytes. Generator freshness and
RLE round-trip checks pass. Roll back to `five-characters-v1` for the previous
five-character firmware; if a pig had been selected, that firmware shows Sprout
until returning to this version. NVS must never be erased for rollback.

## Deployment — 2026-09-19

Backend commit `1f812cc` is deployed on Ron's Mac mini. Butler is healthy and
all 17 tests pass against its deployed code. The naming fix takes effect
without a device flash.

Firmware `pink-pig-v1` (`bd5a4a8`) builds successfully: `0x245f20` bytes,
24% free in the application partition. Flashing was attempted but USB serial
synchronisation still failed before any writes. Rosy pig is built and ready,
but is not yet installed on the device. Reconnect the device before retrying;
do not erase NVS. Existing firmware and save remain unchanged by this failed
attempt. Logs: `/tmp/pet-pig-final-build.log`, `/tmp/pet-pig-flash.log`.

## Successful flash after USB reconnect — 2026-09-19 20:03 UTC

Reconnecting restored USB communication. Built and flashed commit `bae2114`
(the `pink-pig-v1` code plus deployment notes) via `/dev/cu.usbmodem101`.
Application size remains `0x245f20`, with 24% free. Esptool verified every
written image and finished successfully; NVS was not erased. The voice gateway
recorded the same device's authenticated session ready at 20:03:06 UTC.

USB disappeared during the optional serial boot capture, so this verification
uses the successful flash/hash checks and gateway reconnection, not a physical
screen capture or reread of the saved state. Rosy pig is now installed as
choice 6 of 6. Log: `/tmp/pet-pig-reconnect-flash.log`.
