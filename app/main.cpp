#include <algorithm>
#include <iostream>

#include <cube.h>
#include <kociemba.h>


bool RunTest(const TCube &puzzle, const TCube &target) {
    auto solution = KociembaSolution(puzzle, target);
    std::cout << "Answer contains " << solution.size() << " moves" << std::endl;
    TMove move;
    for (ETurnExt turn : solution) {
        move *= TurnExt2Move(turn);
        std::cout << " " << TurnExt2String(turn);
    }
    std::cout << std::endl;
    return (move.Act(puzzle) == target);
}

void RunTests() {
    std::vector<std::string> tests = {
        "ooyorwyb gobbwyyg rrrboobb rywgwwby bgywywro owgrgggr",
        "OWRWGWWYRRORYWRYGOGOYOOGYYWGBYBWBOGGBORRBGBYBBWR",
        "YBYYYYOBBROYROWGRWWBGRGWRBGYWRGBOWWBOWROYOBGRGOG",
        "ygbgbbyr yggworrr wwooygyo ywwgoooy wrwbbbbg brgywrro",
        "wrwbgwow ryrbgrwr grgwygog yoybgyry owobgoyo brbywbob",
        "yrygbyoy rwrgbryr brbywbob wowgbwrw oyogbowo grgwygog",
        "oroygoro bobyybgb wbwoywbw rwroorbr gbgwwgrg ygywrygy",
        "bwrygwro rbbbbroy gryygggr bggbwboo gwyyroow wwyyrwoo",
        "bygyywwb bbwobyby ooogrywr wrywwwrg rorggbyg rgoboorg",
        "bwrygywr wgwbywby gogyrbww oboygoor boyrgyrb gorrbgwo",
        "ybywoggr rbbryowr bgwyrgbg orogygor ywwbrwgw ywboobyo",
        "rbobrgoo oyrobyrg ywwwgygr rywgwwrg oybyrggb bwbooybw"
    };
    TCube zero = MakeSolvedCube();
    for (const auto &test : tests) {
        std::cout << "Testing " << test << std::endl;
        TCube puzzle = MakePuzzle(test);
        bool result = RunTest(puzzle, zero);
        std::cout << "Solution is " << (!result ? "NOT " : "") << "correct" << std::endl << std::endl;
    }
}


int main() {
    RunTests();
    return 0;
}

