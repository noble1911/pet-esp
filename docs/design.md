# Visual & UX design

Expands [architecture.md §5 (rendering)](architecture.md) and the §10
"polish gate" with the **visual identity** the project is reaching for.
Where the architecture decides *what works*, this doc decides *what it
feels like*. Both must agree; this one moves faster than the architecture
because the visual direction is still being explored.

> Source of truth for: design philosophy, screen layout, icon vocabulary,
> scene plan, asset inventory. Cited by `architecture.md` polish gate.

---

## 1. Design philosophy

These are the rules every screen / asset / interaction decision must
satisfy. If a proposal violates a principle, it gets reworked or the
principle gets updated explicitly here.

1. **Cozy, not utilitarian.** The screen is the toy, not a settings
   panel. The pet lives somewhere real-feeling. Chrome shrinks; the
   scene grows.

2. **Pixel-art with depth.** Backgrounds are layered scenes (floor +
   wall + optional decor) rather than flat fills. The pet sits *in* the
   scene, not on a void.

3. **Icon-first.** Text is reserved for things that *have* to be text —
   the pet's name, possibly age. Stat labels, care actions, menu items
   all use glyphs that a child recognises without reading.

4. **A 5-year-old reads the screen at a glance.** Stats are read by bar
   length and colour. Pet state shows in its face / pose. Menu items
   use universal symbols (heart, food, bed, gear).

5. **Stage-aware scenes.** As the pet grows up, the room gets bigger /
   gains decor / unlocks variations. Visible progression beyond just
   the pet sprite changing.

6. **Every element has weight.** Tamagotchi-Uni look — sharp pixel
   borders, dithered shadows, hard edges. No floaty Material defaults.

These principles inherit from the polish-gate items in
`architecture.md §10` but are *more specific*. The polish-gate item
"Look like a toy" is now defined by this document.

---

## 2. Visual identity

| Aspect | Decision | Open / pending |
|--------|----------|----------------|
| Style | Pixel-art, retro-modern. Tamagotchi-Uni as the primary reference. | — |
| Resolution | Native 368 × 448, scaled assets at integer multiples (1×, 2×) so pixels stay crisp. | — |
| Palette | Warm/cozy: muted greens, butter-yellow accents, soft pinks, warm wood tones. Dark navy stays for negative space. | Palette swatches not yet codified — see §3 references. |
| Typography | UNSCII bitmap (already enabled). Text strictly limited per principle 3. | Whether to also enable a smaller pixel face for tiny status text. |
| Pet style | Tamagotchi-Uni-style abstracted creature (round bodies, expressive eyes, simple shapes — not realistic animals). | Confirm with reference dump. |
| Animation rate | 5 fps for sprite frames, 30 fps for LVGL chrome (already configured). | — |

---

## 3. Reference images

Drop reference screenshots / mood-board images into `docs/references/`
when collected. Target searches:

- **Tamagotchi Uni** — home screen showing the pet in a room with stat
  icons + action dock at the top/bottom.
- **Tamagotchi Pix / Smart** — same family, slightly different layouts.
- **Neko Atsume** — cozy minimalist room art at small resolution.
- **Stardew Valley** farmhouse interiors — pixel-art interiors at our
  scale.
- **Animal Crossing GBA / DS** room shots — cozy interior reference.

Once references land, this section gets a swatch / annotation of what we
took from each.

---

## 4. Screen layout (368 × 448)

```
┌────────────────────────────────────────┐  y=0
│ 🍎▒▒▒▒  ❤▒▒▒▒  💤▒▒▒  💧▒▒▒▒    ⚙    │  y=0–40   stat strip + menu gear
├────────────────────────────────────────┤  y=40
│                                        │
│    [room art — wall + window + floor]  │
│                                        │
│              ⬤  ← pet ~128×128         │  y=40–376 scene area
│           on the floor                 │
│                                        │
├────────────────────────────────────────┤  y=376
│   🍎      🎾      💤      🛁           │  y=376–448 action dock
└────────────────────────────────────────┘  y=448
```

**Compared to the current layout:**

| Region | Now | After |
|--------|-----|-------|
| Stats | 92 px stacked block (4 bars × 22 px rows, with text labels) | 40 px horizontal strip (4 icons + thin bars) |
| Scene | 240 px void with pet in middle | 336 px scene with pet *in* it |
| Action dock | 70 px text buttons (Feed/Play/Rest/Clean) | 72 px icon-only dock + room for a 5th menu/gear |

Implementation: existing `make_stat_bar` and `make_care_button` get
re-skinned; positions move; no new LVGL primitives required.

---

## 5. Icon vocabulary

A small, fixed glyph set used consistently across stats, actions, and
menu. Same glyph means the same thing everywhere.

| Concept | Glyph (semantic) | Size | Use sites |
|---------|------------------|------|-----------|
| Food / hunger | Apple silhouette | 16 px (stat), 32 px (action) | Hunger bar label, Feed button, food picker header |
| Affection / happiness | Heart | 16 / 32 | Happiness bar, Play button |
| Sleep / energy | Bed (or zzz) | 16 / 32 | Energy bar, Rest button |
| Cleanliness / hygiene | Water drop or soap bar | 16 / 32 | Hygiene bar, Clean button |
| Menu | Gear | 24 px | Top-right corner |
| Back / close | Left arrow or X | 24 px | Modal headers |
| Info | Speech bubble or "i" | 24 px | Menu |
| Settings | Slider sliders | 24 px | Menu |

