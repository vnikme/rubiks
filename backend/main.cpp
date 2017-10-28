#include <cstdlib>
#include <ctime>
#include <iostream>
#include <boost/program_options.hpp>
#include "network/echo.h"
#include "network/server.h"
#include "network/worker_pool.h"
#include "rubiks/rubiks.h"
#include "dist/json/json.h"


namespace opt = boost::program_options;

class TApplication {
public:
    int Run(int argc, char *argv[]) {
        if (!ParseCommandLine(argc, argv))
            return 1;

        srand(time(nullptr));

        TRubiks rub;
        std::ifstream fin(ConfigPath);
        Json::Value data;
        Json::Reader reader;
        if (!reader.parse(fin, data, false))
            return 1;
 
        rub.Init(data);
        rub.Run();
        rub.Join();

        return 0;
    }

private:
    std::string ConfigPath;

private:
    bool ParseCommandLine(int argc, char *argv[]) {
        opt::options_description desc("Allowed options");
        desc.add_options()
            ("help",   "produce help message")
            ("config", opt::value<std::string>(), "path to config");

        opt::variables_map vm;
        opt::store(opt::parse_command_line(argc, argv, desc), vm);
        opt::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return false;
        }

        ConfigPath = vm["config"].as<std::string>();

        return true;
    }
};

int main(int argc, char *argv[]) {
    TApplication app;
    return app.Run(argc, argv);
}

