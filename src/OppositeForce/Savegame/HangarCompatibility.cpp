/*
 * OXCE-OF: Match craft, transfers, and production reservations to typed
 * hangars. Unannotated rules use the original unrestricted capacity model.
 */

#include "HangarCompatibility.h"

#include <algorithm>
#include <functional>
#include <map>
#include <numeric>

#include "../../Engine/GraphSubset.h"
#include "../../Mod/RuleBaseFacility.h"
#include "../../Mod/RuleCraft.h"
#include "../../Mod/RuleManufacture.h"
#include "../../Savegame/Base.h"
#include "../../Savegame/BaseFacility.h"
#include "../../Savegame/Craft.h"
#include "../../Savegame/Production.h"
#include "../../Savegame/Transfer.h"
#include "../Mod/HangarRules.h"

namespace OpenXcom
{

    namespace HangarCompatibility
    {

        struct Slot
        {
            const BaseFacility *facility;
            const RuleBaseFacility *rule;
        };

        struct Request
        {
            const RuleCraft *rule;
            Craft *craft;
        };

        static void appendSlots(const RuleBaseFacility *rule, const BaseFacility *facility, std::vector<Slot> &slots)
        {
            for (int i = 0; i < rule->getCrafts(); ++i)
            {
                slots.push_back({facility, rule});
            }
        }

        static std::vector<Slot> collectSlots(const Base &base)
        {
            std::vector<Slot> slots;
            for (const auto *facility : *base.ofGetFacilities())
            {
                if (facility->getBuildTime() == 0)
                {
                    appendSlots(facility->getRules(), facility, slots);
                }
            }
            return slots;
        }

        static std::vector<Request> collectRequests(const Base &base, int proposedCapacity = 0)
        {
            std::vector<Request> requests;
            for (auto *craft : *base.getCrafts())
            {
                requests.push_back({craft->getRules(), craft});
            }
            for (auto *transfer : *base.getTransfers())
            {
                if (transfer->getType() == TRANSFER_CRAFT)
                {
                    for (int i = 0; i < transfer->getQuantity(); ++i)
                    {
                        requests.push_back({transfer->getCraft()->getRules(), nullptr});
                    }
                }
            }
            for (const auto *production : base.getProductions())
            {
                if (const RuleCraft *craft = production->getRules()->getProducedCraft())
                {
                    const int capacity = std::max(base.getAvailableHangars(), proposedCapacity);
                    const int limit = std::max(0, capacity + 1 - static_cast<int>(requests.size()));
                    const int remaining = std::max(0, production->getAmountTotal() - production->getAmountProduced());
                    for (int i = 0; i < std::min(limit, remaining); ++i)
                    {
                        requests.push_back({craft, nullptr});
                    }
                }
            }
            return requests;
        }

        static void appendPendingRequests(std::vector<Request> &requests, const std::vector<TransferRow> *pending,
                                          bool craftObjects)
        {
            if (!pending) return;
            for (const auto &row : *pending)
            {
                if (row.type != TRANSFER_CRAFT) continue;
                const RuleCraft *rule = craftObjects ? static_cast<const Craft *>(row.rule)->getRules()
                                                     : static_cast<const RuleCraft *>(row.rule);
                for (int i = 0; i < row.amount; ++i)
                {
                    requests.push_back({rule, nullptr});
                }
            }
        }

        static bool isHangarCompatibilityEnabledForRequests(const std::vector<Slot> &slots,
                                                            const std::vector<Request> &requests)
        {
            for (const auto &slot : slots)
            {
                if (!slot.rule->ofGetHangarCompatibilityRule().hangarType.empty()) return true;
            }
            for (const auto &request : requests)
            {
                if (request.rule->ofGetHangarCompatibilityRule().allowedHangarTypesSpecified) return true;
            }
            return false;
        }

        static size_t getCraftRestrictionRank(const Request &request)
        {
            const auto &rule = request.rule->ofGetHangarCompatibilityRule();
            return rule.allowedHangarTypesSpecified ? rule.allowedHangarTypes.size() : static_cast<size_t>(-1);
        }

