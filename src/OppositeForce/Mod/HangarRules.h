/*
 * OXCE-OF: Opt-in hangar type rules shared by Opposite Force craft and facilities.
 * Untyped rules retain the unrestricted OXCE hangar behavior.
 */
#pragma once

#include <string>
#include <vector>
#include "../../Engine/Yaml.h"

namespace OpenXcom
{

namespace HangarRules
{

/// Optional hangar type carried by a base facility rule.
struct Facility
{
	/// Empty when the facility uses unrestricted OXCE hangar behavior.
	std::string hangarType;
};

/// Optional hangar restrictions carried by a craft rule.
struct Craft
{
	/// Whether the craft rule explicitly declares allowed hangar types.
	bool allowedHangarTypesSpecified = false;
	/// Hangar type names that can hold this craft when specified.
	std::vector<std::string> allowedHangarTypes;
};

/// Loads a facility's optional hangar type from one ruleset layer.
void loadFacility(Facility &rule, const YAML::YamlNodeReader &reader, const std::string &name);
/// Requires a typed hangar to hold exactly one craft.
void validateFacility(const Facility &rule, const std::string &name, int crafts);
/// Loads a craft's optional allowed hangar types from one ruleset layer.
void loadCraft(Craft &rule, const YAML::YamlNodeReader &reader, const std::string &name);
/// Returns whether a facility can hold a craft under the opt-in rules.
bool accepts(const Facility &facility, const Craft &craft);

}

}
