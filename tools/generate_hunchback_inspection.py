#!/usr/bin/env python3
"""Author-owned HBK-4G inspection art. No extracted game pixels as input."""
import argparse
import math
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/hunchback'
PALETTE = [
    (0, 0, 0), (0, 0, 0), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (85, 85, 255), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (0, 0, 170), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
]
# Half-open patch rectangles: retain labels, status pips, heat gauge and frame verbatim.
REGIONS = [(120, 0, 232, 24), (120, 24, 264, 144), (120, 144, 252, 192), (264, 96, 320, 170)]

# Location order: LA, LT, LL, H, CT, RA, RT, RL, RLT, RCT, RRT.
# Arm gauges shift slightly outward to follow the opened-arm stance; torso/head
# gauges sit below the Hunchback's high AC/20 housing, tucked head, and small laser.
GAUGES = [
    (251, 85), (208, 73), (224, 151), (192, 44), (192, 76),
    (132, 85), (176, 73), (160, 151), (272, 139), (288, 145), (304, 139),
]


def lerp(y, y0, x0, y1, x1):
    if y1 == y0:
        return float(x0)
    t = max(0.0, min(1.0, (y - y0) / (y1 - y0)))
    return x0 + t * (x1 - x0)


def profile_val(y, pts):
    for i in range(len(pts) - 1):
        y0, x0 = pts[i]
        y1, x1 = pts[i + 1]
        if y0 <= y <= y1:
            return int(round(lerp(y, y0, x0, y1, x1)))
    return pts[0][1] if y < pts[0][0] else pts[-1][1]


