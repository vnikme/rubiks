#pragma once


#include <cstddef>
#include <vector>
#include <map>
#include <list>
#include <exception>
#include <string>
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
        static size_t GetOppositeEdge(size_t field);

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
        TMove &operator *= (const TMove &rgt);
        TMove &operator /= (const TMove &rgt);
        TCube Act(const TCube &cube) const;
        const std::vector<ETurn> &GetTurns() const;

        static TMove &F();
        static TMove &U();
        static TMove &R();
        static TMove &B();
        static TMove &D();
        static TMove &L();

    private:
        std::vector<ETurn> Turns;
        size_t Permutation[NUM_FIELDS];
};
TMove operator * (TMove lft, const TMove &rgt);
TMove operator / (TMove lft, const TMove &rgt);


std::vector<std::string> PrintableMoves(const std::vector<ETurn> &turns);


template<typename THomomorphism>
TMove Solve(const TCube &from, const TCube &to, const std::vector<TMove> &moves, const THomomorphism &hom) {
    using TCubeImage = typename THomomorphism::TCubeImageType;
    using TReachMap = std::map<TCubeImage, TMove>;
    using TQueue = std::list<TCube>;
    if (hom.GetImage(from) == hom.GetImage(to))
        return TMove();             // The cube is already solved
    TQueue queueForward, queueBackward;
    queueForward.push_back(from);
    queueBackward.push_back(to);
    TReachMap reachedForward, reachedBackward;
    reachedForward[hom.GetImage(from)] = TMove();
    reachedBackward[hom.GetImage(to)] = TMove();
    while (!queueForward.empty() || !queueBackward.empty()) {
        if (!queueForward.empty()) {
            TCube cur = queueForward.front();
            queueForward.pop_front();
            const TMove &curMove = reachedForward[hom.GetImage(cur)];
            for (const auto &move : moves) {
                TCube c = move.Act(cur);
                TCubeImage img = hom.GetImage(c);
                if (reachedForward.find(img) != reachedForward.end())
                    continue;
                TMove m = curMove * move;
                auto it = reachedBackward.find(img);
                if (it != reachedBackward.end()) {
                    std::cout << "Found on forward move, fwd=" << reachedForward.size() << ", bwd=" << reachedBackward.size() << std::endl;
                    return m / it->second;
                }
                reachedForward[img] = m;
                queueForward.push_back(c);
            }
        }
        if (!queueBackward.empty()) {
            TCube cur = queueBackward.front();
            queueBackward.pop_front();
            const TMove &curMove = reachedBackward[hom.GetImage(cur)];
            for (const auto &move : moves) {
                TCube c = move.Act(cur);
                TCubeImage img = hom.GetImage(c);
                if (reachedBackward.find(img) != reachedBackward.end())
                    continue;
                TMove m = curMove * move;
                auto it = reachedForward.find(img);
                if (it != reachedForward.end()) {
                    std::cout << "Found on backward move, fwd=" << reachedForward.size() << ", bwd=" << reachedBackward.size() << std::endl;
                    return it->second / m;
                }
                reachedBackward[img] = m;
                queueBackward.push_back(c);
            }
        }
    }
    throw std::logic_error("Unsolvable cube!");
}

