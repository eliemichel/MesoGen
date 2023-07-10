/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */
#pragma once

#include "ndvector.h"
#include "Ruleset.h"
#include "SlotTopology.h"

#include <utils/streamutils.h>
#include <Logger.h>
#include <magic_enum.hpp>
#include <unordered_set>
#include <random>

namespace libwfc {

    template <
        typename TileSuperposition,
        typename SlotIndex,
        typename RelationEnum,
        typename Tile = TileSuperposition::tile_type,
        bool Verbose = false
    >
    class Solver {
    public:
        struct Options {
            int maxSteps = 100000;
            int maxAttempts = 20;
            int randomSeed = 0;
            int debug = 0;
            bool useRecursive = false; // recursive is the reference implem but it does not scale well (stack overflow)
        };
        const Options& options() const { return m_options; }
        Options& options() { return m_options; }

        struct Stats {
            int attemptCount = 0;
            int observeCount = 0;
            int choiceCount = 0; // number of observations for which there were several possibilities
            std::vector<std::array<std::optional<std::pair<TileSuperposition, RelationEnum>>, 4>> impossibleNeighborhoods;
        };
        const Stats& stats() const { return m_stats; }

        enum class Status {
            Finished,
            Failed,
            Continue,
        };

    public:
        Solver(
            const AbstractSlotTopology<SlotIndex, RelationEnum>& topology,
            const AbstractRuleset<Tile, TileSuperposition, RelationEnum>& ruleset,
            TileSuperposition tileSuperpositionExample
        )
            : m_topology(topology)
            , m_ruleset(ruleset)
            , m_slots(topology.slotCount(), tileSuperpositionExample)
        {}

        const std::vector<TileSuperposition>& slots() const { return m_slots; }
        std::vector<TileSuperposition>& slots() { return m_slots; }

        bool solve(bool resetBefore = true) {
            if (resetBefore) {
                if (!reset(true)) {
                    // Initial configuration cannot be solved
                    return false;
                }
            }
            m_initialSlots = m_slots;

            for (int i = 0; i < m_options.maxAttempts; ++i) {
                if constexpr (Verbose) {
                    DEBUG_LOG << "Attempt #" << i;
                }

                if (trySolve()) {
                    return true;
                }

                restart();
            }

            WARN_LOG << "Maximum number of attempts exceeded!";
            return false;
        }

        bool reset(bool resetRandomEngine = true) {
            for (auto& slot : m_slots) {
                slot.setToAll();
            }
            if (resetRandomEngine) {
                m_rand_engine.seed(options().randomSeed);
            }

            m_stats = Stats();

            if (!applyInitialContraints()) {
                return false;
            }

            if (options().debug == -1) {
                std::ostringstream ss;
                for (int i = 0; i < m_slots.size(); ++i) {
                    ss << " - #" << i << ": " << m_slots[i] << "\n";
                }
                LOG << "After initial constraints, m_slots = [\n" << ss.str() << "]";
            }

            // Propagate from all slots for init
            bool success = true;
            for (SlotIndex slotIndex = 0; slotIndex < m_slots.size(); ++slotIndex) {
                if (!propagate(slotIndex)) {
                    if constexpr (Verbose) {
                        DEBUG_LOG << "Nothing to propagate, cannot solve.";
                    }
                    success = false;
                    break;
                }
            }

            if (options().debug == -1) {
                assert(checkIntegrity());
                std::ostringstream ss;
                for (int i = 0; i < m_slots.size(); ++i) {
                    ss << " - #" << i << ": " << m_slots[i] << "\n";
                }
                LOG << "After initial propagation, m_slots = [\n" << ss.str() << "]";
            }

            return success;
        }

        void restart() {
            m_slots = m_initialSlots;
            //m_stats = Stats();
            if (options().debug == -1) {
                LOG << "Restart";
            }
        }

        Status step() {
            auto slotIndexOpt = observe();
            if (!slotIndexOpt.has_value()) {
                if constexpr (Verbose) {
                    DEBUG_LOG << "Nothing left to observe, we are done.";
                }
                return Status::Finished;
            }

            SlotIndex slotIndex = *slotIndexOpt;
            if (!propagate(slotIndex)) {
                if constexpr (Verbose) {
                    DEBUG_LOG << "Nothing to propagate, cannot solve.";
                }
                return Status::Failed;
            }

            return Status::Continue;
        }

        int randInt(int maxBound) {
            std::uniform_int_distribution dist(0, maxBound - 1);
            return dist(m_rand_engine);
        }

    protected:
        /**
         * To apply initial contraints (remove possible values from slots at init)
         * inherit from this class and override this method.
         * (I know I'm mixing inheritance and template class it's a bit messy)
         * @return false if initial conditions cannot be met (empty slot)
         */
        virtual bool applyInitialContraints() { return true; }

