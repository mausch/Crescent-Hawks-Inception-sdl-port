#!/usr/bin/env python3
"""Author and encode the HBK-4G's complete native 20-frame body-sprite bank.

Requires Python + Pillow (nix develop). No original game assets are read.
Run from any directory; --check verifies checked-in native data and PNG pixels.
Source art is deliberately placed pixels and articulated parts, not a 3D render.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/hunchback'
HEADER = ROOT / 'src/Original/BTECH_HUNCHBACK_ART.inc'
WIDTH = HEIGHT = 24
FRAME_BYTES = 4 + HEIGHT * 3 * 4

COLORS = {
    '.': (0, 0, 0, 0), 'B': (20, 26, 24, 255), 'J': (44, 53, 49, 255),
    'D': (65, 83, 71, 255), 'S': (103, 123, 103, 255),
    'H': (160, 177, 147, 255), 'T': (202, 212, 180, 255),
    'C': (234, 193, 104, 255),
}
# Gameplay's RGBI palette. Index 0 is transparent to the sprite blitter;
# index 1 is also black, and therefore supplies *opaque* outlines and bores.
EGA = [
    (0, 0, 0), (0, 0, 0), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (85, 85, 255), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (0, 0, 170), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
]
BAYER = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))
FRONT_HALF = [
    '', '..........BB', '..........BH', '..........BC', '..........BJ',
    '.......BTHHH', '......BHSBHH', '......BSDHSD', '......BHJBHS',
    '......BDJBSD', '......BJBHSS', '.....BHDBDSJ', '....BHSDBSHH',
    '....BHSDBHSS', '.....BJBBHHS', '......B.BHSB', '.......BHSD.',
    '.......BTHB.', '.......BHSB.', '.......BJDB.', '......BHSSB.',
    '.......BJD..', '......BHHHB.', '......BSBSB.',
]
BACK_HALF = [
    '', '..........BB', '..........BH', '..........BD', '..........BJ',
    '.......BTHHH', '......BHSBJD', '......BSDHDJ', '......BHJBJD',
    '......BDJBDJ', '......BJBHSS', '.....BHDBDJS', '....BHSDBSDD',
    '....BHSDBHDD', '.....BJBBHHS', '......B.BHSB', '.......BHSD.',
    '.......BHDB.', '.......BSSB.', '.......BJDB.', '......BHSSB.',
    '.......BJD..', '......BHSSB.', '......BBBBB.',
]
# Seven pixels across, six high: larger than the four-pixel-wide head.
FRONT_CANNON = ('.BBBBB.', 'BTHHHSB', 'BHJJJDB', 'BHJJJDB', 'BHSSSDB', '.BDSDB.')
BACK_CANNON = ('.BBBBB.', 'BTHHHSB', 'BHDJDDB', 'BHDJDDB', 'BHSSSDB', '.BDSDB.')

# Distinct side views: east exposes the right-side cannon, west puts it
# behind the left shoulder/arm. These are intentionally NOT mirrored images.
EAST = [
    '........BBBBBB',
    '.......BTHHSSDB',
    '.......BHJJJDSBBBBBB',
    '.......BHJJJDSHHSSJB',
    '.......BHSSSDBBBBBB',
    '........BSSDBBHCB',
    '.......BBHHSHHSSB',
    '......BHDSBTHSSDB',
    '......BHSJBHSSDB',
    '......BSDJBHSDDB',
    '.......BJBBJDBB',
    '.......BSDBHHSB',
    '.......BJBBTHSDB',
    '........BDSHSSDJB',
    '........BHHBSJBB',
    '........BHSSBB',
    '........BHDBHSSB',
    '........BHSBTHSB',
    '........BSDJBSSB',
    '........BJDBBJDB',
    '.......BHSSBBHSB',
    '........BJBBBJDB',
    '........BHHHBBHHHSB',
    '........BBBBBBBBBBB',
]
WEST = [
    '............BBBBBB',
    '...........BTHHSSDB',
    '.....BBBBBBBHDJDDB',
    '.....BJSSHHBHDJDDB',
    '......BBBBBBHSSSDB',
    '......BCHBB.BDSDB',
    '......BSSHHSHHBBB',
    '......BHSSHTBSDHB',
    '.......BHSSBJSDB',
    '.......BSDHBJDSB',
    '........BBDJBBJB',
    '.......BHSHBDSB',
    '......BDSHTBBJB',
    '......BJDSHSDB',
    '.......BBJSBHHB',
    '.........BBSSHB',
    '.......BSSHBDSB',
    '.......BSHTBSHB',
    '.......BSSBJDSB',
    '.......BDJBBJDB',
    '.......BSHBBSSHB',
    '.......BDJBBBJB',
    '....BSHHHBBHHHB',
    '....BBBBBBBBBBB',
]


def indexed(rows, native):
    """Quantize authored pixels before articulation, so stippling moves with parts."""
    assert len(rows) == HEIGHT and all(len(row) <= WIDTH for row in rows)
    im = Image.new('P', (WIDTH, HEIGHT))
    palette = EGA if native else [COLORS[c][:3] for c in COLORS]
    im.putpalette([v for rgb in palette for v in rgb] + [0] * (768 - len(palette) * 3))
    symbols = list(COLORS)
    for y, row in enumerate(rows):
        for x, symbol in enumerate(row):
            if not native:
                color = symbols.index(symbol)
            elif symbol == 'S':
                # Sparse green/grey stipple approximates the muted concept paint
                # without changing the shared palette or recolouring terrain.
                threshold = BAYER[y % 4][x % 4]
                color = 2 if threshold < 2 else 7 if threshold < 6 else 8
            else:
                color = {'.': 0, 'B': 1, 'J': 1, 'D': 8, 'H': 7, 'T': 15, 'C': 14}[symbol]
            im.putpixel((x, y), color)
    im.info['transparency'] = 0
    return im


def mirrored(half):
    assert all(len(row) <= 12 for row in half)
    return [row.ljust(12, '.') + row.ljust(12, '.')[::-1] for row in half]


def front_or_back(back, native):
    rows = mirrored(BACK_HALF if back else FRONT_HALF)
    for y, row in enumerate(BACK_CANNON if back else FRONT_CANNON):
        x = 13 if back else 4  # cannon shares the head's outside border; no gap
        rows[y] = rows[y][:x] + row + rows[y][x + len(row):]
    return indexed(rows, native)


def blank_like(im):
    out = Image.new('P', im.size)
    out.putpalette(im.getpalette())
    out.info['transparency'] = 0
    return out


def paste(out, part, xy):
    mask = part.point(lambda value: 255 if value else 0, 'L')
    out.paste(part, xy, mask)


def walk(im, phase, side=False):
    """Four-frame gait: independently swing/lift legs and counter-swing arms.

    Hip anchors stay under the torso. No resampling, palette drift, whole-image
    mirroring, or random per-frame pixels. Neutral poses are kept separately.
    """
    out = blank_like(im)
    bob = phase & 1
    if side:
        # Part boundaries follow the two separately authored profile views.
        legs = [im.crop((0, 16, 12, 24)), im.crop((12, 16, 24, 24))]
        offsets = ((-2, 2), (0, 0), (2, -2), (0, 0))[phase]
        for i, part in enumerate(legs):
            lift = 1 if (phase == 1 and i == 0) or (phase == 3 and i == 1) else 0
            paste(out, part, (i * 12 + offsets[i], 16 - lift))
        paste(out, im.crop((0, 0, 24, 16)), (0, bob))
    else:
        for i, bounds in enumerate(((0, 16, 12, 24), (12, 16, 24, 24))):
            lift = ((0, 2), (1, 0), (2, 0), (0, 1))[phase][i]
            inward = (1 if i == 0 else -1) if bob else 0
            paste(out, im.crop(bounds), (bounds[0] + inward, 16 - lift))
        torso = im.crop((0, 0, 24, 16))
        # Move forearms about the elbows rather than stretching rectangles.
        for i, bounds in enumerate(((2, 10, 8, 16), (16, 10, 22, 16))):
            arm = im.crop(bounds)
            ImageDraw.Draw(torso).rectangle((bounds[0], bounds[1], bounds[2]-1, 15), fill=0)
            swing = ((-1, 0), (0, 1), (0, -1), (1, 0))[phase][i]
            paste(torso, arm, (bounds[0], bounds[1] + swing))
        paste(out, torso, (0, bob))
    return out


def firing(im, east):
    """Braced profile with cannon recoil; the bent forearm stays by the torso."""
    out = im.copy()
    # The top cannon recoils one pixel toward the rear, leaving the body fixed.
    top = im.crop((0, 0, 24, 5))
    ImageDraw.Draw(out).rectangle((0, 0, 23, 4), fill=0)
    paste(out, top, (-1 if east else 1, 0))
    return out


def frames(native):
    back = front_or_back(True, native)
    front = front_or_back(False, native)
    east, west = indexed(EAST, native), indexed(WEST, native)
    result = [walk(im, phase, side) for im, side in
              ((back, False), (east, True), (front, False), (west, True)) for phase in range(4)]
    result += [front, back, firing(east, True), firing(west, False)]
    assert len(result) == 20
    return result


def rgba(im):
    out = im.convert('RGBA')
    if im.mode == 'P':
        out.putalpha(im.point(lambda value: 255 if value else 0, 'L'))
    return out


def encode(im):
    """Native four-byte header followed by row/cell/plane bytes, MSB-first."""
    data = bytearray((0, HEIGHT - 1, WIDTH // 8, 0))
    for y in range(HEIGHT):
        for cell in range(WIDTH // 8):
            for plane in range(4):
                byte = sum(((im.getpixel((cell * 8 + bit, y)) >> plane) & 1) << (7-bit)
                           for bit in range(8))
                data.append(byte)
    assert len(data) == FRAME_BYTES
    return bytes(data)


def decode(data):
    im = Image.new('P', (WIDTH, HEIGHT))
    im.putpalette([v for rgb in EGA for v in rgb] + [0] * (768 - 48))
    im.info['transparency'] = 0
    for y in range(HEIGHT):
        for x in range(WIDTH):
            p = 4 + (y * 3 + x // 8) * 4
            c = sum(((data[p+plane] >> (7-x % 8)) & 1) << plane for plane in range(4))
            im.putpixel((x, y), c)
    return im


def sheet(images):
    out = blank_like(images[0]).resize((24 * 4, 24 * 5))
    for i, im in enumerate(images):
        out.paste(im, (i % 4 * 24, i // 4 * 24))
    return out


def preview(images):
    out = Image.new('RGB', (832, 780), (30, 34, 43))
    d = ImageDraw.Draw(out)
    labels = ('BACK / NORTH WALK', 'RIGHT / EAST WALK', 'FRONT / SOUTH WALK',
              'LEFT / WEST WALK', 'FRONT STAND / BACK STAND / EAST FIRE / WEST FIRE')
    for row, label in enumerate(labels):
        d.text((20, row * 152 + 8), label, fill='white')
        for col in range(4):
            i = row * 4 + col
            im = rgba(images[i])
            x, y = 24 + col * 204, row * 152 + 28
            big = im.resize((120, 120), Image.Resampling.NEAREST)
            out.paste(big, (x, y), big)
            out.paste(im, (x + 144, y + 72), im)
            d.text((x + 144, y + 104), f'{i:02}', fill=(180, 190, 200))
    d.text((20, 766), 'Decoded native EGA sprites. Large: 5x nearest neighbour. Small: native 24x24.', fill='white')
    return out


def design_preview(concept, native):
    out = Image.new('RGB', (672, 400), (30, 34, 43))
    d = ImageDraw.Draw(out)
    for i, (label, sprite) in enumerate((('CURRENT DESIGN - AUTHORING COLOURS', concept),
                                        ('ACTUAL ENCODED EGA COLOURS', native))):
        x = 24 + i * 336
        d.text((x, 16), label, fill='white')
        im = rgba(sprite)
        big = im.resize((288, 288), Image.Resampling.NEAREST)
        out.paste(big, (x, 44), big)
        out.paste(im, (x + 132, 350), im)
    d.text((24, 382), 'Compact shoulders / fuller waist / cannon close to head. Native 24x24 below.', fill='white')
    return out


def animation(images):
    result = []
    for phase in range(4):
        out = Image.new('RGB', (512, 176), (30, 34, 43))
        d = ImageDraw.Draw(out)
        for view, label in enumerate(('BACK', 'RIGHT', 'FRONT', 'LEFT')):
            d.text((view * 128 + 20, 12), label, fill='white')
            im = rgba(images[view * 4 + phase]).resize((96, 96), Image.Resampling.NEAREST)
            out.paste(im, (view * 128 + 16, 44), im)
        result.append(out)
    return result


def manifest(images):
    names = [f'{view}-walk-{phase}' for view in ('north', 'east', 'south', 'west') for phase in range(4)]
    names += ['south-standing', 'north-standing', 'east-firing', 'west-firing']
    return json.dumps({
        'width': WIDTH, 'height': HEIGHT, 'columns': 4, 'rows': 5,
        'nativeFamily': 146, 'frameBytes': FRAME_BYTES,
        'format': '4-byte header; scanline, 8-pixel cell, B/G/R/I plane; MSB-first; index 0 transparent',
        'palette': EGA,
        'frames': [dict(frame=i, name=name, x=i % 4 * 24, y=i // 4 * 24,
                        sha256=hashlib.sha256(encode(images[i])).hexdigest()) for i, name in enumerate(names)],
    }, indent=2) + '\n'


def c_header(images):
    lines = ['/* Generated by tools/generate_hunchback_sprites.py. Do not edit.',
             ' * Original HBK-4G art: 20 mutable native EGA sprites; no original game data. */',
             'static uint8_t hunchbackSprites[HunchbackSpriteCount][HunchbackSpriteBytes] = {']
    for i, im in enumerate(images):
        data = encode(im)
        lines.append(f'    {{ /* frame {i} */')
        for start in range(0, len(data), 16):
            lines.append('        ' + ','.join(f'0x{v:02X}' for v in data[start:start+16]) + ',')
        lines.append('    },')
    return '\n'.join(lines + ['};', ''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--frames-dir', type=Path, help='Also export a numbered PNG sequence for LibreSprite')
    args = parser.parse_args()
    if args.check and args.frames_dir:
        parser.error('--frames-dir cannot be combined with --check')
    native, concept = frames(True), frames(False)
    assert len({im.tobytes() for im in native}) == 20, 'duplicate body frames'
    assert all(100 < sum(bool(p) for p in im.tobytes()) < 500 for im in native), 'empty or malformed body frame'
    decoded = [decode(encode(im)) for im in native]
    for im, check in zip(native, decoded):
        assert im.tobytes() == check.tobytes(), 'native plane roundtrip mismatch'
    expected_text = {HEADER: c_header(native), OUT / 'sprites.json': manifest(native)}
    expected_images = {OUT / 'sprites.png': sheet(decoded), OUT / 'concept.png': sheet(concept),
                       OUT / 'preview.png': preview(decoded), OUT / 'standing.png': decoded[16],
                       OUT / 'design-preview.png': design_preview(concept[16], decoded[16])}
    if args.check:
        for path, text in expected_text.items():
            assert path.read_text() == text, f'Stale generated data: {path}'
        for path, im in expected_images.items():
            with Image.open(path) as saved:
                assert saved.size == im.size and rgba(saved).tobytes() == rgba(im).tobytes(), f'Stale PNG: {path}'
        print('HBK-4G: all 20 frames, native encoding and generated assets verified.')
        return
    OUT.mkdir(parents=True, exist_ok=True)
    for path, text in expected_text.items():
        path.write_text(text)
    for path, im in expected_images.items():
        im.save(path)
    anim = animation(decoded)
    anim[0].save(OUT / 'walk.gif', save_all=True, append_images=anim[1:], duration=180, loop=0, disposal=2)
    if args.frames_dir:
        args.frames_dir.mkdir(parents=True, exist_ok=True)
        for i, im in enumerate(decoded):
            im.save(args.frames_dir / f'hunchback{i:03}.png')
    print(f'Wrote 20 HBK-4G sprites, previews and native C data to {OUT}.')


if __name__ == '__main__':
    main()
