# Offline nursery-rhyme library

These nine MIDI files were supplied by the user in `~/Downloads` and copied
unchanged on 2026-09-20. They replace the previous three short demo tunes.
Their arrangements are played through the pet's compact toy synth; these
files are not recordings or a General MIDI soundfont.

| Source | Menu label |
| --- | --- |
| baabaablacksheep.mid | Baa Baa Black Sheep |
| happy_and_you_know.mid | If You're Happy |
| humpty_dumpty.mid | Humpty Dumpty |
| jack_and_jill.mid | Jack and Jill |
| jolly_good_fellow.mid | Jolly Good Fellow |
| london_bridge.mid | London Bridge |
| mary_had_lamb.mid | Mary Had a Little Lamb |
| old_mac_donald.mid | Old MacDonald |
| twinkle_little_star.mid | Twinkle Twinkle |

To add or replace songs, edit `FILES` in `scripts/import_midi.py`, install
`mido==1.3.3` in a Python environment, then run:

```sh
python scripts/import_midi.py
python tests/audio/test_midi.py
./scripts/test_audio.sh /tmp/pet-midi-auditions
```

The generated `firmware/components/audio/music_data.inc` is committed, so
firmware builds need no Python MIDI dependency or access to Downloads.
Timing follows MIDI tempo changes. The importer retains overlapping notes,
velocity, program selection and sustain, samples channel volume/expression
at note-on, and removes only leading/trailing silence. The bounded renderer
supports 64 voices (these songs need at most 59), simple piano/pluck, bell,
sustained, bass and percussion timbres. Stereo panning, pitch bends, modulation,
reverb/chorus and changes to gain during an already-held note are not rendered.
Song playback is offline and incurs no API calls. AI composition is separate.
