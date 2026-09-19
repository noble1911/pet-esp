# Care reactions

These three PNGs are actual LVGL captures of the immediate local feedback for
cuddles, toast and waking from a completed nap. Local feedback works offline,
with sound muted, and with Little chats disabled. Captions disappear after
3.5 seconds unless a conversation is active. Automatic speech has a 45-second
cooldown shared with manual conversation and idle remarks; it never queues up
reactions to play later. Individual star catches and bubble pops stay local;
completing the activity may trigger a spoken reaction.

`voice_note_event` captures a bounded event code with the next voice snapshot,
even when speech is suppressed. This context expires after two minutes and is
not persisted in device NVS. The pet route uses events younger than 15 seconds
for short spontaneous reactions; older events remain context for direct chat.
The backend does not store these events as child speech or lasting preferences.

Checks: host interaction/cooldown/mute/offline/food/cancellation tests passed;
seven backend tests passed; live Haiku calls recognised toast, a cuddle, and a
completed nap. The live check used and removed a separate temporary pet account.
Brighter Sprout, Haiku, the volume slider and default 100% volume are unchanged.
