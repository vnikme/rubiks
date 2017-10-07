#pragma once


#include <cstddef>
#include <vector>
#include <map>
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
        TMove CloneAsOneMove() const;
        TMove &operator *= (const TMove &rgt);
        TMove &operator /= (const TMove &rgt);
        TCube Act(const TCube &cube) const;
        const std::vector<ETurn> &GetTurns() const;
        size_t GetTurnsCount() const;
        bool operator != (const TMove &rgt) const;
        bool operator < (const TMove &rgt) const;

        static TMove &F();
        static TMove &F2();
        static TMove &F1();
        static TMove &U();
        static TMove &U2();
        static TMove &U1();
        static TMove &R();
        static TMove &R2();
        static TMove &R1();
        static TMove &B();
        static TMove &B2();
        static TMove &B1();
        static TMove &D();
        static TMove &D2();
        static TMove &D1();
        static TMove &L();
        static TMove &L2();
        static TMove &L1();

    private:
        std::vector<ETurn> Turns;
        unsigned char Permutation[NUM_FIELDS];
        size_t TurnsCount = 0;
};
TMove operator * (TMove lft, const TMove &rgt);
TMove operator / (TMove lft, const TMove &rgt);


std::vector<std::string> PrintableMoves(const std::vector<ETurn> &turns);


class TMoveChain {
    public:
        TMoveChain();
        void AddMove(const TMove &move);
        TMoveChain &operator *= (const TMove &rgt);
        TMoveChain &operator /= (const TMove &rgt);
        bool operator < (const TMoveChain &rgt) const;
        TMove ToMove() const;
        size_t GetTurnsCount() const;

    private:
        std::vector<TMove> Moves;
};
TMoveChain operator * (TMoveChain lft, const TMove &rgt);
TMoveChain operator / (TMoveChain lft, const TMove &rgt);


template<typename THomomorphism>
std::set<TMoveChain> DoSolve(const TCube &from, const TCube &to,
                             const std::vector<TMoveChain> &doneMoves, size_t candidatesCount,
                             const std::vector<TMove> &allowedMoves, const THomomorphism &hom) {
    using TCubeImage = typename THomomorphism::TCubeImageType;
    using TQueue = std::list<TCube>;
    std::set<TMoveChain> result;
    TQueue queueForward, queueBackward;
    queueBackward.push_back(to);
    std::map<TCubeImage, TMoveChain> reachedForward;
    std::map<TCubeImage, TMove> reachedBackward;
    reachedBackward[hom.GetImage(to)] = TMove();
    for (size_t i = 0; i < doneMoves.size() || !queueForward.empty() || !queueBackward.empty(); ) {
        std::cout << i << " " << result.size() << " " << queueForward.size() << " " << queueBackward.size() << " " << reachedForward.size() << " " << reachedBackward.size() << std::endl;
        for (; i < doneMoves.size() && (queueForward.empty() ||
               doneMoves[i].GetTurnsCount() <= reachedForward[hom.GetImage(queueForward.front())].GetTurnsCount());
               ++i)
        {
            TCube c = doneMoves[i].ToMove().Act(from);
            TMoveChain chain(doneMoves[i]);
            chain.AddMove(TMove());
            if (hom.GetImage(c) == hom.GetImage(to)) {
                result.insert(chain);
                if (result.size() >= candidatesCount)
                    return result;
            }
            queueForward.push_front(c);
            reachedForward[hom.GetImage(c)] = chain;
        }
        if (!queueForward.empty()) {
            TCube cur = queueForward.front();
            queueForward.pop_front();
            const TMoveChain &curMove = reachedForward[hom.GetImage(cur)];
            for (const auto &move : allowedMoves) {
                TCube c = move.Act(cur);
                TCubeImage img = hom.GetImage(c);
                if (reachedForward.find(img) != reachedForward.end())
                    continue;
                TMoveChain m = curMove * move;
                reachedForward[img] = m;
                queueForward.push_back(c);
                auto it = reachedBackward.find(img);
                if (it != reachedBackward.end()) {
                    result.insert(m / it->second);
                    if (result.size() >= candidatesCount)
                        return result;
                }
            }
        }
        if (!queueBackward.empty()) {
            TCube cur = queueBackward.front();
            queueBackward.pop_front();
            const TMove &curMove = reachedBackward[hom.GetImage(cur)];
            for (const auto &move : allowedMoves) {
                TCube c = move.Act(cur);
                TCubeImage img = hom.GetImage(c);
                if (reachedBackward.find(img) != reachedBackward.end())
                    continue;
                TMove m = curMove * move;
                reachedBackward[img] = m;
                queueBackward.push_back(c);
                auto it = reachedForward.find(img);
                if (it != reachedForward.end()) {
                    result.insert(it->second / m);
                    if (result.size() >= candidatesCount)
                        return result;
                }
            }
        }
    }
    throw std::logic_error("Unsolvable cube!");
}

template<typename THomomorphism>
std::vector<TMoveChain> Solve(const TCube &from, const TCube &to, const std::vector<TMoveChain> &doneMoves, size_t candidatesCount, const std::vector<TMove> &allowedMoves, const THomomorphism &hom) {
    auto solution = DoSolve(from, to, doneMoves, candidatesCount, allowedMoves, hom);
    std::vector<TMoveChain> result(solution.begin(), solution.end());
    std::sort(result.begin(), result.end(), [] (const TMoveChain &a, const TMoveChain &b) {
        return a.GetTurnsCount() < b.GetTurnsCount();
    });
    return result;
}

