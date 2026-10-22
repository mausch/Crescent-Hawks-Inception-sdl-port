# Original bugs and compatibility behavior

The supported DOS executable is the behavioral reference. A quirk demonstrated
by assembly or the original runtime is preserved by default, even when it is
undesirable by modern standards.

A change to original behavior is acceptable only when required for host safety,
interoperability or access to a modern platform boundary. Such a change must:

- be the smallest practical intervention;
- remain outside reconstructed game logic whenever possible;
- identify the original procedure/address and observed behavior;
- carry a regression test or reproducible acceptance case; and
- be documented as an SDL/host compatibility decision rather than presented as
  original behavior.

Convenience, stylistic preference and speculative design improvement are not
sufficient reasons to fix an original game bug in the preservation branch.

## Gameplay fixes

### Rick's lounge rendezvous and invisible NPC blockers (2026-10-20)

After Rick invites Jason to the lounge, loading a save could leave Rick
permanently waiting in the weapons shop instead. His invisible outdoor position
could also block Mechit-Lube's entrance, making another character circle outside
without ever entering. Waiting longer would not resolve either problem.

The fix restores Rick's pending lounge meeting when rebuilding the Citadel map
and prevents characters waiting inside buildings from blocking outdoor movement.
This deliberately corrects original DOS gameplay behavior; it is not an SDL or
host compatibility substitution.

`Load_Game` (`0800:32B3..35D2`) reads saved NPC records, then rebuilds maps via
`DOS_Load_Map_Files` (`0800:2DA8..320A`), which randomizes their routes and puts
their collision positions at their destinations before their initial wait ends.
The pending Rick flag survives. `Update_Roaming_Map_Npcs` (`0800:24C2..2866`)
resets Rick's nonzero delay to FF, then decrements it to FE indefinitely, without
checking his building. The renderer hides delayed NPCs, but occupancy
(`1631:16AB..1B43`) still sees their coordinates.

An affected local save had Rick's route 35 (weapons shop to Mechit-Lube), delay
FE and hold flag 01. Rebuilding the original Citadel map reproduced an invisible
Rick at Mechit-Lube's destination, blocking another NPC indefinitely. Removing
the invisible position allowed arrival two movement updates later.

The loader now restores pending Rick to TRAINING.BLD's lounge staging route 70
when loading the intact Citadel. Other routes and RNG consumption are unchanged.
Waiting NPCs have zero outdoor collision coordinates, both at initialization
and during updates; they materialize at their starting waypoint only when their
delay expires. The native hold and conversation handshake otherwise remain.
Tests: `test_mapfiles.c` (reload/repeated reload, cache exclusion, hidden initial
positions) and `test_walk_npcs.c` (held and ordinary waiting blockers, respawn).
