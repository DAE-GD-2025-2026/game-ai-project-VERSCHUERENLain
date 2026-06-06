# Game AI Project

**Created by Lain Verschueren**

## Extra Assignment Info
- **Assignment:** Hierarchical Space Partitioning (QuadTree) in Flocking
- **Week/Topic:** Week 4 - Spatial Partitioning
- **Editor Startup Level:** `FlockingSpacialPartitioning` (Set in project settings / `DefaultEngine.ini`)

---

## Architecture

### Flocking

The flock uses Reynolds' classic boid model with three group behaviors plus individual behaviors:

- **Cohesion** — steer toward the center of nearby neighbors
- **Separation** — steer away from neighbors that are too close
- **Alignment (Velocity Match)** — match the average velocity of neighbors

These are blended together and combined with **Seek** and **Wander**. **Evade** runs as a higher-priority behavior so boids flee a predator before applying flocking.

The `Flock` class owns all agents and drives them manually: engine tick is disabled on flock agents (`SetActorTickEnabled(false)`), and `Flock::Tick` calls each agent's `Tick` in sequence. This keeps update order consistent and avoids double-ticking.

Each agent needs its neighbors within a `NeighborhoodRadius`. That neighbor query can be done in three ways: brute force (O(n²)), flat spatial partitioning (SP), or hierarchical spatial partitioning (HI).

### Spatial Partitioning (SP) — Flat Grid

`CellSpace` divides the world into a uniform grid of cells. Each `Cell` holds a `std::list` of agent pointers (the flock remains owner).

- **PositionToIndex** — maps a 2D position to a cell index. Agents outside the grid bounds are skipped (not clamped).
- **AddAgent** — adds an agent to the cell at its position.
- **UpdateAgentCell** — when an agent moves, removes it from the old cell and adds it to the new one. Requires storing previous positions per agent.
- **RegisterNeighbors** — builds an axis-aligned query rect around the agent (position ± radius), uses `DoRectsOverlap` to find overlapping cells, collects agents from those cells, then filters by actual distance.

The grid size is derived from `TrimWorld` so the partition matches the playable area.

### Hierarchical Spatial Partitioning (HI) — QuadTree

`QuadTree` recursively subdivides space. Each node has an axis-aligned bounds and either stores agents (leaf) or four children (NW, NE, SW, SE). Nodes subdivide when they exceed `MaxAgentsPerNode` and haven't hit `MaxDepth`.

- **Insert** — traverses from root to the correct leaf, subdividing if needed. Agents outside the root bounds are skipped.
- **Clear + Insert** — the tree is rebuilt every frame for moving agents (simplest approach).
- **RegisterNeighbors (QueryNode)** — recurses from root, prunes nodes that don't overlap the query rect, and at leaves collects agents within the query radius with a distance check.

Both SP and HI reduce neighbor checks from O(n) per agent to only agents in nearby cells/leaves, but the tree adds subdivision and traversal overhead.

---

## Performance Note

In-editor testing shows the spatial partitioning logic working as intended (correct neighbors, correct cell/leaf assignment, debug visualization). However, **there is no measurable FPS gain** when switching from brute force to SP or HI, even with hundreds of agents.

This is likely due to **Unreal Engine 5 editor overhead** — the editor adds profiling, hot-reload, debug drawing, and other costs that dominate frame time. The theoretical O(n²) → O(n) improvement from spatial partitioning may only show up in a packaged (shipping) build where the editor is not running.