def artwork():
    im = Image.new('P', (320, 200), 0)
    im.putpalette([c for rgb in PALETTE for c in rgb])
    px = im.load()

    def in_regions(x, y):
        return any(x0 <= x < x1 and y0 <= y < y1 for x0, y0, x1, y1 in REGIONS)

    def put(x, y, c):
        if in_regions(x, y):
            px[x, y] = c

    # 1. Background 8x8 dark-blue grid (index 9 maps to EGA dark blue at runtime).
    for x0, y0, x1, y1 in REGIONS:
        for y in range(y0, y1):
            for x in range(x0, x1):
                px[x, y] = 9 if (x % 8 == 7 or y % 8 == 0) else 0

    def shade_span(y, x0, x1, chamfer=3, split=0.58,
                   left_aa=False, right_rim=True, top_lip=False, bot_seam=False):
        """Render a clean planar EGA metallic scanline without noisy checker stippling."""
        if x1 < x0:
            return
        w = x1 - x0 + 1
        if top_lip:
            for x in range(x0, x1 + 1):
                rel = (x - x0) / max(1, w - 1)
                if x == x0 and left_aa:
                    put(x, y, 8)
                elif rel < 0.65:
                    put(x, y, 15)
                elif rel < 0.88:
                    put(x, y, 7)
                else:
                    put(x, y, 8)
            return
        if bot_seam:
            for x in range(x0, x1 + 1):
                put(x, y, 8 if (x == x0 or x == x1) else 0)
            return

        mid_x = x0 + int(round(w * split))
        for x in range(x0, x1 + 1):
            dx = x - x0
            rx = x1 - x
            if dx == 0:
                put(x, y, 8 if left_aa else 15)
            elif dx == 1 and left_aa:
                put(x, y, 15)
            elif chamfer > 0 and dx == chamfer + (1 if left_aa else 0) and w >= chamfer + 6:
                put(x, y, 15)  # Clean vertical/angled bevel ridge
            elif right_rim and rx == 0:
                put(x, y, 8)
            elif right_rim and rx == 1 and w >= 8:
                put(x, y, 8 if w >= 14 else 0)
            elif right_rim and rx == 2 and w >= 14:
                put(x, y, 0)
            elif x < mid_x:
                put(x, y, 7)
            else:
                put(x, y, 8)

    def sculpt_plate(left_pts, right_pts, chamfer=3, split=0.58,
                     right_rim=True, top_bevel=True, bot_seam=True):
        y0 = max(left_pts[0][0], right_pts[0][0])
        y1 = min(left_pts[-1][0], right_pts[-1][0])
        prev_lx = None
        for y in range(y0, y1 + 1):
            lx = profile_val(y, left_pts)
            rx = profile_val(y, right_pts)
            left_aa = prev_lx is not None and lx != prev_lx
            prev_lx = lx
            if top_bevel and y == y0:
                shade_span(y, lx, rx, left_aa=True, top_lip=True)
            elif bot_seam and y == y1:
                shade_span(y, lx, rx, bot_seam=True)
            else:
                shade_span(y, lx, rx, chamfer=chamfer, split=split,
                           left_aa=left_aa, right_rim=right_rim)

    def hline(x0, x1, y, c):
        for x in range(min(x0, x1), max(x0, x1) + 1):
            put(x, y, c)

    def vline(x, y0, y1, c):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            put(x, y, c)

    def dline(x0, y0, x1, y1, c):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(steps + 1):
            x = int(round(x0 + (x1 - x0) * i / steps))
            y = int(round(y0 + (y1 - y0) * i / steps))
            put(x, y, c)

    # =========================================================================
    # 1. LEGS: Sturdy middle-ground thighs (25px), articulated knees, flared
    # shins (35px) and wide split-toe HBK-4G boots with subtle A-stance
    # =========================================================================
    for side in (-1, 1):
        tcx = 165 if side < 0 else 223
        scx = 162 if side < 0 else 226

        # Upper thigh / hip block (y=89..99, 25px wide)
        sculpt_plate(
            [(89, tcx - 11), (93, tcx - 13), (99, tcx - 13)],
            [(89, tcx + 11), (93, tcx + 12), (99, tcx + 12)],
            chamfer=3, split=0.60 if side < 0 else 0.52,
        )
        hline(tcx - 10, tcx + 10, 94, 0)
        hline(tcx - 10, tcx + 5, 95, 15)

        # Sturdy angular thigh armor plate (y=99..122, 25px -> 23px wide)
        sculpt_plate(
            [(99, tcx - 13), (112, scx - 12), (122, scx - 11)],
            [(99, tcx + 12), (112, scx + 12), (122, scx + 11)],
            chamfer=4, split=0.60 if side < 0 else 0.52,
        )
        vline(scx + 6, 100, 120, 0)
        vline(scx + 7, 100, 120, 7)

        # Articulated knee joint + side trunnion caps (y=120..129, 26px wide)
        for y in range(120, 130):
            for x in range(scx - 13, scx + 14):
                put(x, y, 0 if y in (122, 125, 128) else 8)
        sculpt_plate(
            [(121, scx - 14), (125, scx - 15), (129, scx - 13)],
            [(121, scx - 9), (125, scx - 9), (129, scx - 10)],
            chamfer=1, split=0.65,
        )
        sculpt_plate(
            [(121, scx + 9), (125, scx + 9), (129, scx + 10)],
            [(121, scx + 14), (125, scx + 15), (129, scx + 13)],
            chamfer=1, split=0.40,
        )
        # Prominent hexagonal kneecap block above the leg gauge (y=117..125, 16px wide)
        sculpt_plate(
            [(117, scx - 6), (120, scx - 8), (125, scx - 7)],
            [(117, scx + 6), (120, scx + 8), (125, scx + 7)],
            chamfer=2, split=0.62,
        )

        # Sturdy flared lower leg / shin armor (y=126..172, 35px wide at calf flare)
        sculpt_plate(
            [(126, scx - 13), (137, scx - 17), (153, scx - 18), (164, scx - 15), (172, scx - 12)],
            [(126, scx + 13), (137, scx + 16), (153, scx + 17), (164, scx + 15), (172, scx + 12)],
            chamfer=4, split=0.58 if side < 0 else 0.50,
        )
        dline(scx - 12, 128, scx - 14, 148, 15)
        dline(scx - 14, 148, scx - 11, 170, 15)
        dline(scx + 10, 128, scx + 13, 148, 0)
        dline(scx + 13, 148, scx + 10, 170, 0)
        # Central lower shin armor plate below the leg gauge (y=154..171)
        vline(scx - 7, 154, 171, 0)
        vline(scx + 7, 154, 171, 0)
        sculpt_plate(
            [(154, scx - 6), (171, scx - 6)],
            [(154, scx + 6), (171, scx + 6)],
            chamfer=2, split=0.62,
        )
        for y_vent in (159, 163):
            dline(scx + 9, y_vent, scx + 14, y_vent - 1, 0)
            dline(scx - 15, y_vent - 1, scx - 10, y_vent, 0)

        # Recessed ankle actuator collar (y=172..174, 24px wide)
        for y in range(172, 175):
            for x in range(scx - 12, scx + 13):
                put(x, y, 0 if y in (172, 174) else (15 if x < scx else 7))

        # Sturdy HBK-4G split-toe boot (y=175..189, 35px wide at sole)
        sculpt_plate(
            [(175, scx - 13), (180, scx - 16), (186, scx - 18), (189, scx - 17)],
            [(175, scx + 13), (180, scx + 16), (186, scx + 18), (189, scx + 17)],
            chamfer=3, split=0.58,
        )
        hline(scx - 13, scx + 13, 179, 0)
        hline(scx - 13, scx - 2, 180, 15)
        hline(scx + 2, scx + 11, 180, 15)
        dline(scx - 10, 180, scx - 13, 188, 0)
        dline(scx - 9, 180, scx - 12, 188, 15)
        dline(scx + 10, 180, scx + 13, 188, 0)
        for y in range(180, 190):
            put(scx - 1, y, 0)
            put(scx, y, 0)
            put(scx + 1, y, 15 if y < 188 else 7)
        hline(scx - 9, scx - 2, 185, 0)
        hline(scx + 2, scx + 9, 185, 0)
        hline(scx - 16, scx - 2, 189, 7)
        hline(scx + 2, scx + 16, 189, 7)

    # =========================================================================
    # 2. COMPACT PELVIS, HIP JOINTS & SLIMMED WAIST BELT (y=79..104)
    # =========================================================================
    for hx0, hx1 in ((173, 182), (206, 215)):
        sculpt_plate(
            [(87, hx0), (92, hx0), (97, hx0)],
            [(87, hx1), (92, hx1), (97, hx1)],
            chamfer=1, split=0.5,
        )
        vline((hx0 + hx1) // 2, 88, 96, 0)

    sculpt_plate(
        [(85, 181), (94, 182), (101, 185)],
        [(85, 207), (94, 206), (101, 203)],
        chamfer=2, split=0.60,
    )
    for py in (90, 94):
        hline(186, 202, py, 0)
        hline(186, 199, py + 1, 15)
    hline(185, 203, 101, 0)
    hline(185, 201, 102, 15)
    for x in range(185, 204):
        put(x, 103, 7 if x in (185, 186, 194, 202, 203) else 0)
    hline(185, 203, 104, 8)

    sculpt_plate(
        [(79, 170), (82, 171), (85, 173)],
        [(79, 218), (82, 217), (85, 215)],
        chamfer=3, split=0.58,
    )
    for gx_seam in (179, 187, 201, 209):
        vline(gx_seam, 80, 84, 0)
        vline(gx_seam + 1, 80, 84, 15 if gx_seam <= 187 else 7)

    # =========================================================================
    # 3. SLIMMER TAPERED TORSO WITH DOMINANT OVERHANGING AC/20 HUNCH (y=6..79)
    # =========================================================================
    sculpt_plate(
        [(24, 163), (44, 164), (58, 167), (79, 170)],
        [(24, 230), (38, 228), (58, 222), (79, 218)],
        chamfer=3, split=0.56,
    )

    sculpt_plate(
        [(24, 203), (36, 203), (46, 204)],
        [(24, 230), (36, 229), (46, 224)],
        chamfer=2, split=0.56,
    )
    dline(204, 39, 228, 30, 0)
    dline(204, 40, 228, 31, 15)

    sculpt_plate(
        [(46, 186), (79, 187)],
        [(46, 202), (79, 201)],
        chamfer=2, split=0.60,
    )
    vline(186, 46, 78, 0)
    vline(187, 46, 78, 15)
    vline(202, 46, 78, 0)
    hline(187, 201, 48, 0)
    hline(187, 199, 49, 15)
    dline(166, 58, 172, 62, 0)
    dline(166, 59, 172, 63, 15)
    dline(217, 62, 222, 58, 0)
    dline(217, 63, 222, 59, 7)

    # =========================================================================
    # 4. DOMINANT RIGHT-TORSO AC/20 HUNCH (Viewer Left, x=155..185, y=6..45)
    # =========================================================================
    sculpt_plate(
        [(6, 160), (10, 156), (14, 155)],
        [(6, 180), (10, 184), (14, 185)],
        chamfer=3, split=0.64,
    )
    hline(160, 180, 8, 0)
    hline(160, 176, 9, 15)
    hline(158, 182, 11, 0)
    hline(158, 178, 12, 15)

    sculpt_plate(
        [(14, 155), (26, 155), (37, 156)],
        [(14, 185), (26, 185), (37, 185)],
        chamfer=3, split=0.62,
    )
    hline(156, 184, 25, 0)
    hline(156, 182, 26, 15)

    mcx, mcy = 170, 25
    for y in range(mcy - 9, mcy + 10):
        for x in range(mcx - 9, mcx + 10):
            dx = x - mcx
            dy = y - mcy
            r = math.hypot(dx, dy)
            lit = (dx + dy) <= 0
            very_lit = (dx + dy) <= -3
            if r <= 3.3:
                if 2.2 <= r <= 3.2 and dx >= 1 and dy >= 1:
                    put(x, y, 8)
                else:
                    put(x, y, 0)
            elif r <= 4.4:
                put(x, y, 0 if lit else (15 if (dx >= 1 and dy >= 1) else 7))
            elif r <= 7.0:
                if very_lit and r >= 5.2:
                    put(x, y, 15)
                elif lit:
                    put(x, y, 7)
                else:
                    put(x, y, 8)
            elif r <= 8.3:
                put(x, y, 0 if (dx <= 2 or dy <= 2) else 15)

    sculpt_plate(
        [(37, 158), (41, 159), (45, 162)],
        [(37, 183), (41, 183), (45, 181)],
        chamfer=2, split=0.60,
    )
    hline(162, 179, 40, 0)
    hline(162, 176, 41, 15)
    hline(163, 178, 43, 0)

    # =========================================================================
    # 5. COMPACT TUCKED HEAD & COCKPIT (x=186..202, y=14..28)
    # =========================================================================
    sculpt_plate(
        [(14, 189), (17, 187), (23, 186), (28, 187)],
        [(14, 199), (17, 201), (23, 202), (28, 201)],
        chamfer=2, split=0.58,
    )
    hline(187, 201, 18, 0)
    hline(187, 198, 19, 15)
    for y in range(20, 25):
        for x in range(189, 200):
            if y == 20 or x == 189 or x == 199:
                put(x, y, 0)
            elif y == 21 and x in (191, 192, 193):
                put(x, y, 15 if x <= 192 else 7)
            elif y == 24:
                put(x, y, 8)
            else:
                put(x, y, 0 if x < 196 else 8)
    hline(188, 200, 25, 15)
    hline(191, 197, 27, 0)

    # =========================================================================
    # 6. LEFT-TORSO SMALL LASER POD, SHOULDER HOOD & ANTENNA (x=203..229, y=3..24)
    # =========================================================================
    sculpt_plate(
        [(14, 214), (18, 214), (24, 214)],
        [(14, 225), (18, 228), (24, 229)],
        chamfer=2, split=0.52,
    )
    hline(216, 227, 19, 0)
    hline(216, 224, 20, 15)

    vline(220, 3, 14, 15)
    vline(221, 7, 14, 8)
    put(220, 4, 7)

    sculpt_plate(
        [(11, 204), (14, 203), (24, 203)],
        [(11, 212), (14, 213), (24, 213)],
        chamfer=2, split=0.58,
    )
    for y in range(15, 20):
        for x in range(206, 211):
            put(x, y, 0)
    put(207, 16, 15)
    put(209, 18, 8)

    # =========================================================================
    # 7. NATURALLY OPENED ARMS (/  \ stance from shoulder -> elbow -> forearm -> fist)
    # Both upper arms AND forearms open moderately outward away from the torso,
    # with a relaxed elbow bend so they never look like vertical hanging rods!
    # =========================================================================
    # Left-side arm (Right Arm of mech):
    # Shoulder pauldron at upper torso (y=24..39, center 149 -> 146)
    sculpt_plate(
        [(24, 146), (28, 143), (35, 141), (39, 142)],
        [(24, 155), (28, 156), (35, 153), (39, 150)],
        chamfer=2, split=0.60,
    )
    dline(144, 29, 152, 34, 0)
    dline(144, 30, 152, 35, 15)
    # Upper arm angling outward from shoulder (center 146 at y=38 -> center 138 at y=53)
    sculpt_plate(
        [(38, 141), (46, 136), (53, 132)],
        [(38, 151), (46, 146), (53, 143)],
        chamfer=2, split=0.58,
    )
    dline(136, 45, 145, 48, 0)
    dline(135, 46, 144, 49, 15)
    # Visible relaxed elbow bend & outer elbow guard (y=52..59, center 137)
    for y in range(52, 59):
        for x in range(130, 144):
            put(x, y, 0 if y in (52, 58) else 8)
    sculpt_plate(
        [(53, 131), (56, 130), (58, 131)],
        [(53, 141), (56, 142), (58, 141)],
        chamfer=2, split=0.60,
    )
    # Forearm gauntlet continuing outward stance from elbow (center 136 at y=57 -> 131 at y=91)
    # Encloses the RA gauge frame (x=129..140, y=60..87)
    sculpt_plate(
        [(57, 128), (64, 125), (78, 123), (88, 123), (92, 124)],
        [(57, 144), (64, 144), (78, 143), (88, 141), (92, 138)],
        chamfer=3, split=0.60,
    )
    dline(127, 60, 125, 89, 15)
    hline(124, 138, 90, 0)
    hline(124, 135, 91, 15)
    # Clenched fist continuing the opened forearm line (y=92..105, center 130, x=123..137)
    sculpt_plate(
        [(92, 124), (96, 123), (102, 123), (105, 125)],
        [(92, 138), (96, 138), (102, 137), (105, 134)],
        chamfer=2, split=0.58,
    )
    vline(133, 95, 103, 0)
    vline(134, 95, 102, 15)
    for fy in (96, 100):
        hline(124, 132, fy, 0)
        hline(125, 131, fy + 1, 15)

    # Right-side arm (Left Arm of mech):
    # Shoulder pauldron at upper torso (y=24..39, center 237 -> 240)
    sculpt_plate(
        [(24, 231), (28, 230), (35, 233), (39, 236)],
        [(24, 240), (28, 243), (35, 245), (39, 244)],
        chamfer=2, split=0.50,
    )
    dline(234, 34, 242, 29, 0)
    dline(234, 35, 242, 30, 7)
    # Upper arm angling outward from shoulder (center 240 at y=38 -> center 248 at y=53)
    sculpt_plate(
        [(38, 235), (46, 240), (53, 243)],
        [(38, 245), (46, 250), (53, 254)],
        chamfer=2, split=0.50,
    )
    dline(241, 48, 250, 45, 0)
    dline(242, 49, 251, 46, 15)
    # Visible relaxed elbow bend & outer elbow guard (y=52..59, center 249)
    for y in range(52, 59):
        for x in range(242, 256):
            put(x, y, 0 if y in (52, 58) else 8)
    sculpt_plate(
        [(53, 245), (56, 244), (58, 245)],
        [(53, 255), (56, 256), (58, 255)],
        chamfer=2, split=0.52,
    )
    # Forearm gauntlet continuing outward stance from elbow (center 250 at y=57 -> 255 at y=91)
    # Encloses the LA gauge frame (x=248..259, y=60..87)
    sculpt_plate(
        [(57, 243), (64, 243), (78, 244), (88, 246), (92, 249)],
        [(57, 259), (64, 262), (78, 263), (88, 263), (92, 262)],
        chamfer=3, split=0.50,
    )
    dline(260, 60, 261, 89, 0)
    hline(249, 262, 90, 0)
    hline(249, 259, 91, 15)
    # Clenched fist continuing the opened forearm line (y=92..105, center 256, x=249..263)
    sculpt_plate(
        [(92, 249), (96, 249), (102, 250), (105, 253)],
        [(92, 262), (96, 263), (102, 263), (105, 261)],
        chamfer=2, split=0.50,
    )
    vline(253, 95, 103, 0)
    vline(254, 95, 102, 15)
    for fy in (96, 100):
        hline(254, 262, fy, 0)
        hline(255, 261, fy + 1, 7)

    # =========================================================================
    # 8. REAR TORSO INSET (x=264..319, y=96..169)
    # Clean, slimmer V-tapered back shell without stipple or louver clutter
    # =========================================================================
    vline(273, 96, 105, 15)
    vline(274, 99, 105, 8)
    sculpt_plate([(102, 276), (106, 275)], [(102, 281), (106, 282)], chamfer=1, split=0.55)

    sculpt_plate(
        [(105, 270), (110, 266), (138, 266), (152, 269), (162, 273)],
        [(105, 316), (110, 317), (138, 317), (152, 314), (162, 310)],
        chamfer=3, split=0.56,
    )

    sculpt_plate(
        [(96, 300), (100, 297), (113, 297)],
        [(96, 313), (100, 316), (113, 317)],
        chamfer=2, split=0.58,
    )
    for vy in (100, 104, 108):
        hline(301, 313, vy, 0)
        hline(301, 310, vy + 1, 15)

    sculpt_plate(
        [(103, 286), (106, 284), (114, 284)],
        [(103, 293), (106, 295), (114, 295)],
        chamfer=2, split=0.58,
    )
    hline(286, 293, 108, 0)
    hline(284, 295, 114, 0)

    sculpt_plate(
        [(115, 283), (146, 283), (155, 281), (166, 279)],
        [(115, 296), (146, 296), (155, 298), (166, 300)],
        chamfer=2, split=0.58,
    )
    vline(282, 112, 148, 0)
    vline(296, 112, 148, 0)
    for vy in (150, 154, 158):
        hline(271, 278, vy, 0)
        hline(271, 276, vy + 1, 15)
        hline(301, 308, vy, 0)
        hline(301, 306, vy + 1, 15)
    for vy in (152, 157, 162):
        hline(284, 295, vy, 0)
        hline(284, 292, vy + 1, 15)

    # =========================================================================
    # 9. AUTHENTIC 12-PIXEL EGA GAUGE FRAMES (yellow 14 / light-red 12 / black 0)
    # =========================================================================
    for gx, gy in GAUGES:
        well_h = 14 if (gx, gy) == (192, 44) else 24
        top = gy - well_h + 1
        vline(gx - 4, top - 2, gy + 2, 0)
        hline(gx - 4, gx + 8, top - 3, 0)
        for dx, c in enumerate((14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14)):
            put(gx - 3 + dx, top - 2, c)
        for dx, c in enumerate((14, 12, 12, 12, 12, 12, 12, 12, 12, 12, 14, 12)):
            put(gx - 3 + dx, top - 1, c)
        for y in range(top, gy + 1):
            for dx, c in enumerate((14, 12, 0, 0, 0, 0, 0, 0, 0, 0, 14, 12)):
                put(gx - 3 + dx, y, c)
        for dx, c in enumerate((14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 12)):
            put(gx - 3 + dx, gy + 1, c)
        for dx, c in enumerate((14, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12)):
            put(gx - 3 + dx, gy + 2, c)

    return im


def outputs():
    im = artwork()
    data = []
    for x0, y0, x1, y1 in REGIONS:
        for y in range(y0, y1):
            for x in range(x0, x1, 2):
                data.append((im.getpixel((x, y)) << 4) | im.getpixel((x + 1, y)))
    lines = [
        '/* Generated by tools/generate_hunchback_inspection.py. Do not edit. */',
        'static const uint8_t hunchbackInspectionPixels[] = {',
    ]
    lines += ['    ' + ','.join(f'0x{b:02x}' for b in data[i:i + 24]) + ',' for i in range(0, len(data), 24)]
    lines += ['};', 'static const uint16_t hunchbackInspectionRegions[][4] = {']
    lines += ['    {' + ','.join(map(str, r)) + '},' for r in REGIONS]
    lines += ['};', 'static const uint16_t hunchbackInspectionGaugeX[] = {']
    lines += ['    ' + ','.join(str(x) for x, _ in GAUGES) + ',']
    lines += ['};', 'static const uint16_t hunchbackInspectionGaugeBottomY[] = {']
    lines += ['    ' + ','.join(str(y) for _, y in GAUGES) + ',']
    lines += ['};', '']
    return im, '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    im, source = outputs()
    inc = ROOT / 'src/Original/BTECH_HUNCHBACK_INSPECTION_ART.inc'
    images = {
        OUT / 'inspection.png': im,
        OUT / 'inspection-preview.png': im.resize((960, 600), Image.Resampling.NEAREST),
    }
    if args.check:
        assert inc.read_text() == source, f'{inc} is stale'
        for path, expected in images.items():
            with Image.open(path) as actual:
                assert actual.mode == expected.mode and actual.size == expected.size
                assert actual.tobytes() == expected.tobytes(), f'{path} is stale'
                assert actual.getpalette() == expected.getpalette()
    else:
        inc.write_text(source)
        for path, image in images.items():
            image.save(path)


if __name__ == '__main__':
    main()
