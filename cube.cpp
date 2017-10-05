#include "cube.h"


// TCube
EColor TCube::GetColor(size_t field) const {
    size_t result = 0;
    for (size_t i = 0; i < BITS_FOR_COLORS; ++i)
        result |= (GetBit(field * BITS_FOR_COLORS + i) << i);
    return static_cast<EColor>(result);
}

void TCube::SetColor(size_t field, EColor color) {
    for (size_t i = 0; i < BITS_FOR_COLORS; ++i)
        SetBit(field * BITS_FOR_COLORS + i, (color & (1 << i)) ? 1 : 0);
}

TCubeImage<(TCube::NUM_FIELDS * TCube::BITS_FOR_COLORS + 7) / 8> TCube::GetImage() const {
    TCubeImage<(NUM_FIELDS * BITS_FOR_COLORS + 7) / 8> result;
    for (size_t i = 0; i < (NUM_FIELDS * BITS_FOR_COLORS + 7) / 8; ++i)
        result.Data[i] = Data[i];
    return result;
}

const std::vector<size_t> &TCube::GetAllCorners() {
    static std::vector<size_t> Corners = {24, 26, 29, 31, 8, 10, 13, 15, 40, 42, 45, 47, 0, 2, 5, 7, 16, 18, 21, 23, 32, 34, 37, 39};
    return Corners;
}

const std::vector<size_t> &TCube::GetAllEdges() {
    static std::vector<size_t> Edges = {25, 27, 28, 30, 9, 11, 12, 14, 41, 43, 44, 46, 1, 3, 4, 6, 17, 19, 20, 22, 33, 35, 36, 38};
    return Edges;
}

size_t TCube::GetOppositeEdge(size_t field) {
    static std::map<size_t, size_t> EdgesMap = {
        {25, 38}, {38, 25}, {27, 43}, {43, 27},
        {28, 20}, {20, 28}, {30, 9}, {9, 30},
        {11, 41}, {41, 11}, {12, 17}, {17, 12},
        {14, 1}, {1, 14}, {3, 44}, {44, 3},
        {4, 19}, {19, 4}, {6, 33}, {33, 6},
        {35, 46}, {46, 35}, {36, 22}, {22, 36}
    };
    auto it = EdgesMap.find(field);
    if (it == EdgesMap.end())
        throw std::logic_error("Can't get opposite edge");
    return it->second;
}

size_t TCube::GetBit(size_t bit) const {
    return (Data[bit / 8] & (1 << (bit % 8))) ? 1 : 0;
}

void TCube::SetBit(size_t bit, size_t value) {
    if (value)
        Data[bit / 8] |= (1 << (bit % 8));
    else
        Data[bit / 8] &= ~(1 << (bit % 8));
}


// TMove
TMove::TMove() {
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = i;
}

TMove::TMove(ETurn id, const size_t permutation[NUM_FIELDS]) {
    Turns.push_back(id);
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
}

TMove::TMove(ETurn id, const std::vector<std::vector<size_t>> &cycles) {
    Turns.push_back(id);
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = i;
    for (const auto &cycle : cycles) {
        size_t n = cycle.size();
        for (size_t i = 0; i < n; ++i) {
            size_t from = cycle[i], to = i + 1 < n ? cycle[i + 1] : cycle[0];
            Permutation[from] = to;
        }
    }
}

TMove &TMove::operator *= (const TMove &rgt) {
    std::vector<ETurn> turns(std::move(Turns));
    turns.insert(turns.end(), rgt.Turns.begin(), rgt.Turns.end());
    Turns.swap(turns);
    size_t permutation[NUM_FIELDS];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        permutation[i] = rgt.Permutation[Permutation[i]];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
    return *this;
}

TMove &TMove::operator /= (const TMove &rgt) {
    std::vector<ETurn> turns(std::move(Turns));
    Turns.clear();
    Turns.reserve(rgt.Turns.size() * 3);
    for (auto turn : rgt.Turns) {
        for (size_t i = 0; i < 3; ++i)
            Turns.push_back(turn);
    }
    turns.insert(turns.end(), Turns.rbegin(), Turns.rend());
    Turns.swap(turns);
    size_t permutation[NUM_FIELDS], inv[NUM_FIELDS];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        inv[rgt.Permutation[i]] = i;
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        permutation[i] = inv[Permutation[i]];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
    return *this;
}

TMove operator * (TMove lft, const TMove &rgt) {
    lft *= rgt;
    return lft;
}

TMove operator / (TMove lft, const TMove &rgt) {
    lft /= rgt;
    return lft;
}

TCube TMove::Act(const TCube &cube) const {
    TCube result;
    for (size_t i = 0; i < NUM_FIELDS; ++i) {
        EColor color = cube.GetColor(i);
        result.SetColor(Permutation[i], color);
    }
    return result;
}

const std::vector<ETurn> &TMove::GetTurns() const {
    return Turns;
}

TMove &TMove::F() {
    static TMove FMove(T_FRONT, {{0, 2, 7, 5}, {1, 4, 6, 3}, {13, 16, 34, 47}, {14, 19, 33, 44}, {15, 21, 32, 42}});
    return FMove;
}

TMove &TMove::U() {
    static TMove FMove(T_UP, {{8, 10, 15, 13}, {9, 12, 14, 11}, {29, 18, 2, 42}, {30, 17, 1, 41}, {31, 16, 0, 40}});
    return FMove;
}

TMove &TMove::R() {
    static TMove FMove(T_RIGHT, {{16, 18, 23, 21}, {17, 20, 22, 19}, {15, 31, 39, 7}, {12, 28, 36, 4}, {10, 26, 34, 2}});
    return FMove;
}

TMove &TMove::B() {
    static TMove FMove(T_BACK, {{29, 24, 26, 31}, {27, 25, 28, 30}, {8, 45, 39, 18}, {9, 43, 38, 20}, {10, 40, 37, 23}});
    return FMove;
}

TMove &TMove::D() {
    static TMove FMove(T_DOWN, {{32, 34, 39, 37}, {33, 36, 38, 35}, {5, 21, 26, 45}, {6, 22, 25, 46}, {7, 23, 24, 47}});
    return FMove;
}

TMove &TMove::L() {
    static TMove FMove(T_LEFT, {{40, 42, 47, 45}, {41, 44, 46, 43}, {8, 0, 32, 24}, {11, 3, 35, 27}, {13, 5, 37, 29}});
    return FMove;

}


std::vector<std::string> PrintableMoves(const std::vector<ETurn> &turns) {
    static const char Symbols[] = "FURBDL";
    std::vector<std::string> result;
    for (size_t i = 0; i < turns.size(); ) {
        size_t j = i + 1;
        while (j < turns.size() && j - i < 4 && turns[j] == turns[i])
            ++j;
        if (j - i == 4) {
            i = j;
            continue;
        }
        result.emplace_back();
        std::string &last = result.back();
        last += Symbols[turns[i]];
        if (j - i == 2)
            last += "2";
        if (j - i == 3)
            last += "'";
        i = j;
    }
    return result;
}

