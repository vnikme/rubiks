#include "kociemba_impl.h"
#include "bfs2.h"
#include <exception>


// Base class for pruners
class TBaseEstimator : private boost::noncopyable {
    public:
        using TCubeImageType = TCubeImage<2>;

        const std::vector<TMove> &GetAllowedMoves() const;
        TCubeImageType GetImage(const TCube &cube) const;
        int Estimate(const TCube &cube) const;

    protected:
        void Init();
        virtual size_t DoGetImage(const TCube &cube) const = 0;

        TBaseEstimator(const std::vector<TMove> &allowedMoves);
        ~TBaseEstimator();

    private:
        const std::vector<TMove> &AllowedMoves;
        bool IsInit = false;
        std::vector<int> Distances;

        void SetDistance(size_t index, int value);
};

TBaseEstimator::TBaseEstimator(const std::vector<TMove> &allowedMoves)
    : AllowedMoves(allowedMoves)
{
}

TBaseEstimator::~TBaseEstimator() {
}

const std::vector<TMove> &TBaseEstimator::GetAllowedMoves() const {
    return AllowedMoves;
}

void TBaseEstimator::Init() {
    if (IsInit)
        return;
    IsInit = true;
    std::unordered_map<TCubeImageType, TMove> reached;
    PlainBFS(*this, reached, 21);
    for (const auto &item : reached) {
        int index = (static_cast<int>(item.first.Data[1]) << 8) + static_cast<int>(item.first.Data[0]);
        SetDistance(index, item.second.GetTotalTurnsCount());
    }
    std::cout << reached.size() << std::endl;
}

void TBaseEstimator::SetDistance(size_t index, int value) {
    if (index >= Distances.size())
        Distances.resize(index + 1, -1);
    Distances[index] = value;
}

TBaseEstimator::TCubeImageType TBaseEstimator::GetImage(const TCube &cube) const {
    int res = DoGetImage(cube);
    TCubeImageType result;
    result.Data[0] = (res & ((1 << 8) - 1));
    result.Data[1] = ((res >> 8) & ((1 << 8) - 1));
    return result;
}

int TBaseEstimator::Estimate(const TCube &cube) const {
    size_t idx = DoGetImage(cube);
    return idx < Distances.size() ? Distances[idx] : -1;
}


// Pruning for corners in the stage 0
class TG0CornersEstimator : public TBaseEstimator {
    public:
        static TG0CornersEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG0CornersEstimator(const std::vector<TMove> &allowedMoves);
        ~TG0CornersEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG0CornersEstimator::TG0CornersEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG0CornersEstimator::~TG0CornersEstimator() {
}

TG0CornersEstimator &TG0CornersEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG0CornersEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG0CornersEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    size_t value = 0;
    for (size_t i = 0; i < 3; ++i) {
        EColor color = cube.GetColor(idxs[i]);
        if (color == C_RED || color == C_ORANGE)
            value = i;
    }
    return value;
}

size_t TG0CornersEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &corners = TCube::GetAllCorners();
    size_t res = 0;
    for (size_t i = 0; i + 3 < corners.size(); i += 3) {        // Do not take the last cubie because of parity
        res *= 3;
        res += CalcCubieValue(cube, &corners[i]);
    }
    return res;
}


// Pruning for edges in the stage 0
class TG0EdgeEstimator : public TBaseEstimator {
    public:
        static TG0EdgeEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG0EdgeEstimator(const std::vector<TMove> &allowedMoves);
        ~TG0EdgeEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG0EdgeEstimator::TG0EdgeEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG0EdgeEstimator::~TG0EdgeEstimator() {
}

TG0EdgeEstimator &TG0EdgeEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG0EdgeEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG0EdgeEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    EColor c1 = cube.GetColor(idxs[0]), c2 = cube.GetColor(idxs[1]);
    if (c1 == C_RED || c1 == C_ORANGE)
        return 0;
    if (c2 == C_RED || c2 == C_ORANGE)
        return 1;
    if (c1 == C_WHITE || c1 == C_YELLOW)
        return 1;
    if (c2 == C_WHITE || c2 == C_YELLOW)
        return 0;
    throw std::logic_error("Bad cubie in edge estimator.");
}

size_t TG0EdgeEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &edges = TCube::GetAllEdges();
    size_t res = 0;
    for (size_t i = 0; i + 2 < edges.size(); i += 2) {          // Do not take the last cubie because of parity
        res *= 2;
        res += CalcCubieValue(cube, &edges[i]);
    }
    return res;
}


