/*
 * OXCE-OF: Assign one craft to each compatible hangar and reserve space for
 * incoming craft. This module owns all opt-in hangar matching behavior.
 */
#pragma once

#include <map>
#include <vector>
#include "../../Mod/RuleBaseFacilityFunctions.h"

namespace OpenXcom
{

class Base;
class BaseFacility;
class Craft;
class RuleCraft;
class RuleBaseFacility;
struct TransferRow;

namespace HangarCompatibility
{

/// Craft placement selected for the base display.
struct Assignment
{
	/// Whether any typed hangar or craft rule enables compatibility matching.
	bool hangarCompatibilityEnabled = false;
	/// Craft selected for each completed hangar facility.
	std::map<const BaseFacility *, Craft *> craftByFacility;
	/// Gets the craft selected for a facility, if any.
	Craft *getCraftForFacility(const BaseFacility *facility) const;
};

/// Returns whether existing base rules enable typed hangar matching.
bool isEnabledForBase(const Base &base);
/// Returns whether typed hangar matching applies to a craft added to this base.
bool shouldApplyForCraft(const Base &base, const RuleCraft *candidate);
/// Returns whether a craft addition lacks a compatible hangar.
bool lacksCompatibleSlot(const Base &base, const RuleCraft *candidate,
	const std::vector<TransferRow> *pending = nullptr, bool pendingCraftObjects = false);
/// Counts hangars available to a craft after existing and pending reservations.
int getFreeHangarsCount(const Base &base, const RuleCraft *candidate,
	const std::vector<TransferRow> *pending = nullptr, bool pendingCraftObjects = false);
/// Checks whether facility removal or replacement preserves a valid assignment.
bool fitsAfterChange(const Base &base, BaseAreaSubset removed, const RuleBaseFacility *replacement);
/// Returns whether a base with typed rules has an invalid craft assignment.
bool hasInvalidAssignment(const Base &base);
/// Returns true once for each newly invalid base assignment.
bool shouldWarn(const Base &base);
/// Selects display craft for compatible hangars, with a visual fallback.
Assignment calculateDisplayAssignment(const Base &base);
/// Updates the craft attached to a facility before it is destroyed.
void prepareDestruction(Base &base, BaseFacility &facility);
/// Updates the craft attached to a facility before damage replaces it.
void prepareDamage(Base &base, BaseFacility &facility);
/// Checks whether a damaged hangar can keep its assigned craft.
bool canKeepAfterDamage(const Base &base, const BaseFacility &damaged, const BaseFacility &replacement);

}

}
