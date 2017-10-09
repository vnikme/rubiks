#include <iostream>
#include <algorithm>
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


EColor Char2Color(char ch) {
    if (ch == 'W' || ch == 'w')
        return C_WHITE;
    if (ch == 'Y' || ch == 'y')
        return C_YELLOW;
    if (ch == 'R' || ch == 'r')
        return C_RED;
    if (ch == 'O' || ch == 'o')
        return C_ORANGE;
    if (ch == 'G' || ch == 'g')
        return C_GREEN;
    if (ch == 'B' || ch == 'b')
        return C_BLUE;
    throw std::logic_error("Unknown color");
}

TCube MakePuzzle(std::string colors) {
    colors.erase(std::remove(colors.begin(), colors.end(), ' '), colors.end());
    std::cout << colors.size() << std::endl;
    TCube cube;
    for (size_t i = 0; i < 48; ++i)
        cube.SetColor(i, Char2Color(colors[i]));
    return cube;
}

/*

          24 25 26
          27  B 28
          29 30 31

           8  9 10
          11  U 12
          13 14 15

40 41 42   0  1  2  16 17 18
43  L 44   3  F  4  19  R 20
45 46 47   5  6  7  21 22 23

          32 33 34
          35  D 36
          37 38 39

*/

int main() {
    TCube zero;
    for (size_t i = 0; i < TCube::NUM_FIELDS; ++i)
        zero.SetColor(i, static_cast<EColor>(i / 8));
    //TCube puzzle(MakePuzzle("ooyorwyb gobbwyyg rrrboobb rywgwwby bgywywro owgrgggr"));
    //TCube puzzle(MakePuzzle("OWRWGWWYRRORYWRYGOGOYOOGYYWGBYBWBOGGBORRBGBYBBWR"));
    TCube puzzle(MakePuzzle("YBYYYYOBBROYROWGRWWBGRGWRBGYWRGBOWWBOWROYOBGRGOG"));
    //TCube puzzle(MakePuzzle("ygbgbbyr yggworrr wwooygyo ywwgoooy wrwbbbbg brgywrro"));
    //TCube puzzle(MakePuzzle("wrwbgwow ryrbgrwr grgwygog yoybgyry owobgoyo brbywbob"));
    //TCube puzzle(MakePuzzle("yrygbyoy rwrgbryr brbywbob wowgbwrw oyogbowo grgwygog"));
    //TCube puzzle(MakePuzzle("oroygoro bobyybgb wbwoywbw rwroorbr gbgwwgrg ygywrygy"));
    //TCube puzzle(MakePuzzle("bwrygwro rbbbbroy gryygggr bggbwboo gwyyroow wwyyrwoo"));
    //TCube puzzle(MakePuzzle("bygyywwb bbwobyby ooogrywr wrywwwrg rorggbyg rgoboorg"));
    //TCube puzzle(MakePuzzle("bwrygywr wgwbywby gogyrbww oboygoor boyrgyrb gorrbgwo"));
    //TCube puzzle(MakePuzzle("ybywoggr rbbryowr bgwyrgbg orogygor ywwbrwgw ywboobyo"));
    //TCube puzzle(MakePuzzle("rbobrgoo oyrobyrg ywwwgygr rywgwwrg oybyrggb bwbooybw"));
    //TCube puzzle(zero);
    TG0Homomorphism hom;
    TIsomorphism iso;
    std::vector<TMove> g0, g1;
    for (size_t i = 0; i < TE_G0_COUNT; ++i)
        g0.push_back(TurnExt2Move(static_cast<ETurnExt>(i)));
    for (size_t i = 0; i < TE_G1_COUNT; ++i)
        g1.push_back(TurnExt2Move(static_cast<ETurnExt>(i)));
    std::vector<TMove> candidates = Solve(puzzle, zero, {TMove()}, 10, 3000000, g0, hom, iso);
    //for (const auto &m : candidates) {
    //    std::cout << "First stage moves count: " << PrintableMoves(m.GetTurns()).size() << std::endl;
    //}
    auto solution = Solve(puzzle, zero, candidates, 1, 3000000, g1, iso, iso);
    for (const auto &m : solution) {
        auto turns = Turns2Exts(m.GetTurns());
        std::cout << "Second stage moves count: " << turns.size() << " " << m.GetTurnsCount() << " " << m.GetTurns().size() << std::endl;
        for (auto t : turns)
            std::cout << " " << TurnExt2String(t);
        std::cout << std::endl;
        //turns = ReduceSolution(puzzle, turns, g0, iso);
        //std::cout << "Reduced second stage moves count: " << turns.size() << std::endl;
        //for (auto t : turns)
        //    std::cout << " " << TurnExt2String(t);
        //std::cout << std::endl;
    }
    return 0;
}