        static bool matchRequestsToSlots(const std::vector<Slot> &slots, const std::vector<Request> &requests,
                                         std::vector<int> *assigned = nullptr)
        {
            std::vector<int> slotOrder(slots.size());
            std::iota(slotOrder.begin(), slotOrder.end(), 0);
            // Rule authors can prefix type names to set a stable hangar preference.
            std::stable_sort(slotOrder.begin(), slotOrder.end(), [&](int a, int b) {
                const std::string &left = slots[a].rule->ofGetHangarCompatibilityRule().hangarType;
                const std::string &right = slots[b].rule->ofGetHangarCompatibilityRule().hangarType;
                if (left.empty() != right.empty()) return !left.empty();
                return left < right;
            });
            std::vector<int> requestOrder(requests.size());
            std::iota(requestOrder.begin(), requestOrder.end(), 0);
            std::stable_sort(requestOrder.begin(), requestOrder.end(), [&](int a, int b) {
                return getCraftRestrictionRank(requests[a]) < getCraftRestrictionRank(requests[b]);
            });
            std::vector<int> owner(slots.size(), -1);
            std::function<bool(int, std::vector<bool> &)> place = [&](int request, std::vector<bool> &visited) {
                for (int slot : slotOrder)
                {
                    if (!visited[slot] && owner[slot] == -1 &&
                        HangarRules::accepts(slots[slot].rule->ofGetHangarCompatibilityRule(),
                                             requests[request].rule->ofGetHangarCompatibilityRule()))
                    {
                        owner[slot] = request;
                        return true;
                    }
                }
                for (int slot : slotOrder)
                {
                    if (visited[slot] || !HangarRules::accepts(slots[slot].rule->ofGetHangarCompatibilityRule(),
                                                               requests[request].rule->ofGetHangarCompatibilityRule()))
                        continue;
                    visited[slot] = true;
                    if (owner[slot] != -1 && place(owner[slot], visited))
                    {
                        owner[slot] = request;
                        return true;
                    }
                }
                return false;
            };
            bool complete = true;
            for (int request : requestOrder)
            {
                std::vector<bool> visited(slots.size(), false);
                if (!place(request, visited)) complete = false;
            }
            if (assigned)
            {
                assigned->assign(requests.size(), -1);
                for (size_t i = 0; i < owner.size(); ++i)
                {
                    if (owner[i] != -1) (*assigned)[owner[i]] = i;
                }
            }
            return complete;
        }

        Craft *Assignment::getCraftForFacility(const BaseFacility *facility) const
        {
            auto found = craftByFacility.find(facility);
            return found == craftByFacility.end() ? nullptr : found->second;
        }

        bool isEnabledForBase(const Base &base)
        {
            auto slots = collectSlots(base);
            auto requests = collectRequests(base);
            return isHangarCompatibilityEnabledForRequests(slots, requests);
        }

        bool shouldApplyForCraft(const Base &base, const RuleCraft *candidate)
        {
            if (!candidate) return false;
            auto slots = collectSlots(base);
            auto requests = collectRequests(base);
            requests.push_back({candidate, nullptr});
            return isHangarCompatibilityEnabledForRequests(slots, requests);
        }

        bool lacksCompatibleSlot(const Base &base, const RuleCraft *candidate, const std::vector<TransferRow> *pending,
                                 bool pendingCraftObjects)
        {
            return shouldApplyForCraft(base, candidate) &&
                   getFreeHangarsCount(base, candidate, pending, pendingCraftObjects) <= 0;
        }

        int getFreeHangarsCount(const Base &base, const RuleCraft *candidate, const std::vector<TransferRow> *pending,
                                bool pendingCraftObjects)
        {
            if (!candidate) return 0;
            auto slots = collectSlots(base);
            auto requests = collectRequests(base);
            const int existingCrafts = static_cast<int>(requests.size());
            appendPendingRequests(requests, pending, pendingCraftObjects);
            if (!isHangarCompatibilityEnabledForRequests(slots, requests) &&
                !candidate->ofGetHangarCompatibilityRule().allowedHangarTypesSpecified)
            {
                return base.getAvailableHangars() - base.getUsedHangars() -
                       (static_cast<int>(requests.size()) - existingCrafts);
            }
            if (!matchRequestsToSlots(slots, requests)) return 0;
            int free = 0;
            for (size_t i = requests.size(); i < slots.size(); ++i)
            {
                requests.push_back({candidate, nullptr});
                if (!matchRequestsToSlots(slots, requests)) break;
                ++free;
            }
            return free;
        }

