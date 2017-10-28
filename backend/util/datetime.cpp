#include "datetime.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>


time_t LocalDateTimeToTimeT(const std::string &tm) {
    try {
        std::istringstream ss(tm);
        auto *ifc = new boost::local_time::local_time_input_facet("%Y-%m-%d %H:%M:%S%F %ZP");
        ss.imbue(std::locale(ss.getloc(), ifc));
        boost::local_time::local_date_time localTime(boost::local_time::not_a_date_time);
        ss >> localTime;
        boost::posix_time::ptime postTime(localTime.utc_time()), base(boost::posix_time::from_time_t(0));
        return (postTime - base).total_seconds();
    } catch (...) {
        return -1;
    }
}

