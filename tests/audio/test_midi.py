"""Importer regression: seconds across tempo changes, overlaps, sustain and drums."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import mido

spec = importlib.util.spec_from_file_location('import_midi', Path(__file__).resolve().parents[2]/'scripts/import_midi.py')
importer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(importer)

class MidiImportTest(unittest.TestCase):
    def song(self, messages):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'fixture.mid'
            midi = mido.MidiFile(ticks_per_beat=480)
            midi.tracks.append(mido.MidiTrack(messages))
            midi.save(path)
            return importer.compile_song(path)

    def test_tempo_sustain_overlap_and_velocity(self):
        notes, duration, voices = self.song([
            mido.Message('control_change', control=7, value=127),
            mido.Message('program_change', program=10),
            mido.Message('note_on', note=60, velocity=90, time=480),
            mido.Message('control_change', control=64, value=127),
            mido.Message('note_on', note=64, velocity=70, time=480),
            mido.MetaMessage('set_tempo', tempo=1000000),
            mido.Message('note_off', note=60, time=240),
            mido.Message('note_on', note=64, velocity=0, time=240),
            mido.Message('control_change', control=64, value=0, time=480),
        ])
        self.assertEqual(notes, [[0,2500,60,90,10],[500,2000,64,70,10]])
        self.assertEqual((duration, voices), (2500,2))

    def test_drums_ignore_sustain_and_reset_releases_held_notes(self):
        notes, duration, voices = self.song([
            mido.Message('control_change', channel=9, control=64, value=127),
            mido.Message('note_on', channel=9, note=36, velocity=127),
            mido.Message('control_change', control=64, value=127),
            mido.Message('note_on', note=60, velocity=127),
            mido.Message('note_off', channel=9, note=36, time=480),
            mido.Message('note_off', note=60),
            mido.Message('control_change', control=121, value=0, time=480),
        ])
        self.assertEqual(notes, [[0,500,36,100,128],[0,1000,60,100,0]])
        self.assertEqual((duration, voices), (1000,2))

    def test_library_is_bounded(self):
        for filename, _ in importer.FILES:
            notes, duration, voices = importer.compile_song(importer.ROOT/'assets/music'/f'{filename}.mid')
            self.assertGreater(len(notes),100)
            self.assertLessEqual(voices,64)
            self.assertLess(duration,90000)

if __name__ == '__main__':
    unittest.main()
