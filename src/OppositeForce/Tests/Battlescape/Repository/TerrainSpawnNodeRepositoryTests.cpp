#include "OppositeForce/Battlescape/Repository/TerrainSpawnNodeRepository.h"
#include "Savegame/Node.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;

TEST_CASE("TerrainSpawnNodeRepository_CustomOrOrdinaryNode_RoundTripsOnlyCustomMetadata", "[terrain-spawns][save]")
{
    // Arrange
    const int size = GENERATE(0, 1, 2);
    Node original;
    original.ofSetSpawnSize(size);
    YAML::YamlRootNodeWriter snapshot;
    snapshot.setAsMap();
    snapshot.write("id", 5);
    Node restored;
    restored.ofSetSpawnSize(2);

    // Act
    TerrainSpawnNodeRepository::save(original, snapshot);
    TerrainSpawnNodeRepository::load(restored, snapshot.toReader());

    // Assert
    REQUIRE(restored.ofGetSpawnSize() == size);
    REQUIRE(snapshot.toReader()["id"].readVal<int>() == 5);
    REQUIRE(static_cast<bool>(snapshot.toReader()["ofSpawnSize"]) == (size != 0));
    REQUIRE_FALSE(snapshot.toReader()["position"]);
    REQUIRE_FALSE(snapshot.toReader()["links"]);
}

TEST_CASE("TerrainSpawnNodeRepository_OldSaveWithoutMetadata_ResetsCustomSize", "[terrain-spawns][save]")
{
    // Arrange
    YAML::YamlRootNodeReader snapshot(YAML::YamlString("id: 5"), "legacy node");
    Node node;
    node.ofSetSpawnSize(2);

    // Act
    TerrainSpawnNodeRepository::load(node, snapshot);

    // Assert
    REQUIRE(node.ofGetSpawnSize() == 0);
}
