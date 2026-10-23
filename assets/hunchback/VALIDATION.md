# Hunchback validation — 2026-10-23

## Checks

- `nix flake check`: passed for x86_64-linux (native runtime build/tests and
  reproducible sprite-art check); aarch64 was not run.
- All 150 asset-independent runtime tests passed, including all twenty Hunchback frames,
  friendly/enemy identity, depth sorting, formation compaction, transparency,
  opaque-black pixels, 0/8/16-row occlusion and header restoration.
- LibreSprite headless import/export: all twenty ASE frames reproduce the native
  PNG sheet pixel-for-pixel, including transparency.
- Chromium headless: native previews load through this Pi Web instance's actual
  file-read endpoint (not guessed filesystem links).
- Temporary scene fixture: actual original startup/new-game map, then the real
  exploration compositor drawing a Hunchback alongside the unchanged humanoid.
  The screenshot contains private game art and is not included in Git.
- CMake and Visual Studio source/include lists include the new sprite bank.

No original installation was used as a test working directory. Original-data
runs use `nix run .#integration-tests`, which copies the installation first.

## Original-data training timeout (also present before this change)

The initial complete integration run passed 167/169 tests. The ordinary training
mission and slow-operator training variant both reached their existing 40-second
limits. The ordinary training mission passed on retry (34.09 seconds).

The slow-operator training variant timed out again on a fresh temporary copy.
For comparison, the unchanged pre-art commit `2eab4d472d7d` was exported with
`git archive HEAD` into a separate source directory and its original integration
target was run on another fresh temporary asset copy. Its slow-operator training
variant also timed out at 40 seconds. No deadlines or training behavior were
changed to hide this result.

After the final proportion adjustments, all 17 non-training local original-data
scenarios passed on a fresh temporary copy. They can be checked independently with:

```sh
CHI_ORIGINAL_ASSET_DIRECTORY=/path/to/original-installation \
  nix run .#integration-tests -- -L local -E training
```

The shared wreck, impact, projectile, cinematic and statistics graphics remain
original. These checks do not certify a complete manual playthrough or full
presentation fidelity.