        bool fitsAfterChange(const Base &base, BaseAreaSubset removed, const RuleBaseFacility *replacement)
        {
            std::vector<Slot> slots;
            for (const auto *facility : *base.ofGetFacilities())
            {
                if (BaseAreaSubset::intersection(facility->getPlacement(), removed)) continue;
                if (facility->getBuildTime() == 0 || facility->getIfHadPreviousFacility())
                {
                    appendSlots(facility->getRules(), facility, slots);
                }
            }
            if (replacement) appendSlots(replacement, nullptr, slots);
            auto requests = collectRequests(base, slots.size());
            return !isHangarCompatibilityEnabledForRequests(slots, requests) || matchRequestsToSlots(slots, requests);
        }

        bool hasInvalidAssignment(const Base &base)
        {
            auto slots = collectSlots(base);
            auto requests = collectRequests(base);
            return isHangarCompatibilityEnabledForRequests(slots, requests) && !matchRequestsToSlots(slots, requests);
        }

        bool shouldWarn(const Base &base)
        {
            static std::map<const Base *, bool> warned;
            if (!hasInvalidAssignment(base))
            {
                warned.erase(&base);
                return false;
            }
            if (warned[&base]) return false;
            warned[&base] = true;
            return true;
        }

        Assignment calculateDisplayAssignment(const Base &base)
        {
            Assignment result;
            auto slots = collectSlots(base);
            auto requests = collectRequests(base);
            result.hangarCompatibilityEnabled = isHangarCompatibilityEnabledForRequests(slots, requests);
            if (!result.hangarCompatibilityEnabled) return result;
            std::vector<int> assigned;
            matchRequestsToSlots(slots, requests, &assigned);
            std::vector<bool> occupied(slots.size(), false);
            for (size_t i = 0; i < requests.size(); ++i)
            {
                if (!requests[i].craft || assigned[i] < 0) continue;
                const auto *facility = slots[assigned[i]].facility;
                if (facility)
                {
                    result.craftByFacility[facility] = requests[i].craft;
                    occupied[assigned[i]] = true;
                }
            }
            for (size_t i = 0; i < requests.size(); ++i)
            {
                if (!requests[i].craft || assigned[i] >= 0) continue;
                for (size_t j = 0; j < slots.size(); ++j)
                {
                    if (!occupied[j] && slots[j].facility && !result.getCraftForFacility(slots[j].facility))
                    {
                        result.craftByFacility[slots[j].facility] = requests[i].craft;
                        occupied[j] = true;
                        break;
                    }
                }
            }
            return result;
        }

        void prepareDestruction(Base &base, BaseFacility &facility)
        {
            if (facility.getRules()->getCrafts() <= 0 || !isEnabledForBase(base)) return;
            for (const auto *other : *base.getFacilities())
            {
                if (other != &facility && other->getBuildTime() == 0 && other->getX() == facility.getX() &&
                    other->getY() == facility.getY() && other->getRules()->getCrafts() > 0)
                {
                    return;  // A damaged-hangar replacement already received this craft.
                }
            }
            Craft *craft = calculateDisplayAssignment(base).getCraftForFacility(&facility);
            facility.setCraftForDrawing(craft && craft->getStatus() != "STR_OUT" ? craft : nullptr);
        }

        void prepareDamage(Base &base, BaseFacility &facility)
        {
            if (facility.getRules()->getCrafts() > 0 && isEnabledForBase(base))
            {
                Craft *craft = calculateDisplayAssignment(base).getCraftForFacility(&facility);
                facility.setCraftForDrawing(craft && craft->getStatus() != "STR_OUT" ? craft : nullptr);
            }
        }

        bool canKeepAfterDamage(const Base &base, const BaseFacility &damaged, const BaseFacility &replacement)
        {
            if (!isEnabledForBase(base) || !damaged.getCraftForDrawing()) return true;
            return HangarRules::accepts(replacement.getRules()->ofGetHangarCompatibilityRule(),
                                        damaged.getCraftForDrawing()->getRules()->ofGetHangarCompatibilityRule());
        }

    }  // namespace HangarCompatibility

}  // namespace OpenXcom
