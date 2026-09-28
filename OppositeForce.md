# Opposite Force extended rules

This document describes ruleset properties added by OXCE Opposite Force. Standard OXCE properties remain documented in the upstream ruleset reference. The properties below are optional: rulesets that do not use them retain the original behavior.

## Hangar compatibility

Add `hangarType` to a base facility to give its hangar a type. Add `allowedHangarTypes` to a craft to list the types it can use. Type names are arbitrary strings; they are not limited to built-in sizes. Matching is exact and case-sensitive.

```yaml
facilities:
  - type: STR_SMALL_HANGAR
    crafts: 1
    hangarType: A_SMALL
  - type: STR_MEDIUM_HANGAR
    crafts: 1
    hangarType: B_MEDIUM
  - type: STR_LARGE_HANGAR
    crafts: 1
    hangarType: C_LARGE

crafts:
  - type: STR_SMALL_SCOUT
    allowedHangarTypes: [A_SMALL, B_MEDIUM, C_LARGE]
  - type: STR_MEDIUM_SCOUT
    allowedHangarTypes: [B_MEDIUM, C_LARGE]
  - type: STR_BATTLESHIP
    allowedHangarTypes: [C_LARGE]
```

These are partial rule entries; supply the other fields each facility and craft normally needs. Each typed hangar must declare `crafts: 1`. Each hangar still holds at most one craft.

| Property             | Location       | Requirements                                                                   |
| -------------------- | -------------- | ------------------------------------------------------------------------------ |
| `hangarType`         | `facilities[]` | A non-empty string. A facility with this property must have `crafts: 1`.       |
| `allowedHangarTypes` | `crafts[]`     | A non-empty list of non-empty strings. Repeated names are treated as one type. |

If either side omits its property, that craft–hangar pairing is unrestricted. For example, a typed craft can use an untyped hangar, and an untyped craft can use a typed hangar. Omitting both properties preserves ordinary OXCE behavior.

When compatibility applies, the game checks a one-to-one assignment for craft at the base and reserved incoming craft. Hangar type names are tried in alphabetical order, so prefixes such as `A_SMALL`, `B_MEDIUM`, and `C_LARGE` express a preference. The assignment may use a later compatible type when necessary to fit all craft; alphabetical order is a preference, not an absolute placement guarantee.
