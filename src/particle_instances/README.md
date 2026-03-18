# particle_instances

This directory represents the **Data-Oriented / Structure of Arrays (SoA) Particle** architecture approach.

Instead of combining GPU instance data and CPU simulation data in a single unified pool (like `src/particles`),
this approach splits them into **fully separate arrays** aligned to their access patterns:
- One array exclusively for GPU instance properties (position, scale, color) — never touched by simulation loops.
- One array exclusively for CPU simulation state (velocity, life, maxLife).

## Concept Overview

```
[GPU Instance Buffer]   position[]  scale[]  color[]
[CPU Simulation State]  velocity[]  life[]   maxLife[]
```

By separating access streams, the CPU-side simulation loop achieves perfect cache utilization by
iterating only over `velocity` and `life` without loading GPU data. The GPU upload likewise copies
only a contiguous block of instance data without any `sizeof(SimData)` gaps in between.

This directory would host a full standalone implementation of this approach. See `docs/particle_comparison.md` for a deep technical comparison.
