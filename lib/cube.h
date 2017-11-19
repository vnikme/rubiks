#pragma once


#include <cstddef>
#include <vector>
#include <unordered_map>
#include <set>
#include <list>
#include <exception>
#include <string>
#include <algorithm>
#include <iostream>


enum EColor {
    C_WHITE,
    C_RED,
    C_GREEN,
    C_YELLOW,
    C_ORANGE,
    C_BLUE
};


enum ETurn {
    T_FRONT,
    T_UP,
    T_RIGHT,
    T_BACK,
    T_DOWN,
    T_LEFT
};

enum ETurnExt {
    TE_U,
    TE_U2,
    TE_U1,
    TE_D,
    TE_D2,
    TE_D1,
    TE_L2,
    TE_R2,
    TE_F2,
    TE_B2,
    TE_L,
    TE_L1,
    TE_R,
    TE_R1,
    TE_F,
    TE_F1,
    TE_B,
    TE_B1
};


template<size_t N>
struct TCubeImage {
    unsigned char Data[N];
    bool operator < (const TCubeImage &rgt) const {
        for (size_t i = 0; i < N; ++i)
            if (Data[i] != rgt.Data[i])
                return (Data[i] < rgt.Data[i]);
        return false;
    }
    bool operator == (const TCubeImage &rgt) const {
        return (!(*this < rgt) && !(rgt < *this));
    }
};


namespace std {
    template<size_t N>
    struct hash<TCubeImage<N>> {
        typedef TCubeImage<N> argument_type;
        typedef size_t result_type;
        result_type operator () (const argument_type &obj) const noexcept {
            return std::hash<std::string>()(std::string(obj.Data, obj.Data + N));
        }
    };
} // namespace std


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

class TCube {
    public:
        static constexpr size_t NUM_FIELDS = 48;
        static constexpr size_t BITS_FOR_COLORS = 3;

        EColor GetColor(size_t field) const;
        void SetColor(size_t field, EColor color);
        TCubeImage<(NUM_FIELDS * BITS_FOR_COLORS + 7) / 8> GetImage() const;
        static const std::vector<size_t> &GetAllCorners();
        static const std::vector<size_t> &GetAllEdges();
        static const std::vector<size_t> &GetTopBottomEdges();
        static const std::vector<size_t> &GetMiddleEdges();
        static size_t GetOppositeEdge(size_t field);
        bool operator == (const TCube &rgt) const;
        bool operator != (const TCube &rgt) const;

    private:
        unsigned char Data[(NUM_FIELDS * BITS_FOR_COLORS + 7) / 8];

        size_t GetBit(size_t bit) const;
        void SetBit(size_t bit, size_t value);
};



class TMove {
    public:
        static constexpr size_t NUM_FIELDS = TCube::NUM_FIELDS;

        TMove();
        TMove(ETurn id, const size_t permutation[NUM_FIELDS]);
        TMove(ETurn id, const std::vector<std::vector<size_t>> &cycles);    // Construct from independent cycles.
        TMove CloneAsOneMove() const;
        TMove &operator *= (const TMove &rgt);
        TMove &operator /= (const TMove &rgt);
        TCube Act(const TCube &cube) const;
        std::vector<ETurn> GetTurns() const;
        size_t GetTurnsCount() const;
        bool operator != (const TMove &rgt) const;
        bool operator < (const TMove &rgt) const;

    private:
        std::vector<bool> Turns;
        unsigned char Permutation[NUM_FIELDS];
        size_t TurnsCount = 0;

        void AddTurn(ETurn turn);
};
TMove operator * (TMove lft, const TMove &rgt);
TMove operator / (TMove lft, const TMove &rgt);


const TMove &TurnExt2Move(ETurnExt turn);
std::string TurnExt2String(ETurnExt turn);
std::vector<ETurnExt> Turns2Exts(const std::vector<ETurn> &turns);


TCube MakePuzzle(std::string colors);
TCube MakeSolvedCube();

