# Opposite Force extended rules

This document describes ruleset properties added by OXCE Opposite Force. Standard OXCE properties remain documented in the upstream ruleset reference. The properties below are optional: rulesets that do not use them retain the original behavior.

## Supported Features

- [Hangar compatibility](#hangar-compatibility)
- [Funding actor names](#funding-actor-names)
- [Hidden UFOs](#hidden-ufos)
- [Geographic mission terrain](#geographic-mission-terrain)
- [Excluded landing textures](#excluded-landing-textures)
- [Custom terrain spawn points](#custom-terrain-spawn-points)
- [Disable avatar selection](#disable-avatar-selection)
- [Unit kill credit and stun damage](#unit-kill-credit-and-stun-damage)
- [Script-controlled mission completion](#script-controlled-mission-completion)

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

| Context                                                              | Name used          |
| -------------------------------------------------------------------- | ------------------ |
| Funding screen, including name sorting                               | Funding actor      |
| Monthly report: pleased, unhappy, signed pact, cancelled pact        | Funding actor      |
| Income graph country buttons                                         | Funding actor      |
| Stats for Nerds: `requiresBuyCountry` for items, craft, and soldiers | Funding actor      |
| UFO and XCOM activity graphs by country                              | Geographic country |
| Earth Map labels and country-boundary debug selector                 | Geographic country |
| Alien-base location popup                                            | Geographic country |
| Soldier diary mission location                                       | Geographic country |

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

## Hidden UFOs

Set `isHidden: true` on a UFO type to prevent normal detection and automatic
reveals. The optional boolean defaults to `false`.

```yaml
ufos:
  - type: CRAFT_HIDDEN
    isHidden: true
```

Hidden UFOs bypass base radar, hyperwave, craft radar, and detection scripts,
including guaranteed detection. Crashes and hunter-killer targeting do not
reveal them or trigger hunting notifications. During normal play they have no
globe marker or UFO tracker entry.

Mission waves, movement, landing, and activity continue normally. Created
mission sites remain visible and trigger their normal alerts. Use
`objectiveOnTheLandingSite: true` to place a site at the UFO's landing point;
existing land checks and fallback behavior are unchanged.

The debug **Ctrl+7** command can still reveal hidden UFOs and their hyperwave
information. Loading a save clears their detection flags, including debug
reveals.

## Geographic mission terrain

Set `useGlobalTerrain: true` on an `alienDeployments` entry used by a mission site
to select battlefield terrain from the globe texture at that site's coordinates.
This is useful for mission sites whose stored regional texture differs from the
actual biome.

```yaml
alienDeployments:
  - type: STR_SMALL_RESEARCH
    useGlobalTerrain: true
    width: 40
    length: 40
    height: 4
```

The selected terrain uses its normal map script, block frequencies, and placement
rules. The arriving player craft and any UFO supplied through existing rules use
normal placement. `width`, `length`, and `height` still control battlefield size.

Explicit `terrains`, `script`, and `mapScripts` overrides retain their normal
precedence. A terrain already selected by the caller also takes precedence.

The boolean defaults to `false` and affects initial entry into **mission sites only**.
Direct UFO encounters, base assaults, base defense, and later mission stages keep
their ordinary terrain selection. `refNode` inheritance and later rule layers
preserve omitted values; set
`useGlobalTerrain: false` to disable an inherited value. The site's stored texture
and starting-condition rules remain unchanged.

## Excluded landing textures

Set `excludedLandingTextures` on an `alienMissions` entry to prevent its standard
UFO landing procedure from choosing coordinates on the listed globe texture IDs:

```yaml
alienMissions:
  - type: STR_SMALL_RESEARCH_GROUND
    spawnZone: 1
    excludedLandingTextures: [9, 12]
```

The optional array defaults to empty, preserving the original landing behavior.
Each value is an actual globe texture ID at the sampled coordinates.

Exclusions apply to ordinary UFO landings and also when both
`objectiveOnTheLandingSite: true` on the wave and `useGlobalTerrain: true` on the
site deployment are enabled. The landing coordinates are checked before creating
the site and selecting its battlefield terrain.

The search checks up to 100 random coordinates in the selected area, then tries
the remaining areas in the same zone in random order, once each. Fixed-point
areas are checked once. Existing landing requirements still apply. The bounded
search can miss small valid patches.

If every area fails, the engine logs a warning and skips that UFO spawn or removes
the in-flight UFO. It creates no site and allows later waves to proceed. Excluded
textures are never accepted as a forced fallback.

Directly spawned sites without a UFO, fixed XCOM/alien base targets, crashes, and
separate alien-base placement procedures are not filtered.

## Custom terrain spawn points

Use `mapBlockSpawnPoints` on a terrain to add spawn points to existing fragments.
Definitions apply wherever the terrain is used, including repeated fragments.
No replacement terrain files or extra `mapBlocks` entries are needed.

```yaml
terrains:
  - name: FOREST
    mapBlockSpawnPoints:
      FOREST00:
        - position: [3, 4, 0]
          unitSize: 1
          rank: 0
          priority: 1
        - position: [6, 6, 0]
          unitSize: 2
          rank: 0
          priority: 1
```

The fragment name must match an existing terrain block. These example coordinates
illustrate syntax; they are **not verified locations** in FOREST00.

| Property | Default | Meaning |
| --- | --- | --- |
| `position` | Required | Fragment-local `[x, y, z]`, zero-based, with ground at `z: 0`. |
| `unitSize` | Required | `1` for a 1×1 point or `2` for a dedicated 2×2 point. Each point accepts only its declared size. |
| `rank` | `0` | OXCE node rank, from `0` through `8`; see below. |
| `priority` | `1` | Spawn priority, from `1` through `255`; higher priorities are tried first within a rank. |

A 2×2 point extends toward increasing x and y, occupying `[x, y]`, `[x+1, y]`,
`[x, y+1]`, and `[x+1, y+1]` on the same floor. Its four tiles are reserved against
1×1 spawning across all ranks, including through original nodes. Normal movement
is unaffected. Choose non-overlapping footprints inside the fragment with a
walkable route to its ground-level exterior; upper floors can use stairs.

| Rank | Spawn usage |
| --- | --- |
| `0` | Shared fallback for all AI opponent unit ranks, and the only rank used when deploying them outside a UFO. Neutral AI units also use this by default. |
| `1` | Player-controlled units when deployment uses terrain spawn nodes. Historically called “XCOM”; it does not mean units belonging to XCOM in every mod. |
| `2` | AI opponent spawn category: Soldier. |
| `3` | AI opponent spawn category: Navigator. |
| `4` | AI opponent spawn category: Leader; preferred by both commanders and leaders. |
| `5` | AI opponent spawn category: Engineer. |
| `6` | AI opponent spawn category: Miscellaneous 1. |
| `7` | AI opponent spawn category: Medic. |
| `8` | AI opponent spawn category: Miscellaneous 2. |

These are **location categories**, not faction IDs or deployment `alienRank`
values. They use the same numbers as vanilla `.RMP` spawn nodes. There is no
separate commander location category: commanders and leaders both try node ranks
`4, 3, 5, 8, 7, 2, 0`, in that order. Category names describe preferences rather
than restricting points to one unit role. AI opponents try categories in an order
determined by their deployment rank, with rank `0` as the final fallback. Their identity—police, human soldiers,
aliens, or tanks—does not determine the spawn-node rank.

Use `rank: 0` (or omit it) if any AI opponent should be able to use a point.
Use `unitSize: 1` for small units and `unitSize: 2` for large units; both can use
rank `0`. Ranks `2–8` are useful when you want preferred locations for particular
AI opponent roles. Neutral AI deployment can specify a different node rank.

`refNode` inheritance and later rule layers preserve omitted fragment definitions.
Supplying a fragment's list replaces its previous custom points. Use `[]` to clear
that fragment's custom points:

```yaml
terrains:
  - name: FOREST
    mapBlockSpawnPoints:
      FOREST00: []
```

## Disable avatar selection

Set `disableAvatarSelection: true` on a soldier type to disable the inventory's
manual avatar selector, both through right-click and the avatar keyboard shortcut.
The action silently does nothing; armor selection is unaffected.

```yaml
soldiers:
  - type: STR_SECTOID
    disableAvatarSelection: true
  - type: STR_FLOATER
    disableAvatarSelection: true
```

The optional boolean defaults to `false`, preserving normal avatar selection for
all soldier types that do not opt in, including humans. It follows `refNode`
inheritance and ruleset layering: omission preserves an inherited or previously
loaded value, and explicit `false` re-enables selection.

The restriction applies to all existing and new soldiers of that type. It does
not change their stored gender or appearance, and no save migration is required.

## Unit kill credit and stun damage

Use these optional `units[]` properties for destructible exploration objects:

```yaml
units:
  - type: OBJ_SAMPLE_GROUND
    disableKillRecords: true
    canBeStunned: false
```

| Property | Default | Effect |
| --- | --- | --- |
| `disableKillRecords` | `false` | Excludes this target from damage-related kill records, kill totals and attacker experience. |
| `canBeStunned` | `true` | Set to `false` to prevent stun damage from weapons, explosions and smoke, including attacks that bypass `painImmune`. Other damage still applies. |

With `disableKillRecords: true`, killing or stunning this unit grants no kill
records, kill-based commendations or promotion credit. Damaging it grants no
skill, mana or reaction experience. Shots, hits, diary accuracy and hit-based
commendations still count. Mission participation and unrelated actions can still
advance a soldier.

Mind-control and panic rewards are unchanged. For exploration objects, use
`canBeMindControlled: false` and `canPanic: false` to prevent those attacks.

`canBeStunned: false` protects against stun damage; explicit scripted or mission
knockouts remain possible. Use either property independently, or both together
for exploration objects. Omitting them keeps normal behavior.

## Script-controlled mission completion

Use manual ending for missions whose result is determined by scripts. Enable it
once at mission start:

```text
battle_game.setMissionEndManual 1;
```

The mission then continues after enemy elimination or completion of its ordinary
objectives. Squad defeat, deployment timers (`turnLimit` / `chronoTrigger`) and
player retreat retain their usual effects.

Request the mission result when your scripted objective is decided:

```text
# Objective completed successfully.
battle_game.completeMission 1;

# Objective failed.
battle_game.completeMission 0;
```

Use the appropriate command for the result. Each call replaces any earlier
completion request. Both commands work with manual ending enabled or disabled.
With manual ending disabled, ordinary mission-ending conditions already met when
queued actions finish take priority and use the ordinary mission result. Otherwise,
the requested script result applies before turn-ending effects. Squad defeat and
an already-expired deployment timer take priority in either mode.

For a request made during your turn, press **Next Turn** to complete the mission
before enemy actions. A request made during turn-ending explosions takes effect
after those explosions resolve. Requests made during an enemy or civilian turn
take effect at the next turn transition. Current actions and their queued
consequences finish before mission completion. Squad defeat, a deployment timer or player
retreat can finish the mission first and use their ordinary outcome rules.

Both results finish the entire mission, including deployments with `nextStage`.
Surviving soldiers return from their current positions with their equipment.
After a scripted failure with a surviving squad, the craft returns to base.
Total squad loss results in mission failure even when success was requested.
Normal craft and base-loss rules apply.

The manual mission completion result determines the success/failure titles,
cutscenes, research, bounty rewards, counters and events instead of the ordinary
success or failure of the mission objective would decide on them. The
corresponding `objectiveComplete` or `objectiveFailed` scores are given once at
their configured value.

Restore ordinary OXCE mission-ending conditions with:

```text
battle_game.setMissionEndManual 0;
```

This is the default setting. Changing it restores ordinary mission-ending checks
while keeping any queued completion request. An ordinary ending takes priority
when already due; otherwise the queued result applies. Manual ending and queued
results persist through save/load, including across enemy and civilian turns.
A stage reached through ordinary mission progression starts with ordinary OXCE
ending conditions.
