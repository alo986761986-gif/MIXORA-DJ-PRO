from pathlib import Path
p=Path(__file__).resolve().parents[1]
m=(p/'src/live/main.cpp').read_text()
h=(p/'src/midi/MidiMapping.h').read_text()
assert 'mapping_.lookup(deviceKeyForPort(event.device),msg)' in m
assert 'midiLearnPort_->currentData().toString()' in m
assert 'midi/bindings_v2' in m
assert 'DJ PRO 2.4' in m
assert 'lookup(const std::string& deviceKey,MidiMessage m)' in h
assert 'midi_persistent_identity' in (p/'CMakeLists.txt').read_text()
assert 'DeviceIdentity.h' in (p/'src/midi/WinMidiInput.cpp').read_text()
print('2.4 static integration: OK (not a Windows build)')
