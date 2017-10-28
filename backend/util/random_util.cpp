#include "random_util.h"


//
// TRandomLetter
//

TRandomLetter::TRandomLetter()
    : Uniform(0, 10 + 26 * 2 - 1)
{
    Rng.seed(rand());
}

TRandomLetter &TRandomLetter::Instance() {
    static TRandomLetter Obj;
    return Obj;
}

char TRandomLetter::Generate() {
    static const char Letters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return Letters[Uniform(Rng)];
}


//
// TRandomUUID
//

TRandomUUID &TRandomUUID::Instance() {
    static TRandomUUID Obj;
    return Obj;
}

std::string TRandomUUID::Generate() {
    std::string result;
    std::unique_lock<std::mutex> lk(Mutex);
    static const char letters[] = "0123456789abcdefgh";
    boost::uuids::uuid u = Gen();
    for (unsigned char c : u) {
        result += letters[c & ((1 << 4) - 1)];
        result += letters[(c >> 4) & ((1 << 4) - 1)];
    }
    return result;
}

