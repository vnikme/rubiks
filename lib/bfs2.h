#pragma once

#include "cube.h"
#include <vector>
#include <unordered_map>


template <typename TStage>
void PlainBFS(TStage &stage, std::unordered_map<typename TStage::TCubeImageType, TMove> &reachedPositions, size_t depth) {
    const std::vector<TMove> &allowedMoves = stage.GetAllowedMoves();
    std::vector<TCube> queue;
    TCube cube = MakeSolvedCube();
    queue.push_back(cube);
    reachedPositions[stage.GetImage(cube)] = TMove();
    for (size_t i = 0; i < queue.size(); ++i) {
        TCube currentCube = queue[i];
        TMove currentMove = reachedPositions[stage.GetImage(currentCube)];
        //std::cout << "Processing " << i << ", queue.size()=" << queue.size() << ", currentMove.GetTotalTurnsCount()=" << currentMove.GetTotalTurnsCount() << std::endl;
        for (const TMove &move : allowedMoves) {
            //std::cout << " Turning" << std::endl;
            cube = move.Act(currentCube);
            auto img = stage.GetImage(cube);
            if (reachedPositions.find(img) != reachedPositions.end())
                continue;
            //std::cout << " Got new cube" << std::endl;
            reachedPositions[img] = currentMove * move;
            if (currentMove.GetTotalTurnsCount() + 1 < depth)
                queue.push_back(cube);
        }
    }
}


template<typename THomomorphism>
bool UpdateReached(const TCube &cube, const TMove &move,
                   const THomomorphism &hom, std::unordered_map<typename THomomorphism::TCubeImageType, TMove> &result) {
    auto img = hom.GetImage(cube);
    auto it = result.find(img);
    if (it != result.end()) {
        if (Turns2Exts(move.GetTurns()).size() < Turns2Exts(it->second.GetTurns()).size())
            it->second = move;
        return false;
    } else {
        result[img] = move;
        return true;
    }
}

/*
    BFS2 - two-way bfs, backward moves precomputed
*/
template<typename TCurrentStage, typename TNextStage>
std::unordered_map<typename TNextStage::TCubeImageType, TMove>
    BFS2(const TCube &cube,
         const std::vector<TMove> &doneMoves,
         size_t candidatesCount, size_t maxTotalTurnsCount, size_t maxForwardStageTurnsCount, size_t maxStageTurnsCount,
         const TCurrentStage &currentStage, const TNextStage &nextStage) {
    using TCurrentCubeImage = typename TCurrentStage::TCubeImageType;
    using TNextCubeImage = typename TNextStage::TCubeImageType;
    using TCurrentReachedMap = std::unordered_map<TCurrentCubeImage, TMove>;
    using TNextReachedMap = std::unordered_map<TNextCubeImage, TMove>;
    using TQueue = std::list<TCube>;
    TQueue queue;
    TCurrentReachedMap reached;
    TNextReachedMap result;
    const auto &allowedMoves = currentStage.GetAllowedMoves();
    const auto &reachedBackward = currentStage.GetReachedPositions();
    for (size_t i = 0; i < doneMoves.size() || !queue.empty(); ) {
        //std::cout << i << " " << result.size() << " " << (queue.empty() ? -1 : static_cast<int>(reached[currentStage.GetImage(queue.front())].GetTotalTurnsCount())) << " " << queue.size() << " " << reached.size() << " " << reachedBackward.size() << std::endl;
        for (; i < doneMoves.size() && (queue.empty() ||
               doneMoves[i].GetTotalTurnsCount() <= reached[currentStage.GetImage(queue.front())].GetTotalTurnsCount());
               ++i)
        {
            TMove m = doneMoves[i];
            m.ResetLastStageTurnsCount();
            TCube c = m.Act(cube);
            auto img = currentStage.GetImage(c);
            auto it = reachedBackward.find(img);
            if (it != reachedBackward.end()) {
                TMove solution = m / it->second;
                auto img = nextStage.GetImage(c);
                auto it = result.find(img);
                if (it != result.end()) {
                    if (solution.GetTotalTurnsCount() < it->second.GetTotalTurnsCount())
                        it->second = solution;
                } else {
                    result[img] = solution;
                }
                //std::cout << "i " << i << " " << result.size() << " " << m.GetTotalTurnsCount() << " " << reached.size() << std::endl;
                if (result.size() >= candidatesCount)
                    return result;
            }
            int estimate = currentStage.Estimate(c);
            //std::cout << "i estimate=" << estimate << std::endl;
            if (estimate == -1) {
                //std::cout << "est=-1" << std::endl;
                result.clear();
                return result;
            }
            if (m.GetTotalTurnsCount() + estimate < maxTotalTurnsCount && reached.find(img) == reached.end()) {
                reached[img] = m;
                queue.push_front(c);
            }
        }
        if (!queue.empty()) {
            TCube cur = queue.front();
            queue.pop_front();
            TMove curMove = reached[currentStage.GetImage(cur)];
            for (const auto &move : allowedMoves) {
                TCube c = move.Act(cur);
                TMove m = curMove * move;
                auto img = currentStage.GetImage(c);
                if (reached.find(img) == reached.end()) {
                    int estimate = currentStage.Estimate(c);
                    if (estimate == -1) {
                        //std::cout << "est2=-1" << std::endl;
                        result.clear();
                        return result;
                    }
                    if (m.GetTotalTurnsCount() + estimate < maxTotalTurnsCount && m.GetLastStageTurnsCount() < maxForwardStageTurnsCount && m.GetLastStageTurnsCount() + estimate < maxStageTurnsCount) {
                        reached[img] = m;
                        queue.push_back(c);
                        //std::cout << "f estimate=" << estimate << "\t" << m.GetTotalTurnsCount() << "\t" << m.GetLastStageTurnsCount() << std::endl;
                    }
                    auto it = reachedBackward.find(img);
                    if (it != reachedBackward.end()) {
                        auto solution = m / it->second;
                        auto img = nextStage.GetImage(c);
                        auto it = result.find(img);
                        if (it != result.end()) {
                            if (solution.GetTotalTurnsCount() < it->second.GetTotalTurnsCount())
                                it->second = solution;
                        } else {
                            result[img] = solution;
                        }
                        if (solution.GetTotalTurnsCount() + 1 < maxTotalTurnsCount)
                            maxTotalTurnsCount = solution.GetTotalTurnsCount() + 1;
                        //std::cout << "f " << i << " " << result.size() << " " << solution.GetTotalTurnsCount() << "=" << m.GetTotalTurnsCount() << "+" << solution.GetTotalTurnsCount() - m.GetTotalaTurnsCount() << " " << reached.size() << std::endl;
                        if (result.size() >= candidatesCount) {
                            std::cout << reached.size() << std::endl;
                            return result;
                        }
                    }
                }
            }
        }
    }
    return result;
}

template<typename TCurrentStage, typename TNextStage>
std::vector<TMove> Solve(const TCube &cube,
                         const std::vector<TMove> &doneMoves,
                         size_t candidatesCount, size_t maxTotalTurnsCount, size_t maxForwardStageTurnsCount, size_t maxStageTurnsCount,
                         const TCurrentStage &currentStage, const TNextStage &nextStage) {
    std::vector<TMove> result;
    for (auto it : BFS2(cube, doneMoves, candidatesCount, maxTotalTurnsCount, maxForwardStageTurnsCount, maxStageTurnsCount, currentStage, nextStage))
        result.push_back(it.second);
    std::sort(result.begin(), result.end(), [] (const TMove &a, const TMove &b) {
        return a.GetTotalTurnsCount() < b.GetTotalTurnsCount();
    });
    return result;
}

