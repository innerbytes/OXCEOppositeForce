#pragma once

namespace OpenXcom
{
    class ScriptParserBase;
}  // namespace OpenXcom

namespace OppositeForce::MissionScriptController
{
    /// Registers the battle_game OF mission commands with a script parser.
    void registerScriptBindings(OpenXcom::ScriptParserBase *parser);
}  // namespace OppositeForce::MissionScriptController