// Pruning for middle layer edges in the stage 0
class TG0MiddleLayerEdgesEstimator : public TBaseEstimator {
    public:
        static TG0MiddleLayerEdgesEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG0MiddleLayerEdgesEstimator(const std::vector<TMove> &allowedMoves);
        ~TG0MiddleLayerEdgesEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG0MiddleLayerEdgesEstimator::TG0MiddleLayerEdgesEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG0MiddleLayerEdgesEstimator::~TG0MiddleLayerEdgesEstimator() {
}

TG0MiddleLayerEdgesEstimator &TG0MiddleLayerEdgesEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG0MiddleLayerEdgesEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG0MiddleLayerEdgesEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    EColor c1 = cube.GetColor(idxs[0]), c2 = cube.GetColor(idxs[1]);
    if (c1 == C_RED || c1 == C_ORANGE || c2 == C_RED || c2 == C_ORANGE)
        return 0;
    return 1;
}

size_t TG0MiddleLayerEdgesEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &edges = TCube::GetAllEdges();
    size_t res = 0;
    for (size_t i = 0; i + 2 < edges.size(); i += 2) {          // Do not take the last cubie because of parity
        res <<= 1;
        if (CalcCubieValue(cube, &edges[i]))
            res |= 1;
    }
    return res;
}


// Main description of the stage 0
TG0Stage::TG0Stage() {
}

TG0Stage::~TG0Stage() {
}

TG0Stage &TG0Stage::Instance() {
    static TG0Stage Obj;
    Obj.Init();
    return Obj;
}

TG0Stage::TCubeImageType TG0Stage::GetImage(const TCube &cube) const {
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

const std::vector<TMove> &TG0Stage::GetAllowedMoves() const {
    return AllowedMoves;
}

const TG0Stage::TReachedPositions &TG0Stage::GetReachedPositions() const {
    return ReachedPositions;
}

int TG0Stage::Estimate(const TCube &cube) const {
    int a = TG0CornersEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    int b = TG0EdgeEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    int c = TG0MiddleLayerEdgesEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    if (a == -1 || b == -1 || c == -1)
        return -1;
    return std::max(a, std::max(b, c));
}

void TG0Stage::Init() {
    if (!AllowedMoves.empty())
        return;
    FillAllowedMoves();
    TG0CornersEstimator::Instance(GetAllowedMoves());
    TG0EdgeEstimator::Instance(GetAllowedMoves());
    TG0MiddleLayerEdgesEstimator::Instance(GetAllowedMoves());
    FillReachedPositions();
}

void TG0Stage::FillAllowedMoves() {
    static constexpr ETurnExt moves[] = { TE_U, TE_U2, TE_U1, TE_D, TE_D2, TE_D1, TE_L2, TE_R2, TE_F2, TE_B2,
                                          TE_L, TE_L1, TE_R, TE_R1, TE_F, TE_F1, TE_B, TE_B1 };
    for (auto move : moves)
        AllowedMoves.push_back(TurnExt2Move(move));
    std::cout << AllowedMoves.size() << std::endl;
}

void TG0Stage::FillReachedPositions() {
    PlainBFS(*this, ReachedPositions, 6);
    std::cout << ReachedPositions.size() << std::endl;
}


static size_t PermutationIndex(const std::vector<size_t> &p) {
    size_t f = 1, n = p.size();
    for (size_t i = 0; i < n; ++i)
        f *= (i + 1);
    auto begin = p.begin(), end = p.end();
    size_t res = 0;
    for (size_t i = 0; i < n; ++i) {
        f /= (n - i);
        size_t val = p[i];
        res += (std::count_if(begin + i + 1, end, [val] (size_t item) { return item < val; }) * f);
    }
    return res;
}


// Pruning for corners in the stage 1
class TG1CornersEstimator : public TBaseEstimator {
    public:
        static TG1CornersEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG1CornersEstimator(const std::vector<TMove> &allowedMoves);
        ~TG1CornersEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG1CornersEstimator::TG1CornersEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG1CornersEstimator::~TG1CornersEstimator() {
}

TG1CornersEstimator &TG1CornersEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG1CornersEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG1CornersEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    size_t value = 0;
    for (size_t i = 0; i < 3; ++i) {
        EColor color = cube.GetColor(idxs[i]);
        if (color == C_ORANGE)
            value += 4;
        else if (color == C_BLUE)
            value += 2;
        else if (color == C_YELLOW)
            value += 1;
    }
    return value;
}

size_t TG1CornersEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &corners = TCube::GetAllCorners();
    std::vector<size_t> p(corners.size() / 3);
    for (size_t i = 0; i < corners.size(); i += 3)
        p[i / 3] = CalcCubieValue(cube, &corners[i]);
    return PermutationIndex(p);
}


// Pruning for edges in the stage 1
class TG1EdgeEstimator : public TBaseEstimator {
    public:
        static TG1EdgeEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG1EdgeEstimator(const std::vector<TMove> &allowedMoves);
        ~TG1EdgeEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG1EdgeEstimator::TG1EdgeEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG1EdgeEstimator::~TG1EdgeEstimator() {
}

TG1EdgeEstimator &TG1EdgeEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG1EdgeEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG1EdgeEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    size_t value = 0;
    for (size_t i = 0; i < 2; ++i) {
        EColor color = cube.GetColor(idxs[i]);
        if (color == C_ORANGE)
            value += 4;
        else if (color == C_GREEN)
            value += 3;
        else if (color == C_BLUE)
            value += 2;
        else if (color == C_YELLOW)
            value += 1;
    }
    return value;
}