        /**
         * For debug purpose only
         */
        virtual bool checkImpossibleNeighborhood(const std::array<std::optional<std::pair<TileSuperposition, RelationEnum>>, 4>& neighborhood) const { (void)neighborhood; return true; }

        /**
         * For debug purpose only
         * When allowIncompatible, we allow tiles that are not compatible with neighbors
		 * but still require that when a tile is not present, it is because none of the
		 * neighbors is compatible with it.
         */
        virtual bool checkIntegrity(bool allowIncompatible = false) const { (void)allowIncompatible; return true; }

        // Accessors to be used when implementing applyInitialContraints
        const AbstractSlotTopology<SlotIndex, RelationEnum>& topology() const { return m_topology; }

    private:
        bool trySolve() {
            ++m_stats.attemptCount;

            for (int i = 0; i < m_options.maxSteps; ++i) {
                if constexpr (Verbose) {
                    DEBUG_LOG << "  Iteration #" << i << ":";
                    print_array(DEBUG_LOG, m_slots);
                }

                Status status = step();
                if (status == Status::Failed) return false;
                if (status == Status::Finished) return true;
            }

            return false;
        }

        /**
         * Observe the less entropic superposition (reduce it to a single tile) and
         * return its index, or none if there is nothing left to observe
         */
        std::optional<SlotIndex> observe() {
            ++m_stats.observeCount;

            // 1. Find less entropic superposition
            float minEntropy = std::numeric_limits<float>::max();
            std::unordered_set<SlotIndex> argminEntropy;
            SlotIndex slotIndex = -1;
            for (auto& slot : m_slots) {
                ++slotIndex;
                float entropy = slot.measureEntropy();
                if (entropy > 0 && entropy < minEntropy) {
                    minEntropy = entropy;
                    argminEntropy.clear();
                    argminEntropy.insert(slotIndex);
                }
                else if (minEntropy == entropy) {
                    argminEntropy.insert(slotIndex);
                }
            }

            if constexpr (Verbose) {
                DEBUG_LOG
                    << "minEntropy = " << minEntropy << ", "
                    << "argminEntropy.size() = " << argminEntropy.size();
            }

            if (argminEntropy.empty()) {
                return {};
            }

            // 2. Pick one of the possible tile in one of the possible slots
            auto it = argminEntropy.begin();
            if (argminEntropy.size() > 1) {
                std::uniform_int_distribution dist(0, static_cast<int>(argminEntropy.size()) - 1);
                std::advance(it, dist(m_rand_engine));
            }
            SlotIndex observedSlotIndex = *it;
            if (m_slots[observedSlotIndex].tileCount() > 1) {
                ++m_stats.choiceCount;
                m_slots[observedSlotIndex].observe(m_rand_engine);
            }
            if constexpr (Verbose) {
                DEBUG_LOG << "Observed Slot #" << observedSlotIndex;
            }
            return observedSlotIndex;
        }

        bool propagate(SlotIndex slotIndex) {
            return
                options().useRecursive
                ? propagate_rec(slotIndex)
                : propagate_it(slotIndex);
        }

