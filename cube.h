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
    TE_G1_COUNT,
    TE_L = TE_G1_COUNT,
    TE_L1,
    TE_R,
    TE_R1,
    TE_F,
    TE_F1,
    TE_B,
    TE_B1,
    TE_G0_COUNT
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

    private:
        std::vector<ETurn> Turns;
        unsigned char Permutation[NUM_FIELDS];
        size_t TurnsCount = 0;
};
TMove operator * (TMove lft, const TMove &rgt);
TMove operator / (TMove lft, const TMove &rgt);


const TMove &TurnExt2Move(ETurnExt turn);
std::string TurnExt2String(ETurnExt turn);
std::vector<ETurnExt> Turns2Exts(const std::vector<ETurn> &turns);


template<typename THomomorphism>
bool UpdateReached(const TCube &cube, const TMove &move,
                   const THomomorphism &hom, std::map<typename THomomorphism::TCubeImageType, TMove> &result) {
    auto img = hom.GetImage(cube);
    auto it = result.find(img);
    if (it != result.end()) {
        if (move.GetTurnsCount() < it->second.GetTurnsCount())
            it->second = move;
        return false;
    } else {
        result[img] = move;
        return true;
    }
}

template<typename TCurrentHomomorphism, typename TNextHomomorphism>
std::map<typename TNextHomomorphism::TCubeImageType, TMove> DoSolve(const TCube &from, const TCube &to,
                        const std::vector<TMove> &doneMoves, size_t candidatesCount, size_t maxGraphSize,
                        const std::vector<TMove> &allowedMoves,
                        const TCurrentHomomorphism &currentHomomorphism, const TNextHomomorphism &nextHomomorphism) {
    using TCurrentCubeImage = typename TCurrentHomomorphism::TCubeImageType;
    using TNextCubeImage = typename TNextHomomorphism::TCubeImageType;
    using TCurrentReachedMap = std::map<TCurrentCubeImage, TMove>;
    using TNextReachedMap = std::map<TNextCubeImage, TMove>;
    using TQueue = std::list<TCube>;
    TQueue queueForward, queueBackward;
    queueBackward.push_back(to);
    TCurrentReachedMap reachedForward, reachedBackward;
    TNextReachedMap result;
    reachedBackward[currentHomomorphism.GetImage(to)] = TMove();
    for (size_t i = 0; reachedForward.size() < maxGraphSize &&
                       (i < doneMoves.size() || !queueForward.empty() || !queueBackward.empty()); )
    {
        //std::cout << i << " " << result.size() << " " << (queueForward.empty() ? -1 : static_cast<int>(reachedForward[currentHomomorphism.GetImage(queueForward.front())].GetTurnsCount())) << " " << queueForward.size() << " " << queueBackward.size() << " " << reachedForward.size() << " " << reachedBackward.size() << std::endl;
        for (; i < doneMoves.size() && (queueForward.empty() ||
               doneMoves[i].GetTurnsCount() <= reachedForward[currentHomomorphism.GetImage(queueForward.front())].GetTurnsCount());
               ++i)
        {
            TMove m = doneMoves[i];
            TCube c = m.Act(from);
            auto img = currentHomomorphism.GetImage(c);
            if (img == currentHomomorphism.GetImage(to)) {
                UpdateReached(c, m, nextHomomorphism, result);
                std::cout << "i " << i << " " << result.size() << " " << m.GetTurnsCount() << " " << reachedForward.size() << std::endl;
                if (result.size() >= candidatesCount)
                    return result;
            }
            if (UpdateReached(c, m, currentHomomorphism, reachedForward))
                queueForward.push_front(c);
        }
        if (!queueForward.empty()) {
            TCube cur = queueForward.front();
            queueForward.pop_front();
            const TMove &curMove = reachedForward[currentHomomorphism.GetImage(cur)];
            for (const auto &move : allowedMoves) {
                TCube c = move.Act(cur);
                TMove m = curMove * move;
                if (UpdateReached(c, m, currentHomomorphism, reachedForward))
                    queueForward.push_back(c);
                auto it = reachedBackward.find(currentHomomorphism.GetImage(c));
                if (it != reachedBackward.end()) {
                    m /= it->second;
                    UpdateReached(m.Act(from), m, nextHomomorphism, result);
                    std::cout << "f " << i << " " << result.size() << " " << m.GetTurnsCount() << "=" << m.GetTurnsCount() - it->second.GetTurnsCount() << "+" << it->second.GetTurnsCount() << " " << reachedForward.size() << std::endl;
                    if (result.size() >= candidatesCount)
                        return result;
                }
            }
        }
        if (!queueBackward.empty()) {
            TCube cur = queueBackward.front();
            queueBackward.pop_front();
            const TMove &curMove = reachedBackward[currentHomomorphism.GetImage(cur)];
            for (const auto &move : allowedMoves) {
                TCube c = move.Act(cur);
                TMove m = curMove * move;
                if (UpdateReached(c, m, currentHomomorphism, reachedBackward))
                    queueBackward.push_back(c);
                auto it = reachedForward.find(currentHomomorphism.GetImage(c));
                if (it != reachedForward.end()) {
                    m = it->second / m;
                    UpdateReached(m.Act(from), m, nextHomomorphism, result);
                    std::cout << "b " << i << " " << result.size() << " " << m.GetTurnsCount() << "=" << it->second.GetTurnsCount() << "+" << m.GetTurnsCount() - it->second.GetTurnsCount() << " " << reachedForward.size() << std::endl;
                    if (result.size() >= candidatesCount)
                        return result;
                }
            }
        }
    }
    return result;
}

template<typename TCurrentHomomorphism, typename TNextHomomorphism>
std::vector<TMove> Solve(const TCube &from, const TCube &to,
                         const std::vector<TMove> &doneMoves, size_t candidatesCount, size_t maxGraphSize,
                         const std::vector<TMove> &allowedMoves,
                         const TCurrentHomomorphism &currentHomomorphism, const TNextHomomorphism &nextHomomorphism) {
    std::vector<TMove> result;
    for (auto it : DoSolve(from, to, doneMoves, candidatesCount, maxGraphSize, allowedMoves, currentHomomorphism, nextHomomorphism))
        result.push_back(it.second);
    std::sort(result.begin(), result.end(), [] (const TMove &a, const TMove &b) {
        return a.GetTurnsCount() < b.GetTurnsCount();
    });
    return result;
}

