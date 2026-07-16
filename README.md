# DoDSimulation — A Unity DOTS-style ECS for Unreal Engine

A personal R&D project that brings a **Unity DOTS-style, archetype-based
Entity-Component-System (ECS)** to **Unreal Engine 4.26 (C++)**, which ships no
such framework out of the box.

## Intent

Unreal's default model is object-oriented — gameplay lives in Actors that each
tick independently, with data scattered across the heap. Unity's DOTS takes the
opposite, **data-oriented** approach: components are packed into contiguous
arrays and systems iterate over them linearly, which is far more cache-friendly
at scale.

The goal of this project was to **re-implement that archetype ECS from scratch**
inside Unreal and understand how it works under the hood — component storage,
archetype migration, and query-driven systems.

## What's in it

- **A hand-written ECS core** (`Source/DoDSimulation/ECS/`): a `Database` of
  entities, **archetypes** that store components in structure-of-arrays pools,
  `Query` for iterating matching entities (with include/exclude filters), and a
  simple system interface — all built with C++ variadic templates and move
  semantics.
- **An Unreal integration layer**: Actors register as ECS entities and sync
  their transforms to/from the ECS, so entities can be authored right in the
  editor.
- **A demo** (moving agents + proximity-sensing doors) implemented **both** the
  classic per-Actor `Tick()` way and the ECS way, so the two approaches can be
  toggled at runtime and compared.

## Build

Open `DoDSimulation.uproject` with **Unreal Engine 4.26** and compile the
`DoDSimulation` C++ module.
