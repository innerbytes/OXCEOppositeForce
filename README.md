# OXCE Opposite Force

OXCE Opposite Force is a strictly additive modification of the OpenXcom Extended (OXCE) engine, adding missing engine features required by the X-COM: Opposite Force mod.

The goal is to extend OXCE without changing existing behavior. The fork is designed to remain fully backwards compatible with standard OXCE rulesets and mods.

Ruleset authors can find the added properties in [Opposite Force rules](OppositeForce.md).

## Demo

[![X-COM: Opposite Force Demo](https://img.youtube.com/vi/UejqRzjD9r0/maxresdefault.jpg)](https://www.youtube.com/watch?v=UejqRzjD9r0)

## Rules

See [Opposite Force extended rules](OppositeForce.md) for the full list of added properties and their usage.

## Extra files

Those files are not to be merged to the upstream OXCE repository.

- This `README.md`. The original OXCE README is moved to `README-OXCE.md` and must follow the upstream.
- `scripts-of` - automation helpers folder
- `package.json` - automation targets
- `.vscode` - vscode settings
- `AGENTS.md` - agents documentation
- [OppositeForce.md](OppositeForce.md) - Opposite Force ruleset properties
- `todo.md` - list of tasks and planned improvements

## References

- [OpenXcom Extended (OXCE)](https://github.com/MeridianOXC/OpenXcom)
- [OpenXcom Extended More (OXCEM)](https://github.com/SCPRUdev/OXCEM)
