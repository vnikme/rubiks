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
    //TCube puzzle(MakePuzzle("OWRWGWWYRRORYWRYGOGOYOOGYYWGBYBWBOGGBORRBGBYBBWR"));
    //TCube puzzle(MakePuzzle("YBYYYYOBBROYROWGRWWBGRGWRBGYWRGBOWWBOWROYOBGRGOG"));
    TCube puzzle(MakePuzzle("ygbgbbyr yggworrr wwooygyo ywwgoooy wrwbbbbg brgywrro"));
    //TCube puzzle(MakePuzzle("wrwbgwow ryrbgrwr grgwygog yoybgyry owobgoyo brbywbob"));
    //TCube puzzle(MakePuzzle("yrygbyoy rwrgbryr brbywbob wowgbwrw oyogbowo grgwygog"));
    TG0Homomorphism hom;
    TIsomorphism iso;
    std::vector<TMove> g0 = {TMove::U(), TMove::U() * TMove::U(), TMove::U() * TMove::U() * TMove::U(),
                             TMove::D(), TMove::D() * TMove::D(), TMove::D() * TMove::D() * TMove::D(),
                             TMove::F(), TMove::F() * TMove::F(), TMove::F() * TMove::F() * TMove::F(),
                             TMove::B(), TMove::B() * TMove::B(), TMove::B() * TMove::B() * TMove::B(),
                             TMove::L(), TMove::L() * TMove::L(), TMove::L() * TMove::L() * TMove::L(),
                             TMove::R(), TMove::R() * TMove::R(), TMove::R() * TMove::R() * TMove::R()};
    std::vector<TMove> g1 = {TMove::U(), TMove::U() * TMove::U(), TMove::U() * TMove::U() * TMove::U(),
                             TMove::D(), TMove::D() * TMove::D(), TMove::D() * TMove::D() * TMove::D(),
                             TMove::F() * TMove::F(), TMove::B() * TMove::B(),
                             TMove::L() * TMove::L(), TMove::R() * TMove::R()};
    TMove m = Solve(puzzle, zero, g0, hom);
    std::cout << "First stage moves count: " << PrintableMoves(m.GetTurns()).size() << std::endl;
    puzzle = m.Act(puzzle);
    m *= Solve(puzzle, zero, g1, iso);
    std::cout << "Second stage moves count: " << PrintableMoves(m.GetTurns()).size() << std::endl;
    for (auto t : PrintableMoves(m.GetTurns()))
        std::cout << " " << t;
    std::cout << std::endl;
    return 0;
}