        /**
         * Propagate a change that has been made to the superposition at a
         * given slot to its neighbors, recursively, and return false if this
         * leads to an inconsistent state.
         */
        bool propagate_rec(SlotIndex slotIndex) {
            if (options().debug == -1) {
                std::ostringstream ss;
                for (int i = 0; i < m_slots.size(); ++i) {
                    ss << " - #" << i << ": " << m_slots[i] << "\n";
                }
                LOG << "Starting propagation from slot #" << slotIndex << ", m_slots = [\n" << ss.str() << "]";
                assert(checkIntegrity(true /* allowIncompatible */));
            }
            for (auto grelation : magic_enum::enum_values<RelationEnum>()) {
                auto neighborSlotIndexOpt = m_topology.neighborOf(slotIndex, grelation);
                if (neighborSlotIndexOpt.has_value()) {
                    SlotIndex neighborSlotIndex = neighborSlotIndexOpt->first;
                    RelationEnum neighborRelation = neighborSlotIndexOpt->second;
                    TileSuperposition& neighborSlot = m_slots[neighborSlotIndex];
#ifndef NDEBUG
                    TileSuperposition previousNeighborSlot = neighborSlot;
#endif // NDEBUG

                    if (options().debug == -1) {
                        if (neighborSlotIndex == 3) {
                            LOG << "break";
                        }
                    }
                    bool neighborWasObserved = neighborSlot.tileCount() == 1;

                    // Build a mask
                    TileSuperposition allowed = m_ruleset.allowedStates(m_slots[slotIndex], grelation, neighborRelation);
                    //assert(allowed.checkIntegrity());

                    // Apply the mask to the neighbor
                    bool changed = neighborSlot.maskBy(allowed);

                    // Check that sizes of superpositions only decrease
                    assert(neighborSlot.tileCount() <= previousNeighborSlot.tileCount());

                    if (changed) {
                        if (neighborSlot.isEmpty()) {
                            // Inconsistency, abort
                            //DEBUG_LOG << "Inconsistency, abort";
                            logImpossibleNeighborhoods(neighborSlotIndex, neighborWasObserved);

                            return false;
                        }

                        // Recursive call
                        if (!propagate_rec(neighborSlotIndex)) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        /**
         * Iterative version of the propagation
         * WARNING: This implementation is not equivalent, because it walks the graph breadth first rather than depth first
         * (to address this, the Frame should contain the grelation)
         */
        bool propagate_it(SlotIndex slotIndex) {
            struct Frame {
                SlotIndex slotIndex;
            };
            std::vector<Frame> stack;

            auto process = [&stack, this](const Frame& frame) -> bool {
                for (auto grelation : magic_enum::enum_values<RelationEnum>()) {
                    auto neighborSlotIndexOpt = m_topology.neighborOf(frame.slotIndex, grelation);
                    if (neighborSlotIndexOpt.has_value()) {
                        SlotIndex neighborSlotIndex = neighborSlotIndexOpt->first;
                        RelationEnum neighborRelation = neighborSlotIndexOpt->second;
                        TileSuperposition& neighborSlot = m_slots[neighborSlotIndex];

                        bool neighborWasObserved = neighborSlot.tileCount() == 1;

                        // Build a mask
                        TileSuperposition allowed = m_ruleset.allowedStates(m_slots[frame.slotIndex], grelation, neighborRelation);
                        //assert(allowed.checkIntegrity());

                        // Apply the mask to the neighbor
                        bool changed = neighborSlot.maskBy(allowed);

                        if (changed) {
                            if (neighborSlot.isEmpty()) {
                                // Inconsistency, abort
                                //DEBUG_LOG << "Inconsistency, abort";
                                logImpossibleNeighborhoods(neighborSlotIndex, neighborWasObserved);

                                return false;
                            }

                            // "Recursive" call
                            stack.push_back(Frame{ neighborSlotIndex });
                        }
                    }
                }
                return true;
            };

            {
                // Initial frame
                Frame frame = { slotIndex };
                stack.push_back(frame);
            }

            while (!stack.empty()) {
                auto frame = stack.back();
                stack.pop_back();
                if (!process(frame)) {
                    return false;
                }
            }

            return true;
        }

    protected:
        /**
         * Get the neighborhood of a given slot.
         * For each direction, it tells the set of all tiles that are around and
         * in which relation they are, or {} if we are at a border.
         */
        std::array<std::optional<std::pair<TileSuperposition, RelationEnum>>, 4> getNeighborhood(SlotIndex slotIndex) const {
            std::array<std::optional<std::pair<TileSuperposition, RelationEnum>>, 4> neighborhood;
            for (int rel = 0; rel < 4; ++rel) {
                auto neighborSlotIndexOpt = m_topology.neighborOf(slotIndex, static_cast<RelationEnum>(rel));
                if (neighborSlotIndexOpt.has_value()) {
                    // Neighbors must not be empty when logImpossibleNeighborhoods is called
                    assert(!m_slots[neighborSlotIndexOpt->first].isEmpty());
                    neighborhood[rel] = std::make_pair(
                        m_slots[neighborSlotIndexOpt->first],
                        neighborSlotIndexOpt->second
                    );
                }
            }
            return neighborhood;
        }

        /**
         * Add to stats the neighborhood of a given slot, deeming it as impossible
         * This must be called on the first slot that gets empty, thus its neighbors
         * are non-empty by construction.
         */
        void logImpossibleNeighborhoods(SlotIndex slotIndex, bool neighborWasObserved = false) {
            auto neighborhood = getNeighborhood(slotIndex);
            m_stats.impossibleNeighborhoods.push_back(neighborhood);

            (void)neighborWasObserved;
            assert(neighborWasObserved || checkImpossibleNeighborhood(neighborhood));
        }

    private:
        Options m_options;
        Stats m_stats;
        const AbstractSlotTopology<SlotIndex, RelationEnum>& m_topology;
        const AbstractRuleset<Tile, TileSuperposition, RelationEnum>& m_ruleset;
        std::vector<TileSuperposition> m_slots;
        std::vector<TileSuperposition> m_initialSlots;
        std::minstd_rand m_rand_engine;
    };

} // namespace libwfc
