#include "cube.h"
#include <map>
#include <sstream>


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
    // r/o, w/y, g/b
    static std::vector<size_t> Corners = { 13, 0, 42,  15, 2, 16,  8, 29, 40,  10, 31, 18,
                                           32, 5, 47,  34, 7, 21,  37, 24, 45,  39, 26, 23 };
    return Corners;
}

const std::vector<size_t> &TCube::GetAllEdges() {
    static std::vector<size_t> Edges = { 14, 1,  9, 30,  33, 6,  38, 25,
                                         11, 41,  12, 17,  35, 46,  36, 22,
                                         44, 3,  19, 4,  43, 27,  20, 28 };
    return Edges;
}

const std::vector<size_t> &TCube::GetTopBottomEdges() {
    static std::vector<size_t> Edges = { 14, 1,  9, 30,  33, 6,  38, 25,
                                         11, 41,  12, 17,  35, 46,  36, 22, };
    return Edges;
}

const std::vector<size_t> &TCube::GetMiddleEdges() {
    static std::vector<size_t> Edges = { 44, 3,  19, 4,  43, 27,  20, 28 };
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

bool TCube::operator == (const TCube &rgt) const {
    for (size_t i = 0; i < sizeof(Data) / sizeof(*Data); ++i)
        if (Data[i] != rgt.Data[i])
            return false;
    return true;
}

bool TCube::operator != (const TCube &rgt) const {
    return !(*this == rgt);
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
TMove::TMove()
    : TurnsCount(0)
{
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = i;
}

TMove::TMove(ETurn id, const size_t permutation[NUM_FIELDS])
    : TurnsCount(1)
{
    AddTurn(id);
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
}

TMove::TMove(ETurn id, const std::vector<std::vector<size_t>> &cycles)
    : TurnsCount(1)
{
    AddTurn(id);
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

TMove TMove::CloneAsOneMove() const {
    TMove result(*this);
    result.TurnsCount = 1;
    return result;
}

TMove &TMove::operator *= (const TMove &rgt) {
    std::vector<bool> turns(std::move(Turns));
    turns.insert(turns.end(), rgt.Turns.begin(), rgt.Turns.end());
    Turns.swap(turns);
    unsigned char permutation[NUM_FIELDS];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        permutation[i] = rgt.Permutation[Permutation[i]];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
    TurnsCount += rgt.TurnsCount;
    return *this;
}

TMove &TMove::operator /= (const TMove &rgt) {
    std::vector<bool> turns(std::move(Turns));
    Turns.clear();
    for (size_t i = 0; i < rgt.Turns.size(); i += 3) {
        for (size_t j = 0; j < 3; ++j) {                    // 3 == -1, so repeat 3 times
            for (size_t k = 0; k < 3; ++k) {                // 3 == number of bits in ETurn representation
                Turns.push_back(rgt.Turns[i + 2 - k]);
            }
        }
    }
    turns.insert(turns.end(), Turns.rbegin(), Turns.rend());
    Turns.swap(turns);
    unsigned char permutation[NUM_FIELDS], inv[NUM_FIELDS];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        inv[rgt.Permutation[i]] = i;
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        permutation[i] = inv[Permutation[i]];
    for (size_t i = 0; i < NUM_FIELDS; ++i)
        Permutation[i] = permutation[i];
    TurnsCount += rgt.TurnsCount;
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

std::vector<ETurn> TMove::GetTurns() const {
    std::vector<ETurn> result;
    for (size_t i = 0; i < Turns.size(); i += 3) {
        int turn = 0;
        for (size_t j = 0; j < 3; ++j) {
            turn *= 2;
            if (Turns[i + j]) {
                turn += 1;
            }
        }
        result.push_back(static_cast<ETurn>(turn));
    }
    return result;
}

void TMove::AddTurn(ETurn turn) {
    for (size_t i = 0; i < 3; ++i) {
        Turns.push_back((turn & (1 << (2 - i))) ? true : false);
    }
}

size_t TMove::GetTurnsCount() const {
    return TurnsCount;
}

bool TMove::operator != (const TMove &rgt) const {
    for (size_t i = 0; i < NUM_FIELDS; ++i) {
        if (Permutation[i] != rgt.Permutation[i])
            return true;
    }
    return false;
}

bool TMove::operator < (const TMove &rgt) const {
    for (size_t i = 0; i < NUM_FIELDS; ++i) {
        if (Permutation[i] != rgt.Permutation[i])
            return Permutation[i] < rgt.Permutation[i];
    }
    return false;
}

static std::vector<TMove> CreateAllMoves() {
    std::vector<TMove> moves;
    moves.reserve(18);
    TMove f(T_FRONT, {{0, 2, 7, 5}, {1, 4, 6, 3}, {13, 16, 34, 47}, {14, 19, 33, 44}, {15, 21, 32, 42}});
    TMove u(T_UP, {{8, 10, 15, 13}, {9, 12, 14, 11}, {29, 18, 2, 42}, {30, 17, 1, 41}, {31, 16, 0, 40}});
    TMove r(T_RIGHT, {{16, 18, 23, 21}, {17, 20, 22, 19}, {15, 31, 39, 7}, {12, 28, 36, 4}, {10, 26, 34, 2}});
    TMove b(T_BACK, {{29, 24, 26, 31}, {27, 25, 28, 30}, {8, 45, 39, 18}, {9, 43, 38, 20}, {10, 40, 37, 23}});
    TMove d(T_DOWN, {{32, 34, 39, 37}, {33, 36, 38, 35}, {5, 21, 26, 45}, {6, 22, 25, 46}, {7, 23, 24, 47}});
    TMove l(T_LEFT, {{40, 42, 47, 45}, {41, 44, 46, 43}, {8, 0, 32, 24}, {11, 3, 35, 27}, {13, 5, 37, 29}});
    moves.push_back(u);
    moves.push_back((u * u).CloneAsOneMove());
    moves.push_back((u * u * u).CloneAsOneMove());
    moves.push_back(d);
    moves.push_back((d * d).CloneAsOneMove());
    moves.push_back((d * d * d).CloneAsOneMove());
    moves.push_back((l * l).CloneAsOneMove());
    moves.push_back((r * r).CloneAsOneMove());
    moves.push_back((f * f).CloneAsOneMove());
    moves.push_back((b * b).CloneAsOneMove());
    moves.push_back(l);
    moves.push_back((l * l * l).CloneAsOneMove());
    moves.push_back(r);
    moves.push_back((r * r * r).CloneAsOneMove());
    moves.push_back(f);
    moves.push_back((f * f * f).CloneAsOneMove());
    moves.push_back(b);
    moves.push_back((b * b * b).CloneAsOneMove());
    return moves;
}

const TMove &TurnExt2Move(ETurnExt turn) {
    static std::vector<TMove> Moves(CreateAllMoves());
    if (turn < 0 || turn >= Moves.size())
        throw std::logic_error("Turn is out of bounds.");
    return Moves[turn];
}

std::string TurnExt2String(ETurnExt turn) {
    static std::string Ids[] = { "U", "U2", "U'", "D", "D2", "D'", "L2", "R2", "F2", "B2", "L", "L'", "R", "R'", "F", "F'", "B", "B'" };
    if (turn < 0 || turn >= sizeof(Ids) / sizeof(*Ids))
        throw std::logic_error("Turn is out of bounds.");
    return Ids[turn];
}

static ETurnExt Turn2Ext(ETurn turn, size_t count) {
    if (turn == T_UP)
        return static_cast<ETurnExt>(TE_U + count - 1);
    if (turn == T_DOWN)
        return static_cast<ETurnExt>(TE_D + count - 1);
    if (turn == T_LEFT)
        return static_cast<ETurnExt>(count == 2 ? TE_L2 : count == 1 ? TE_L : TE_L1);
    if (turn == T_RIGHT)
        return static_cast<ETurnExt>(count == 2 ? TE_R2 : count == 1 ? TE_R : TE_R1);
    if (turn == T_FRONT)
        return static_cast<ETurnExt>(count == 2 ? TE_F2 : count == 1 ? TE_F : TE_F1);
    if (turn == T_BACK)
        return static_cast<ETurnExt>(count == 2 ? TE_B2 : count == 1 ? TE_B : TE_B1);
    throw std::logic_error("Undefined turn id.");
}

std::vector<ETurnExt> Turns2Exts(const std::vector<ETurn> &turns) {
    std::vector<ETurnExt> result;
    for (size_t i = 0; i < turns.size(); ) {
        size_t j = i + 1;
        while (j < turns.size() && j - i < 4 && turns[j] == turns[i])
            ++j;
        if (j - i == 4) {
            i = j;
            continue;
        }
        result.push_back(Turn2Ext(turns[i], j - i));
        i = j;
    }
    return result;
}


const std::map<char, EColor> char2color = {
    {'w', C_WHITE},
    {'y', C_YELLOW},
    {'r', C_RED},
    {'o', C_ORANGE},
    {'g', C_GREEN},
    {'b', C_BLUE},
};

TCube MakePuzzle(std::string colors) {
    colors.erase(std::remove(colors.begin(), colors.end(), ' '), colors.end());
    //std::cout << colors.size() << std::endl;
    TCube cube;
    for (size_t i = 0; i < 48; ++i) {
        char clr = std::tolower(colors[i]);
        auto it = char2color.find(clr);
        if (it == char2color.cend()) {
            std::stringstream err;
            err << "Unknown color: '" << clr << "'";
            throw std::logic_error(err.str());
        }
        cube.SetColor(i, it->second);
    }
    return cube;
}

TCube MakeSolvedCube() {
    TCube zero;
    for (size_t i = 0; i < TCube::NUM_FIELDS; ++i)
        zero.SetColor(i, static_cast<EColor>(i / 8));
    return zero;
}

