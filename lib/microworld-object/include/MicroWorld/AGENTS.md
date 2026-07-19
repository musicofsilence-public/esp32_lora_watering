# MicroWorld Object Namespace

Inherits `../AGENTS.md`.

## Architecture

This directory extends the shared `MicroWorld` namespace only with Object
contracts. It may use Core and Memory contracts but must not duplicate them or
introduce Engine, Serialization, Net, Integration, platform, or port APIs.

## Concepts

All Object symbols share the project namespace while retaining their separate
physical package and one-way dependency boundary.

## Verification

Verify public symbols and includes against the declared inward dependency
boundary before release.