**Sources** (free, CC0):

- [Kenney Game Icons](https://kenney.nl/assets/category:Icons) — first
  port of call for placeholder glyphs. CC0 means no attribution needed.
- [game-icons.net](https://game-icons.net/) — larger library, varied
  licences (most CC-BY).

Final icons may be hand-drawn to match the art track's pixel style;
Kenney/game-icons fills the gap until then.

---

## 6. Scenes (room backgrounds)

Scenes occupy the 368 × 336 area between stat strip and action dock.

| Scene | When shown | Notes |
|-------|-----------|-------|
| **Home / living room** | Default at boot, idle, most actions | Wall + floor + a chair / table / window. Sized so pet stands on the floor at the bottom third. |
| **Bedroom** | While Rest is active (sleep overlay) | Dim wall, bed, moonlight from a window. Sleep "Z z z" still floats. |
| **Kitchen** | While Feed is active (food picker → eat sequence) | Counter / fridge / bowl. Food appears on the counter or floor. |
| **Bathroom** | While Clean is active (when art exists) | Tub + soap. Splash effect on Clean tap. |

Scene-switching is **state-driven, not user-driven** — the user doesn't
navigate scenes; the scene reflects what's happening with the pet right
now.

Scene transitions: instant for v1; cross-fade or door-wipe later if it
feels right. Default scene returns ~1.5 s after the action's reaction
animation finishes.

---

## 7. Menu structure

Gear icon top-right opens a modal. Modal contents (vertical stack):

| Item | Action |
|------|--------|
| Pet info | Shows name, age (in days), stage, generation. Glyph-driven but pet name + age are text. |
| Settings | Brightness slider, sound on/off, decay rate (debug). |
| Inventory | List of items the pet owns (relevant from step 11 onwards). |
| (debug) Hatch fresh | Re-rolls genes, resets needs. For dev only — gate behind a long-press. |

The food picker, sleep overlay, etc. are **not** in this menu — they're
care-action affordances accessed via the action dock.

---

## 8. Asset inventory required

Captures what the design needs that doesn't yet exist. Updated as items
land.

### Backgrounds (368 × 336 PNG, ARGB or RGB565 if opaque)

- [ ] `home.png` — default living room
- [ ] `bedroom.png` — sleep scene
- [ ] `kitchen.png` — feed scene
- [ ] `bathroom.png` — clean scene

### Icons (16 × 16 + 32 × 32 ARGB PNG, transparent bg)

- [ ] `food.png` (×2 sizes)
- [ ] `heart.png` (×2)
- [ ] `bed.png` (×2)
- [ ] `soap.png` (×2)
- [ ] `gear.png` (24 px)
- [ ] `arrow_back.png` (24 px)
- [ ] `info.png` (24 px)

### Optional / nice-to-have

- [ ] Pet emote face overlays (happy, sad, sleepy, surprised) — feeds
      the architecture §5.5 eyes/mouth animation states.
- [ ] Body care-state strips (eat / sleep / play / clean) — feeds the
      polish-gate "per-care body state overrides" item.
- [ ] Decor sprites (chair, bed, plant) — placed in scenes; could be
      gene-influenced or unlocked over time.

---

## 9. Implementation tracks (firmware-side)

These are the **firmware** steps once the philosophy is settled. Track
order is recommended but not strict — anything can land independently.

| Track | Description | Blocked by |
|-------|-------------|-----------|
| **L** Layout overhaul | Re-arrange stat strip (top), scene area (middle), action dock (bottom). Uses placeholder rect-backgrounds until real scenes land. | nothing |
| **I** Icon-ify stats + buttons | Replace label text with icons; replace button text with icons. | first 4 icons (food/heart/bed/soap) |
| **S** Scene rendering | Background image as the screen base layer; pet drawn on top. Default = home. | `home.png` |
| **M** Menu modal | Gear → modal with Pet info / Settings entries. | `gear.png`, `arrow_back.png` |
| **X** Scene swapping | State-driven scene change (kitchen on Feed, bedroom on Rest, etc.). | corresponding scene PNGs |
| **F** Smaller stat strip | Compress the 4-row stacked bars into a 40 px horizontal strip with icons. | I (icons first) |

**Recommended initial sequence** while waiting for art:

1. **Track L** (layout overhaul) with placeholder coloured rectangles
   as backgrounds. Implements the new geometry; design lands even
   without art.
2. **Track I** (icons) as soon as the first 4 icons land — Kenney pack
   covers it.
3. **Track S** (scene rendering) when `home.png` lands.
4. **Track M** (menu) when `gear.png` lands.
5. **Track F** (smaller stat strip) once I is in.
6. **Track X** (scene swapping) last — needs all the scene art.

---

## 10. Open design questions

- **Pet style on the body itself** — abstracted creature vs. animal-styled vs. monster-styled. Tamagotchi-Uni is "abstracted creature"; reference dump will confirm.
- **Text budget** — minimum: pet name. Maybe: pet name + age in days. Should age be a number or a glyph (e.g. small candles)?
- **Decor unlocks** — does the room visibly accrue stuff as the pet ages? Or is decor purely a polish layer?
- **Bar restore-fill animation** — when a care action restores a stat, does the bar grow smoothly (LVGL anim) or jump?
- **Day / night cycle** — does the scene shift with real-world time (dimmer at night)? Or only when sleeping?

These resolve as the reference dump narrows them down.
