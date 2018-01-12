#include <algorithm>
#include "bfs2.h"
#include "cube.h"
#include "kociemba.h"
#include "kociemba_impl.h"


TMove ReplayTurns(std::vector<ETurnExt>::const_iterator begin, std::vector<ETurnExt>::const_iterator end) {
    TMove result;
    for (; begin != end; ++begin)
        result *= TurnExt2Move(*begin);
    return result;
}


/*std::vector<ETurnExt> ReduceSolution(const TCube &puzzle, const std::vector<ETurnExt> &solution,
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
}*/

void InitKociemba() {
    TG0Stage::Instance();
    TG1Stage::Instance();
}

bool KociembaSolution(const TCube &puzzle, std::vector<ETurnExt> &result) {
    auto &g0 = TG0Stage::Instance();
    auto &g1 = TG1Stage::Instance();
    auto candidates = Solve(puzzle, {TMove()}, 100, 12, g0, g1);
    //std::cout << "Stage 1: " << (!candidates.empty() ? Turns2Exts(candidates.front().GetTurns()).size() : 0) << std::endl;
    auto solution = Solve(puzzle, candidates, 2, 30, g1, g1);
    //std::cout << "Stage 2: " << (!solution.empty() ? Turns2Exts(solution.front().GetTurns()).size() : 0) << std::endl;
    if (solution.empty())
        return false;
    result = Turns2Exts(solution.front().GetTurns());
    return true;
}

