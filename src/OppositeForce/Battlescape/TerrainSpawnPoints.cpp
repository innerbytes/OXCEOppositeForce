#include "TerrainSpawnPoints.h"

#include <algorithm>
#include <tuple>

#include "../../Battlescape/Pathfinding.h"
#include "../../Engine/Logger.h"
#include "../../Mod/MapBlock.h"
#include "../../Savegame/Node.h"
#include "../../Savegame/SavedBattleGame.h"
#include "../../Savegame/Tile.h"
#include "TerrainSpawnInternal.h"

namespace OpenXcom::TerrainSpawnPoints
{
    bool canUseNode(const Node &node, const BattleUnit &unit)
    {
        return !node.ofGetSpawnSize() || node.ofGetSpawnSize() == unit.getArmor()->getSize();
    }

    std::vector<Position> getReservedFootprints(SavedBattleGame &save, const BattleUnit &unit)
    {
        std::vector<Position> reservations;
        if (unit.isSmallUnit())
            for (const auto *node : *save.getNodes())
                if (!node->isDummy() && node->ofGetSpawnSize() == 2) reservations.push_back(node->getPosition());
        return reservations;
    }

    bool canSpawnAtNode(SavedBattleGame &save, const Node &node, const BattleUnit &unit,
                        const std::vector<Position> &reservations)
    {
        if (!canUseNode(node, unit)) return false;
        if (!unit.isSmallUnit()) return true;
        auto position = node.getPosition();
        // Ordinary RMP nodes on -24 terrain are raised by normal placement to the next floor.
        // Reserve the resulting footprint, including for nodes authored below a reserved point.
        const auto *tile = save.getTile(position);
        while (tile && tile->getTerrainLevel() == -24)
        {
            ++position.z;
            tile = save.getTile(position);
        }
        for (const auto &anchor : reservations)
        {
            if (position.z == anchor.z && position.x >= anchor.x && position.x < anchor.x + 2 &&
                position.y >= anchor.y && position.y < anchor.y + 2)
                return false;
        }
        return true;
    }

    static bool hasCompatibleSize(const Node &node, int size)
    {
        return !node.isDummy() && !(node.getType() & Node::TYPE_FLYING) &&
               (size == 1 || !(node.getType() & Node::TYPE_SMALL)) &&
               (!node.ofGetSpawnSize() || node.ofGetSpawnSize() == size);
    }

    static bool overlapsPoint(const Node &node, const Position &position, int size)
    {
        const auto other = node.getPosition();
        return !node.isDummy() && node.ofGetSpawnSize() && position.z == other.z &&
               position.x < other.x + node.ofGetSpawnSize() && position.x + size > other.x &&
               position.y < other.y + node.ofGetSpawnSize() && position.y + size > other.y;
    }

    static int calculateDistance(const Position &left, const Position &right)
    {
        const auto difference = left - right;
        return difference.x * difference.x + difference.y * difference.y + difference.z * difference.z;
    }

    static void appendLink(Node &source, int destination)
    {
        auto &links = *source.getNodeLinks();
        if (std::find(links.begin(), links.end(), destination) == links.end()) links.push_back(destination);
    }

    void addSpawnNodes(Mod &mod, SavedBattleGame &save, const std::vector<Placement> &placements)
    {
        if (placements.empty()) return;
        struct AcceptedPoint
        {
            Node *node;
            std::vector<unsigned char> reachable;
        };
        std::vector<AcceptedPoint> accepted;
        for (const auto &placement : placements)
        {
            const auto &block = *placement.block;
            const Position dimensions(block.getSizeX(), block.getSizeY(), block.getSizeZ());
            for (const auto &point : block.ofGetSpawnPoints().points)
            {
                const Position position = point.position + placement.offset;
                WalkingProbe probe(mod, point.unitSize);
                std::string reason;
                std::vector<unsigned char> reachable;
                if (!canStandAtPosition(save, probe.getUnit(), position))
                    reason = "footprint is blocked in the assembled map";
                else
                {
                    reachable = calculateReachablePositions(save, probe.getUnit(), position);
                    if (!hasGroundExit(save, reachable, placement.offset, dimensions, point.unitSize))
                        reason = "no walking route into neighboring ground terrain";
                    else if (std::any_of(accepted.begin(), accepted.end(), [&](const AcceptedPoint &other) {
                                 return overlapsPoint(*other.node, position, point.unitSize);
                             }))
                        reason = "footprint overlaps another placed custom point";
                }
                if (!reason.empty())
                {
                    Log(LOG_WARNING) << TerrainSpawnRules::describePoint(block.ofGetSpawnTerrain(), block.getName(),
                                                                         point)
                                     << ": skipping battle position [" << position.x << ", " << position.y << ", "
                                     << position.z << "]: " << reason;
                    continue;
                }
                auto *node = new Node(save.getNodes()->size(), position, placement.segment,
                                      point.unitSize == 1 ? Node::TYPE_SMALL : 0, point.rank, 1, 0, point.priority);
                node->ofSetSpawnSize(point.unitSize);
                save.getNodes()->push_back(node);
                accepted.push_back({node, std::move(reachable)});
            }
        }
        // All candidates now exist, so links can include custom points in neighboring fragments.
        for (const auto &point : accepted)
        {
            auto &source = *point.node;
            std::vector<Node *> candidates;
            for (auto *node : *save.getNodes())
                if (node != &source && hasCompatibleSize(*node, source.ofGetSpawnSize()) &&
                    save.getTile(node->getPosition()) && point.reachable[save.getTileIndex(node->getPosition())])
                    candidates.push_back(node);
            std::sort(candidates.begin(), candidates.end(), [&](const Node *left, const Node *right) {
                return std::make_tuple(calculateDistance(source.getPosition(), left->getPosition()), left->getID()) <
                       std::make_tuple(calculateDistance(source.getPosition(), right->getPosition()), right->getID());
            });
            if (candidates.size() > 5) candidates.resize(5);
            for (auto *node : candidates)
            {
                appendLink(source, node->getID());
                appendLink(*node, source.getID());
            }
        }
        save.getPathfinding()->setUnit(nullptr);
    }
}  // namespace OpenXcom::TerrainSpawnPoints
