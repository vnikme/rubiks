#pragma once

#include "cube.h"
#include <boost/noncopyable.hpp>
#include <unordered_map>


class TG0Stage : private boost::noncopyable {
    public:
        using TCubeImageType = TCubeImage<9>;
        using TReachedPositions = std::unordered_map<TCubeImageType, TMove>;

        static TG0Stage &Instance();

        TCubeImageType GetImage(const TCube &cube) const;
        const std::vector<TMove> &GetAllowedMoves() const;
        const TReachedPositions &GetReachedPositions() const;

        int Estimate(const TCube &cube) const;

    private:
        std::vector<TMove> AllowedMoves;
        TReachedPositions ReachedPositions;

        TG0Stage();
        ~TG0Stage();

        void Init();
        void FillAllowedMoves();
        void FillReachedPositions();
};


class TG1Stage : private boost::noncopyable {
    public:
        using TCubeImageType = TCubeImage<5>;
        using TReachedPositions = std::unordered_map<TCubeImageType, TMove>;

        static TG1Stage &Instance();

        TCubeImageType GetImage(const TCube &cube) const;
        const std::vector<TMove> &GetAllowedMoves() const;
        const TReachedPositions &GetReachedPositions() const;

        int Estimate(const TCube &cube) const;

    private:
        std::vector<TMove> AllowedMoves;
        TReachedPositions ReachedPositions;

        TG1Stage();
        ~TG1Stage();

        void Init();
        void FillAllowedMoves();
        void FillReachedPositions();
};


