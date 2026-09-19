# Gene specification

Expands [architecture.md §6](architecture.md). The architecture doc is
authoritative; this file is the working reference for the `pet_state`
component and the sprite forge.

## Current illustrated sprite build

The table below describes the original eight-byte genetics storage format. The current complete-sprite renderer uses **only body colour** for genetic appearance: byte 1 modulo 6 selects Sunny gold, Lilac, Mint, Rose, Peach or Sky blue. The sixteen stored colour values share those six looks. Growth accessories come from life stage, not from another gene.

**My pet** exposes every saved gene. Body shape, eye shape, eye colour, ears, mouth and markings are labelled saved for later; they do not yet affect these sprites. Their numbered variants preserve the original indices without assigning fictional appearances. Personality now supplies explicit voice context from the same catalogue as the UI; the earlier archetypes energetic, grumpy and sleepy are displayed as Bouncy, Feisty and Dreamy. This does not alter genes, needs, rewards or gameplay difficulty. The legacy emote selector remains separate from the illustrated UI.

`data/pet_traits.json` is the current display catalogue. Run `python3 scripts/generate_pet_traits.py` after editing it, or `--check` to verify the committed firmware/voice tables. See [profile and browsing](pet-traits-v1.md).

## Gene vector

Exactly **8 bytes**, stored in `Pet.genes[8]`. Each byte indexes a part
table or palette. Out-of-range values are clamped to the table size at
render time, never rejected.

| Byte | Name          | Range | Selects                                  |
|------|---------------|-------|------------------------------------------|
| 0    | `body_shape`  | 0–7   | Body sprite (layer 0)                    |
| 1    | `body_color`  | 0–15  | Primary tint palette entry               |
| 2    | `eye_shape`   | 0–7   | Eye sprite (layer 4)                     |
| 3    | `eye_color`   | 0–15  | Eye tint palette entry                   |
| 4    | `ear_shape`   | 0–7   | Ear/horn sprite (layer 2)                |
| 5    | `mouth_shape` | 0–7   | Mouth sprite (layer 3)                   |
| 6    | `pattern`     | 0–7   | Pattern overlay (layer 5), body-tinted   |
| 7    | `personality` | 0–7   | Behaviour bias only — **no visual effect**|

`GENE_MAX[8] = {8, 16, 8, 16, 8, 8, 8, 8}` — the per-byte modulus used by
the breeding mixer.

Genes are stable for life: evolution picks a different sprite from the new
stage's library using the *same* genes (architecture §4.3).

## Personality (byte 7)

Does not change appearance. Biases emote selection and reaction weights
(architecture §7.5). Suggested archetypes (0–7), to be tuned:

| Value | Archetype | Bias                                  |
|-------|-----------|---------------------------------------|
| 0     | balanced  | uniform weights                       |
| 1     | shy       | high ❓, low ⚽                        |
| 2     | energetic | high ✨ / ⚽                           |
| 3     | sweet     | high 💕                               |
| 4     | grumpy    | high 👎                               |
| 5     | playful   | high 🎵 / ⚽                           |
| 6     | sleepy    | high 💤                               |
| 7     | curious   | high ✨ / ❓                           |

## Breeding mixer

Both pets must be `adult`. Both devices independently compute the **same**
child genes deterministically (architecture §6.2):

```
seed = hash(min(pa.pet_id, pb.pet_id),
            max(pa.pet_id, pb.pet_id),
            session_timestamp)
for i in 0..7:
    child.genes[i] = (rand(seed) & 1) ? pa.genes[i] : pb.genes[i]
    if rand(seed) % 100 < MUTATION_PCT:
        child.genes[i] = rand(seed) % GENE_MAX[i]
    seed = next_rand(seed)
```

- `MUTATION_PCT` — tunable constant (start ~5).
- `child.generation = max(pa.generation, pb.generation) + 1`.
- `child.parent_a`, `child.parent_b` recorded.
- Cooldown: a pet may breed once / 24 h; a given pair once / 7 days.

The hash and PRNG must be byte-identical across devices — fix the
implementation in `pet_state` and never change it without a schema bump.
