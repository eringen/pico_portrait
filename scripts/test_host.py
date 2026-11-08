#!/usr/bin/env python3
"""Host checks only. Does not emulate RP2040, USB, I2C, radio, or lwIP."""
import ast
import json
import importlib.util
import sys
import os
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
for path in [root/'main.py', root/'config.example.py', root/'config.py', *(root/'scripts').glob('*.py')]:
    if path.exists():
        try:
            ast.parse(path.read_text(), filename=path.name)
        except SyntaxError:
            sys.exit('Python syntax error in ' + path.name + '; inspect locally (source redacted)')
spec = importlib.util.spec_from_file_location('capture', root/'scripts/capture_status.py')
capture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(capture)
configuration = bytes.fromhex('09021900010100803209040000010701020007058902400000')
assert capture.decode_configuration(configuration)[1]['endpoint'] == '0x89'
for length in range(len(configuration)):
    try:
        capture.decode_configuration(configuration[:length])
    except ValueError:
        continue
    raise AssertionError('Truncated configuration accepted')
with tempfile.TemporaryDirectory(prefix='portrait-tests-') as temp:
    binary = str(Path(temp) / 'test_diagnostic')
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=address,undefined', '-g', '-I'+str(root/'firmware/src'),
        str(root/'tests/test_diagnostic.c'), str(root/'firmware/src/diagnostic.c'), '-o', binary], check=True)
    data = json.loads(subprocess.check_output([binary], text=True))
    assert data['motion_enabled'] is False
    assert data['job']['command_bytes_sent'] == 0
    assert data['job']['plotting_complete'] is None
    assert len(bytes.fromhex(data['usb']['configuration_descriptor_hex'])) == 512
    usb_binary = str(Path(temp) / 'test_usb_diag')
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=address,undefined', '-g', '-DPORTRAIT_USB_HOST=1',
        '-I'+str(root/'tests/fakes'), '-I'+str(root/'firmware/src'),
        str(root/'tests/test_usb_diag.c'), str(root/'firmware/src/diagnostic.c'),
        str(root/'firmware/src/usb_diag.c'), '-o', usb_binary], check=True)
    subprocess.run([usb_binary], check=True)
print('PASS: Python syntax; C descriptor bounds and HTTP routes; JSON; ASan/UBSan')
