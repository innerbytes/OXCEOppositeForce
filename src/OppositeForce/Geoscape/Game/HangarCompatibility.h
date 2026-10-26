/*
 * OXCE-OF: Assign one craft to each compatible hangar and reserve space for
 * incoming craft. This module owns all opt-in hangar matching behavior.
 */
#pragma once

#include <map>
#include <vector>

#include "../../../Mod/RuleBaseFacilityFunctions.h"

namespace OpenXcom
{
    class Base;

    class BaseFacility;

    class Craft;

    class RuleCraft;

    class RuleBaseFacility;

    struct TransferRow;
}  // namespace OpenXcom

namespace OppositeForce::HangarCompatibility
{
    /// Craft placement selected for the base display.
    struct Assignment
    {
        /// Whether any typed hangar or craft rule enables compatibility matching.
        bool hangarCompatibilityEnabled = false;

        /// Craft selected for each completed hangar facility.
        std::map<const OpenXcom::BaseFacility *, OpenXcom::Craft *> craftByFacility;

        /// Gets the craft selected for a facility, if any.
        OpenXcom::Craft *getCraftForFacility(const OpenXcom::BaseFacility *facility) const;
    };

    /// Returns whether existing base rules enable typed hangar matching.
    bool isEnabledForBase(const OpenXcom::Base &base);

    /// Returns whether typed hangar matching applies to a craft added to this base.
    bool shouldApplyForCraft(const OpenXcom::Base &base, const OpenXcom::RuleCraft *candidate);

    /// Returns whether a craft addition lacks a compatible hangar.
    bool lacksCompatibleSlot(const OpenXcom::Base &base, const OpenXcom::RuleCraft *candidate,
                             const std::vector<OpenXcom::TransferRow> *pending = nullptr,
                             bool pendingCraftObjects = false);

    /// Counts hangars available to a craft after existing and pending reservations.
    int getFreeHangarsCount(const OpenXcom::Base &base, const OpenXcom::RuleCraft *candidate,
                            const std::vector<OpenXcom::TransferRow> *pending = nullptr,
                            bool pendingCraftObjects = false);

    /// Checks whether facility removal or replacement preserves a valid assignment.
    bool fitsAfterChange(const OpenXcom::Base &base, OpenXcom::BaseAreaSubset removed,
                         const OpenXcom::RuleBaseFacility *replacement);

    /// Returns whether a base with typed rules has an invalid craft assignment.
    bool hasInvalidAssignment(const OpenXcom::Base &base);

    /// Returns true once for each newly invalid base assignment.
    bool shouldWarn(const OpenXcom::Base &base);

    /// Selects display craft for compatible hangars, with a visual fallback.
    Assignment calculateDisplayAssignment(const OpenXcom::Base &base);

    /// Updates the craft attached to a facility before it is destroyed.
    void prepareDestruction(OpenXcom::Base &base, OpenXcom::BaseFacility &facility);

    /// Updates the craft attached to a facility before damage replaces it.
    void prepareDamage(OpenXcom::Base &base, OpenXcom::BaseFacility &facility);

    /// Checks whether a damaged hangar can keep its assigned craft.
    bool canKeepAfterDamage(const OpenXcom::Base &base, const OpenXcom::BaseFacility &damaged,
                            const OpenXcom::BaseFacility &replacement);
}  // namespace OppositeForce::HangarCompatibility
