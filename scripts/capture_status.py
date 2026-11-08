#!/usr/bin/env python3
"""GET-only LAN diagnostic capture. No USB access, no command submission."""
import argparse
import json
from pathlib import Path
from urllib.parse import urlsplit
from urllib.request import urlopen


def decode_configuration(raw):
    if len(raw) < 9 or raw[:2] != b'\x09\x02' or int.from_bytes(raw[2:4], 'little') != len(raw):
        raise ValueError('Invalid configuration header/length')
    result = []
    offset = 0
    while offset < len(raw):
        if offset + 2 > len(raw):
            raise ValueError('Truncated descriptor header')
        size, kind = raw[offset:offset+2]
        if size < 2 or offset + size > len(raw):
            raise ValueError('Invalid descriptor bounds')
        d = raw[offset:offset+size]
        if kind == 4:
            if size < 9:
                raise ValueError('Short interface descriptor')
            result.append({'interface': d[2], 'alternate': d[3], 'endpoint_count': d[4],
                           'class': d[5], 'subclass': d[6], 'protocol': d[7]})
        elif kind == 5:
            if size < 7:
                raise ValueError('Short endpoint descriptor')
            result.append({'endpoint': hex(d[2]), 'direction': 'in' if d[2] & 128 else 'out',
                           'transfer_type': d[3] & 3, 'max_packet': int.from_bytes(d[4:6], 'little'),
                           'interval': d[6]})
        offset += size
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base_url', help='http://<Pico IP>')
    parser.add_argument('--output', type=Path, help='New JSON capture file (will not overwrite)')
    args = parser.parse_args()
    url = urlsplit(args.base_url)
    if url.scheme != 'http' or not url.hostname or url.username or url.password or url.path not in ('', '/') or url.query or url.fragment:
        parser.error('Use http://<Pico IP> with no credentials, path, query, or fragment')
    with urlopen(args.base_url.rstrip('/') + '/api/status', timeout=5) as response:
        raw = response.read(8193)
    if len(raw) > 8192:
        parser.error('Response exceeds diagnostic limit')
    data = json.loads(raw)
    if data.get('api_version') != 1:
        parser.error('This tool expects C firmware API v1 (not the stage 1 MicroPython API)')
    usb = data['usb']
    print('USB:', usb['state'], 'VID:PID %04x:%04x' % (usb['vid'], usb['pid']))
    config = bytes.fromhex(usb['configuration_descriptor_hex'])
    if config:
        print(json.dumps(decode_configuration(config), indent=2))
    if args.output:
        with args.output.open('x') as out:
            json.dump(data, out, indent=2)
            out.write('\n')
        print('Saved', args.output)
    else:
        print(json.dumps(data, indent=2))


if __name__ == '__main__':
    main()
