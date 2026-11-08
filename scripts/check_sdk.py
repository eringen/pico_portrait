#!/usr/bin/env python3
"""Reject drift in the SDK and the three submodules used by this firmware."""
import pathlib
import subprocess
import sys

PINS = {
    '.': 'a1438dff1d38bd9c65dbd693f0e5db4b9ae91779',  # SDK 2.2.0
    'lib/tinyusb': '86ad6e56c1700e85f1c5678607a762cfe3aa2f47',  # 0.18.0
    'lib/cyw43-driver': 'dd7568229f3bf7a37737b9e1ef250c26efe75b23',
    'lib/lwip': '77dcd25a72509eb83f72b033d219b1d40cd8eb95',
}
root = pathlib.Path(sys.argv[1]).resolve()
for path, expected in PINS.items():
    def git(*args):
        return subprocess.check_output(['git', '-C', str(root / path), *args], text=True).strip()
    if git('rev-parse', 'HEAD') != expected:
        sys.exit('SDK pin mismatch: ' + path)
    if git('status', '--porcelain', '--untracked-files=no'):
        sys.exit('Tracked SDK changes: ' + path)
print('SDK 2.2.0 and required submodule pins verified')
