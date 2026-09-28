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

These are partial rule entries; supply the other fields each facility and craft normally needs. Each typed hangar must have an effective `crafts` capacity of 1, either declared here or inherited from an existing facility rule. Each hangar still holds at most one craft.

| Property             | Location       | Requirements                                                                   |
| -------------------- | -------------- | ------------------------------------------------------------------------------ |
| `hangarType`         | `facilities[]` | A non-empty string. The facility's effective `crafts` capacity must be 1.      |
| `allowedHangarTypes` | `crafts[]`     | A non-empty list of non-empty strings. Repeated names are treated as one type. |

If either side omits its property, that craft–hangar pairing is unrestricted. For example, a typed craft can use an untyped hangar, and an untyped craft can use a typed hangar. Omitting both properties preserves ordinary OXCE behavior.

When compatibility applies, the game checks a one-to-one assignment for craft at the base and reserved incoming craft. Hangar type names are tried in alphabetical order, so prefixes such as `A_SMALL`, `B_MEDIUM`, and `C_LARGE` express a preference. The assignment may use a later compatible type when necessary to fit all craft; alphabetical order is a preference, not an absolute placement guarantee.

## Funding actor names

A country can have a separate localized name when it acts as a funding source or
political actor. For example, a mod can map a Cydonian Elder to an engine country
while retaining the country name for Earth geography.

Add one optional localization key to the existing country rule:

```yaml
countries:
  - type: STR_USA
    fundingName: STR_ELDER_ZAR
```

Define the translation in the mod's `Language/en-US.yml`:

```yaml
en-US:
  STR_ELDER_ZAR: "Elder Zar"
```

Other locales use the same key in their respective `Language/<locale>.yml` files.
Alternatively, supply the translation in a ruleset:

```yaml
extraStrings:
  - type: en-US
    strings:
      STR_ELDER_ZAR: "Elder Zar"
```

Both methods use the normal language loader. Its existing language fallback and
mod ordering apply; `extraStrings` is loaded after language files. A key with no
translation uses the normal missing-translation behavior (displaying the key).
Do not replace the translation of `STR_USA` itself: that also changes geographic
country labels.

### Where the name is displayed

| Context | Name used |
| --- | --- |
| Funding screen, including name sorting | Funding actor |
| Monthly report: pleased, unhappy, signed pact, cancelled pact | Funding actor |
| Income graph country buttons | Funding actor |
| Stats for Nerds: `requiresBuyCountry` for items, craft, and soldiers | Funding actor |
| UFO and XCOM activity graphs by country | Geographic country |
| Earth Map labels and country-boundary debug selector | Geographic country |
| Alien-base location popup | Geographic country |
| Soldier diary mission location | Geographic country |

Stats for Nerds still shows the original country rule ID when “show IDs” is
selected. Purchase restrictions, political events, funding calculations, scripts,
and saved games continue to use that original ID.

### Defaults and inheritance

This is opt-in. Without a nonempty `fundingName`, the normal country name is used.
The field follows `refNode` inheritance and ruleset layering: omission preserves
an existing value. To clear an inherited or earlier override:

```yaml
countries:
  - type: STR_USA
    fundingName: ""
```

The feature only changes individual actor names. Generic headings and sentences
such as “Country,” “International Relations,” and “Countries Lost” can be changed
using their existing localization keys. No save migration is required.
