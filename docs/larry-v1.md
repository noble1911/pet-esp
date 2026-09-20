# Larry the salaryman

Larry is choice **7 / 7** in **Options → My pet → Choose character**.
The miniature adult salaryman has a charcoal suit, pale shirt, blue cloud-pattern
tie, silver-streaked dark hair and tired eyes, based on the
[official Pokemon character artwork](https://scarletviolet.pokemon.com/en-gb/characters/larry/).
He has 26 independently painted complete poses: idle, wave, blink, talk, listen,
happy, two reaching poses, sleep/breathe, bath/splash, and hold/bite poses for
all seven foods. Facial parts are never composed or moved separately.

[Chooser](previews/larry-v1/choose-character-6.png) ·
[renderer animation](previews/larry-v1/larry-face-loop.gif) ·
[source images and prompts](../art/characters-v1/larry/)

The six previous characters are unchanged. Marker 246 extends the existing
selection mechanism; the NVS struct and schema stay unchanged. Choosing Larry
changes appearance only, preserving the saved personal name, personality,
progress, needs and memory account. Larry is the character label, not a forced
rename. No separate adult account, paid rewards or different care rules.

Voice context adds a gently dry, understated flavour with occasional lunch-break,
meeting or paperwork humour only when Larry is selected. Existing Haiku and
speech voice settings remain unchanged. Personal-name answers remain separate
from appearance. A current-look reminder overrides old descriptive conversation
history when switching characters; descriptions stay to one short sentence.

Validation: full host gameplay/UI suite, all 182 frames and seven choices,
Larry selection/save/reload, failed-save recovery and bounds checks pass.
Seventeen backend tests pass, covering character markers, optional Larry voice
style, old artwork compatibility, name separation and current appearance.
The generated atlas uses 875,409 bytes of RGB565 RLE data, with exact round-trip
checks. Actual renderer images and chooser layout were visually reviewed.

Rollback: `pink-pig-v1` restores the prior six-character firmware. If Larry was
selected, that firmware displays Sprout; returning to this version recognises
Larry again if marker 246 remains saved. Never erase NVS during a rollback.

A live Haiku check with an old Rosy pig introduction in history answered a
current-look question with Larry's tired eyes, silver-streaked hair and cloud
tie. Name questions returned only the saved personal name, including Larry
when that was the supplied name. Current identity and appearance accompany
the latest utterance as clearly labelled device data; only the original
player transcript is stored in conversation history. No memories were changed
by the smoke checks.

## Deployment — 2026-09-20

Backend commit `0fa13dd` is deployed on Ron's Mac mini. Butler is healthy and
all 17 tests pass against the deployed code. Firmware `larry-v1` (`0644f02`)
built with ESP-IDF 5.3.5: application size `0x263b30`, 20% free. USB flash via
`/dev/cu.usbmodem101` completed successfully with all hashes verified. NVS was
not erased. The existing device reconnected to the authenticated voice gateway
at 12:40:27 UTC. Passive USB capture returned no application logs, so the boot
check relies on the gateway reconnect rather than a reread of the saved state
or a physical screen inspection. Larry is installed as choice 7 of 7.

Logs: `/tmp/pet-larry-build.log`, `/tmp/pet-larry-flash.log`,
`/tmp/pet-larry-gateway.log`, `/tmp/pet-larry-test.log`.
