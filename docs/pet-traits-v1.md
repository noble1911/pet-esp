# My pet and traits

Tap **My pet** beside the pet on the home screen. The profile shows the saved name, life stage, care stars and personality, plus eight trait cards over two pages. Tap any card to explore its variants with the left/right arrows. The back arrow returns to the profile; the profile back arrow returns home.

- **Coat colour:** Sunny gold, Lilac, Mint, Rose, Peach, Sky blue. Preview uses a temporary copy of the pet and the production sprite renderer. All sixteen stored colour genes map to these six colours by modulo six, exactly as before.
- **Personality:** Balanced, Shy, Bouncy, Sweet, Feisty, Playful, Dreamy, Curious. Each has a short, gentle description. Voice receives the pet's actual saved personality on every turn, including automatic remarks. It stays kind regardless of personality and never changes care rules or rewards.
- **Body shape, eye shape, eye colour, ears, smile and markings:** all saved variants are browsable. These six genes are dormant in the current complete-sprite artwork. Grey cards and explicit detail text say that their looks are saved for later. Numbered names identify stored choices without inventing a visual effect.

“This one is mine!” identifies the current choice. “Just looking” identifies alternatives. Browsing does not mutate genes, name, identity, stats, care stars, inventory, or the NVS save. Voice always gets the actual pet, never the browsing preview. No rarity, unlock requirement or cost is implied. Existing pets need no reset or migration.

## Catalogue and voice

`data/pet_traits.json` is the source for both the firmware catalogue and `integrations/butler/pet_traits.py`. `python3 scripts/generate_pet_traits.py --check` detects stale generated files. `pet_trait_choice` mirrors the existing coat mapping and safely handles every byte value. The backend derives readable traits from the authoritative snapshot, rather than asking the model to infer meanings from raw gene numbers. It explicitly distinguishes visible traits, personality and stored traits.

Deploy the generated `pet_traits.py` next to the route's `pet.py` in HomeServer. Backend commit `236a6f8`; the gateway is unchanged. Haiku and the pet's existing memory isolation remain unchanged.

## Validation

- Existing LVGL touch/game/reward/music/reset suite plus home → profile, both pages, all eight cards, forward/backward wraparound through every variant, and return navigation.
- Host tests verify every byte value for every gene and that browsing leaves saved data and genes unchanged.
- Thirteen backend tests cover derived trait context, all byte values, invalid gene input, personality/appearance distinction and existing memory/model isolation.
- Production screen renders reviewed: [overview](previews/pet-traits-v1/overview.png), [coat preview](previews/pet-traits-v1/trait-1-choice-5.png), [personality](previews/pet-traits-v1/trait-7-choice-4.png). Individual trait/colour/personality captures are in the same directory.
- ESP-IDF build and USB flash; post-flash boot checks preserve the current pet and confirm display/touch/audio/Wi-Fi/gateway readiness.

## Rollback

Firmware checkpoint **pre-pet-traits-v1** (`32c6ed9`). Build/flash that tag in a separate worktree to remove the profile while keeping the pet save. The backend may stay updated for readable genetics in voice; its pre-change route is backed up on the Mac mini as `~/pet-voice-backup/pet-pre-traits.py`. Backend pre-change commit is `5d61313`.
