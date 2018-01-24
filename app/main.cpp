#include <algorithm>
#include <iostream>

#include <cube.h>
#include <kociemba.h>


bool RunTest(const TCube &puzzle) {
    std::vector<ETurnExt> solution;
    if (!KociembaSolution(puzzle, solution))
        return false;
    std::cout << "Answer contains " << solution.size() << " moves" << std::endl << "Solution is:";
    TMove move;
    for (ETurnExt turn : solution) {
        move *= TurnExt2Move(turn);
        std::cout << " " << TurnExt2String(turn);
    }
    std::cout << std::endl;
    return (move.Act(puzzle) == MakeSolvedCube());
}

void RunTests() {
    std::vector<std::string> tests = {
        "ooooooooyyyyyyyybbbbbbbbrrrrrrrrwwwwwwwwgggggggg",
        "wwwwwwww wwwwwwww wwwwwwww wwwwwwww wwwwwwww wwwwwwww",
        "wwwwwwww rrrrrrrr gggggggg yyyyyyyy oooooooo bbbbbbbb",
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
    size_t ok = 0, total = 0;
    for (const auto &test : tests) {
        std::cout << "Testing " << test << std::endl;
        TCube puzzle = MakePuzzle(test);
        bool result = RunTest(puzzle);
        std::cout << "Solution is " << (!result ? "NOT " : "") << "correct" << std::endl << std::endl;
        if (result)
            ++ok;
        ++total;
    }
    std::cout << ok << "/" << total << " tests are good" << std::endl;
}


int main() {
    InitKociemba();
    RunTests();
    return 0;
}

