# Character identity and legacy gene storage

## Current artwork version 3

The active renderer uses seven independently illustrated, complete characters.
There are no separately rendered eyes, mouths, ears, markings, colour tints or
body reshaping. See [five characters](five-characters-v1.md).

The existing eight-byte `Pet.genes` field and NVS schema remain unchanged for
save compatibility. Byte 6 values 240–246 explicitly select Sprout, Cloud bunny,
Pebble penguin, Peach kitten, Tiny dragon, Rosy pig and Larry. Every other value selects Sprout,
so an existing save returns to the original artwork without a reset or write.
Byte 7 modulo 8 still selects the personality used in voice context. Other
appearance bytes are retained but have no effect on current graphics.

Options → My pet → Choose character applies an explicit saved choice. Previewing
never changes the pet. Name, identity, needs, personality and progress survive.
The new character catalogue is `data/pet_characters.json`; regenerate with
`python3 scripts/generate_pet_characters.py`. Legacy trait catalogues remain
for personality and backend compatibility with artwork versions 1 and 2.
Breeding and part-based evolution described below are historical design notes,
not active features of the whole-character version.

## Historical gene vector (artwork versions 1–2)

Exactly **8 bytes**, stored in `Pet.genes[8]`. Each byte indexes a part
table or palette. Out-of-range values are reduced modulo the table size at
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