size_t TG1EdgeEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &edges = TCube::GetTopBottomEdges();
    std::vector<size_t> p(edges.size() / 2);
    for (size_t i = 0; i < edges.size(); i += 2)
        p[i / 2] = CalcCubieValue(cube, &edges[i]);
    return PermutationIndex(p);
}


// Pruning for middle layer edges in the stage 1
class TG1MiddleLayerEdgesEstimator : public TBaseEstimator {
    public:
        static TG1MiddleLayerEdgesEstimator &Instance(const std::vector<TMove> &allowedMoves);

    private:
        TG1MiddleLayerEdgesEstimator(const std::vector<TMove> &allowedMoves);
        ~TG1MiddleLayerEdgesEstimator();

        size_t CalcCubieValue(const TCube &cube, const size_t *idxs) const;
        size_t DoGetImage(const TCube &cube) const override;
};

TG1MiddleLayerEdgesEstimator::TG1MiddleLayerEdgesEstimator(const std::vector<TMove> &allowedMoves)
    : TBaseEstimator(allowedMoves)
{
}

TG1MiddleLayerEdgesEstimator::~TG1MiddleLayerEdgesEstimator() {
}

TG1MiddleLayerEdgesEstimator &TG1MiddleLayerEdgesEstimator::Instance(const std::vector<TMove> &allowedMoves) {
    static TG1MiddleLayerEdgesEstimator Obj(allowedMoves);
    Obj.Init();
    return Obj;
}

size_t TG1MiddleLayerEdgesEstimator::CalcCubieValue(const TCube &cube, const size_t *idxs) const {
    size_t value = 0;
    for (size_t i = 0; i < 2; ++i) {
        EColor color = cube.GetColor(idxs[i]);
        if (color == C_BLUE)
            value += 2;
        else if (color == C_YELLOW)
            value += 1;
    }
    return value;
}

size_t TG1MiddleLayerEdgesEstimator::DoGetImage(const TCube &cube) const {
    const std::vector<size_t> &edges = TCube::GetMiddleEdges();
    std::vector<size_t> p(edges.size() / 2);
    for (size_t i = 0; i < edges.size(); i += 2)
        p[i / 2] = CalcCubieValue(cube, &edges[i]);
    return PermutationIndex(p);
}


// Main description of the stage 1
TG1Stage::TG1Stage() {
}

TG1Stage::~TG1Stage() {
}

TG1Stage &TG1Stage::Instance() {
    static TG1Stage Obj;
    Obj.Init();
    return Obj;
}

TG1Stage::TCubeImageType TG1Stage::GetImage(const TCube &cube) const {
    TCubeImageType result;
    const auto &cornersImage = TG1CornersEstimator::Instance(GetAllowedMoves()).GetImage(cube);
    const auto &edgesImage = TG1EdgeEstimator::Instance(GetAllowedMoves()).GetImage(cube);
    const auto &middleImage = TG1MiddleLayerEdgesEstimator::Instance(GetAllowedMoves()).GetImage(cube);
    result.Data[0] = cornersImage.Data[0];
    result.Data[1] = cornersImage.Data[1];
    result.Data[2] = edgesImage.Data[0];
    result.Data[3] = edgesImage.Data[1];
    result.Data[4] = middleImage.Data[0];
    return result;
}

const std::vector<TMove> &TG1Stage::GetAllowedMoves() const {
    return AllowedMoves;
}

const TG1Stage::TReachedPositions &TG1Stage::GetReachedPositions() const {
    return ReachedPositions;
}

int TG1Stage::Estimate(const TCube &cube) const {
    int a = TG1CornersEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    int b = TG1EdgeEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    int c = TG1MiddleLayerEdgesEstimator::Instance(GetAllowedMoves()).Estimate(cube);
    if (a == -1 || b == -1 || c == -1)
        return -1;
    return std::max(a, std::max(b, c));
}

void TG1Stage::Init() {
    if (!AllowedMoves.empty())
        return;
    FillAllowedMoves();
    TG1CornersEstimator::Instance(GetAllowedMoves());
    TG1EdgeEstimator::Instance(GetAllowedMoves());
    TG1MiddleLayerEdgesEstimator::Instance(GetAllowedMoves());
    FillReachedPositions();
}

void TG1Stage::FillAllowedMoves() {
    static constexpr ETurnExt moves[] = { TE_U, TE_U2, TE_U1, TE_D, TE_D2, TE_D1, TE_L2, TE_R2, TE_F2, TE_B2 };
    for (auto move : moves)
        AllowedMoves.push_back(TurnExt2Move(move));
    std::cout << AllowedMoves.size() << std::endl;
}

void TG1Stage::FillReachedPositions() {
    PlainBFS(*this, ReachedPositions, 8);
    std::cout << ReachedPositions.size() << std::endl;
}

