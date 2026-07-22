# Project libraries

## Purpose

`lib/` holds project-private application libraries only — firmware code local
to this repository. The MicroWorld engine is not kept here; it is an external
dependency consumed from the sibling repository at
[`../MicroWorld`](https://github.com/musicofsilence-public/MicroWorld) through
the `symlink://` `lib_deps` declared in `platformio.ini`.

## Boundary rule

A library in `lib/` may depend on MicroWorld or other dependencies, but
application, hardware, and product-policy code owns the direction: a library
must never depend back on the consuming application. This keeps libraries
reusable and the dependency graph pointing from the application toward its
dependencies.

## Format

Format C/C++ files under `lib/` with the repository `clang-format` policy by
passing it explicitly to `clang-format --style=file:clang-format`.
