#include "dump_offload_mgr.hpp"

#include <fmt/format.h>

#include <cstdlib>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

using ::phosphor::logging::level;
using ::phosphor::logging::log;

int main()
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();
        openpower::dump::DumpOffloadManager manager(bus);
        manager.offload();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
        return event.loop();
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("exception during application load({})", ex.what())
                .c_str());
        throw;
    }
}
