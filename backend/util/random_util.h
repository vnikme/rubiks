#pragma once

#include <cstdlib>
#include <mutex>
#include <boost/noncopyable.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>


class TRandomLetter : private boost::noncopyable {
    public:
        static TRandomLetter &Instance();
        char Generate();

    private:
        boost::random::mt19937 Rng;
        boost::random::uniform_int_distribution<> Uniform;

        TRandomLetter();
        ~TRandomLetter() = default;
};


class TRandomUUID : private boost::noncopyable {
    public:
        static TRandomUUID &Instance();
        std::string Generate();

    private:
        boost::uuids::basic_random_generator<boost::mt19937> Gen;
        std::mutex Mutex;

        TRandomUUID() = default;
        ~TRandomUUID() = default;
};

