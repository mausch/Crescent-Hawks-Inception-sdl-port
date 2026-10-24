# HBK-4G Hunchback world sprites

Agent-authored pixel art for this port, iterated with the project owner.
No original game graphics, miniature renders or extracted assets are included.
BattleTech's setting/design rights remain with their respective owners; see
[NOTICE.md](../../NOTICE.md).

The current design has twelve-pixel-wide upper shoulders and a fuller
eight-pixel-wide waist, a four-pixel-wide centred head, a seven-pixel-wide
right-torso cannon housing tucked against the head, articulated arms,
and forward-facing split-toe boots. Each frame fits the original 24×24 canvas.
Front view: cannon on viewer **left**. Back view: cannon on viewer **right**.
The two profile views are separately authored, not mirrored Hunchbacks.

Visual reference supplied by the owner: [HBK-4G miniature](https://assets.myminifactory.com/object-images/5dd0de64af397/images/hbk-4g8.jpg).
The reference image is not distributed here.

## Files

- `sprites.png`: native indexed sprite sheet, 4 columns × 5 rows (96×120).
- `sprites.ase`: editable LibreSprite animation with all 20 frames.
- `sprites.json`: frame names, positions, palette and SHA-256 of each native frame.
- `standing.png`: native front-facing standing frame.
- `design-preview.png`: current standing design beside its actual EGA conversion.
- `concept.png`: the same frames in the muted green-grey authoring palette.
- `preview.png`: enlarged **decoded native** frames, with native-size thumbnails.
- `walk.gif`: preview of all four walk cycles, enlarged without smoothing.
- [`tools/generate_hunchback_sprites.py`](../../tools/generate_hunchback_sprites.py):
  editable pixel definitions, articulation, palette mapping, previews and encoder.

The shared EGA palette cannot reproduce the concept's exact RGB green-greys.
The native art uses grey armour, sparse green stippling, white bevels and a yellow
cockpit. Stippling is applied before articulation so it moves with the parts.
Opaque black uses index **1**; index **0** is reserved for transparency. No game
palette registers are changed, so terrain and other sprites are not recoloured.

## HBK-4G inspection paper doll

- `inspection.png`: native 320×200 indexed authoring canvas; only the documented
  patch rectangles are used at runtime (black outside them is not drawn).
- `inspection-preview.png`: nearest-neighbour 3× preview.
- `inspection.ase`: editable LibreSprite copy of the indexed canvas.
- [`tools/generate_hunchback_inspection.py`](../../tools/generate_hunchback_inspection.py):
  canonical pixel drawing, palette, patch bounds, previews and embedded encoder.

The front view has the AC/20 hump on viewer **left**; the rear inset has it on
viewer **right**. Original-style grey armour, white bevels, dark panel seams,
blue grid and yellow/red gauge frames use the existing inspection palette.
Owner-supplied [HBK-4G reference](https://www.loothoarder.com/wp-content/uploads/2026/06/imgopt_24203_1781040614_0_0.jpg)
was used for the chassis silhouette, not copied into the artwork or distributed.

The port patches the freshly decompressed `BTSTATS.CMP` image only when the live
record has the Hunchback name, for friendly and enemy inspections alike, and
positions the Hunchback's live protection gauges below the high AC/20 housing,
tucked head and small laser and along the opened forearms. It preserves the
original labels, heat display, status pips, live armour/structure scaling and
palette cycling. The original CMP cache and files are never modified; other
chassis retain their original illustration and gauge table.

Regenerate/check with:

```sh
nix develop --command python tools/generate_hunchback_inspection.py
nix develop --command python tools/generate_hunchback_inspection.py --check
nix shell nixpkgs#libresprite --command env SDL_VIDEODRIVER=dummy \
  libresprite --batch assets/hunchback/inspection.png --save-as assets/hunchback/inspection.ase
```

The generator is authoritative: transfer desired LibreSprite edits back to its
pixel definitions before regenerating. No original assets or image loader are
needed to regenerate or use this art. Tests check packed-pixel patch bounds,
unchanged labels/heat area, name selection, non-Hunchback fallback and both
friendly/enemy record mapping through the real statistics parent.

## Native frame contract

| Offsets | Body frames |
| --- | --- |
| 0–3 | north/back walking |
| 4–7 | east/right walking |
| 8–11 | south/front walking |
| 12–15 | west/left walking |
| 16 | south/front standing and firing |
| 17 | north/back standing and firing |
| 18 | east firing/recoil |
| 19 | west firing/recoil |

These match the original humanoid animation streams, including the poses reused
by kicks. The original eight movement headings use these four visual directions.
Shared hit effects, projectiles, wrecks and cinematic animations are unchanged;
this is the complete **world/combat body** sprite bank. The inspection screen
has a separate chassis-specific illustration, described above.

The embedded bank in `src/Original/BTECH_HUNCHBACK_ART.inc` contains twenty mutable
292-byte sprites: four-byte header `(0,23,3,0)`, then scanline → eight-pixel cell →
blue/green/red/intensity plane, MSB-first. Headers remain mutable for the original
compositors' temporary terrain-occlusion clipping.

Selection happens per friendly/enemy Mech record at draw time. The native family
remains `0x92`; names, family/frame bytes, save layout, animation timing and other
chassis are unchanged. No runtime PNG loader, original installation patch, extra
runtime files or Python dependency is needed.

## Regenerate and check

```sh
nix develop --command python tools/generate_hunchback_sprites.py
nix develop --command python tools/generate_hunchback_sprites.py --check
nix flake check
```

To refresh the editable animation as well, use a fresh sequence directory:

```sh
frames=$(mktemp -d)
nix develop --command python tools/generate_hunchback_sprites.py --frames-dir "$frames"
nix shell nixpkgs#libresprite --command env SDL_VIDEODRIVER=dummy \
  libresprite --batch "$frames/hunchback000.png" --save-as assets/hunchback/sprites.ase
rm -rf "$frames"
```

LibreSprite imports the numbered sequence as twenty frames. Its editor timeline
speed is not the game's animation timing. The checked-in ASE was re-exported
and compared pixel-for-pixel to `sprites.png`.

Asset-independent checks verify regenerated art and native encoding, all twenty
frame selections, rear/front cannon placement, friendly/enemy record mapping,
depth-sort identity, exploration formation compaction, original-sprite fallback,
opaque black, transparency, pixel compositing, and 0/8/16-row terrain clipping.
Original-data checks must use the integration target's temporary copy, as
specified in [AGENTS.md](../../AGENTS.md).
