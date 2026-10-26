#include "OppositeForce/Battlescape/Rules/TerrainSpawnRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;

TEST_CASE("TerrainSpawnRules_MalformedDefinitions_ThrowsValidationException", "[terrain-spawns][rules]")
{
    // Arrange
    const auto yaml = GENERATE("mapBlockSpawnPoints: []", "mapBlockSpawnPoints: {BLOCK: {}}",
                               "mapBlockSpawnPoints: {BLOCK: [{unitSize: 1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2], unitSize: 1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0]}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 3}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, rank: -1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, rank: 9}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, priority: 0}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, priority: 256}]}");
    CAPTURE(yaml);
    TerrainSpawnRules::Rules rules;
    YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test-spawns.rul");

    // Act
    const auto load = [&] { TerrainSpawnRules::loadRules(rules, reader["mapBlockSpawnPoints"], "TEST"); };

    // Assert
    REQUIRE_THROWS(load());
}
