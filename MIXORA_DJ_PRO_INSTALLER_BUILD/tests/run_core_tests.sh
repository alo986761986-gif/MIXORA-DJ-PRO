#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build_core
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_midi_mapping.cpp -o build_core/test_midi_mapping
./build_core/test_midi_mapping

c++ -std=c++17 -Wall -Wextra -pedantic tests/test_midi_ports.cpp src/midi/WinMidiInput.cpp src/midi/WinMidiOutput.cpp -o build_core/test_midi_ports
./build_core/test_midi_ports

c++ -std=c++17 -Wall -Wextra -pedantic tests/test_midi_scoped.cpp -o build_core/test_midi_scoped
./build_core/test_midi_scoped

c++ -std=c++17 -Wall -Wextra -pedantic tests/test_midi_identity.cpp -o build_core/test_midi_identity
./build_core/test_midi_identity
