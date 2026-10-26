#pragma once

namespace OpenXcom
{
    class ScriptParserBase;
}  // namespace OpenXcom

namespace OpenXcom::MissionScriptController
{
    /// Registers the battle_game OF mission commands with a script parser.
    void registerScriptBindings(ScriptParserBase *parser);
}  // namespace OpenXcom::MissionScriptController
