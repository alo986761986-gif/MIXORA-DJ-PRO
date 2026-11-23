#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parent.parent
cmake=(root/'CMakeLists.txt').read_text()
main=(root/'src/live/main.cpp').read_text()
build=(root/'tools/package_windows.ps1').read_text()
assert 'VERSION 2.5.0' in cmake
assert 'clearSyncLeds();' in main
assert '~LiveWindow() override' in main
assert 'DJ PRO 2.5' in main
assert 'windeployqt' in build and 'ctest --test-dir' in build
assert 'MIXORA DJ PRO.exe' in build
assert 'DJ PRO 2.4' in main # migration path
print('MIXORA 2.5 packaging and safety static checks PASS')
