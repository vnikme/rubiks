#include <algorithm>
#include <iostream>
#include "cube.h"


struct TG0Homomorphism {
    using TCubeImageType = TCubeImage<9>;
    TCubeImageType GetImage(const TCube &cube) const {
        TCubeImageType result;
        for (size_t i = 0; i < 9; ++i)
            result.Data[i] = 0;
        const auto &corners = TCube::GetAllCorners();
        const auto &edges = TCube::GetAllEdges();
        for (size_t i = 0; i < 24; ++i) {
            EColor color = cube.GetColor(corners[i]);
            if (color == C_RED || color == C_ORANGE)
                result.Data[i / 8] |= (1 << (i % 8));
            color = cube.GetColor(edges[i]);
            if (color == C_RED || color == C_ORANGE)
                result.Data[3 + i / 8] |= (1 << (i % 8));
            if (color == C_WHITE || color == C_YELLOW) {
                color = cube.GetColor(TCube::GetOppositeEdge(edges[i]));
                if (color == C_GREEN || color == C_BLUE)
                    result.Data[6 + i / 8] |= (1 << (i % 8));
            }
        }
        return result;
    }
};


struct TIsomorphism {
    using TCubeImageType = TCubeImage<(TCube::NUM_FIELDS * TCube::BITS_FOR_COLORS + 7) / 8>;
    TCubeImageType GetImage(const TCube &cube) const {
        return cube.GetImage();
    }
};


TMove ReplayTurns(std::vector<ETurnExt>::const_iterator begin, std::vector<ETurnExt>::const_iterator end) {
    TMove result;
    for (; begin != end; ++begin)
        result *= TurnExt2Move(*begin);
    return result;
}


std::vector<ETurnExt> ReduceSolution(const TCube &puzzle, const std::vector<ETurnExt> &solution,
                                     const std::vector<TMove> &g0, TIsomorphism &iso)
{
    std::vector<ETurnExt> result(solution);
    size_t windowSize = solution.size() >= 10 ? 10 : solution.size();
    TMove current;
    for (std::vector<ETurnExt>::const_iterator begin = solution.begin(), it = solution.begin(), end = solution.end();
         it + windowSize < end;
         current *= TurnExt2Move(*it), ++it)
    {
        TCube from = current.Act(puzzle);
        TCube to = ReplayTurns(it, it + windowSize).Act(from);
        auto candidates = Solve(from, to, {TMove()}, 1, 3000000, g0, iso, iso);
        if (candidates.empty())
            continue;
        TMove m = current * candidates.front() * ReplayTurns(it + windowSize, end);
        auto turns = Turns2Exts(m.GetTurns());
        if (turns.size() < result.size())
            result = turns;
    }
    return result;
}


std::vector<ETurnExt> KociembaSolution(const TCube &puzzle, const TCube &target) {
    TG0Homomorphism hom;
    TIsomorphism iso;
    std::vector<TMove> g0, g1;
    for (size_t i = 0; i < TE_G0_COUNT; ++i)
        g0.push_back(TurnExt2Move(static_cast<ETurnExt>(i)));
    for (size_t i = 0; i < TE_G1_COUNT; ++i)
        g1.push_back(TurnExt2Move(static_cast<ETurnExt>(i)));
    std::vector<TMove> candidates = Solve(puzzle, target, {TMove()}, 10, 3000000, g0, hom, iso);
    auto solution = Solve(puzzle, target, candidates, 1, 3000000, g1, iso, iso);
    return !solution.empty() ? Turns2Exts(solution.front().GetTurns()) : std::vector<ETurnExt>();
}

