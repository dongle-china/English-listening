#include "core/Handshake.h"
#include "core/Logger.h"

#include <iostream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace el::core::handshake
{
    namespace
    {
        int currentPid()
        {
#ifdef _WIN32
            return static_cast<int>(_getpid());
#else
            return static_cast<int>(getpid());
#endif
        }
    }

    void emit(const Ready& ready)
    {
        const nlohmann::json payload = {
            {"event", "ready"},
            {"host", ready.host},
            {"port", ready.port},
            {"wsPort", ready.wsPort},
            {"token", ready.token},
            {"version", ready.version},
            {"pid", ready.pid > 0 ? ready.pid : currentPid()},
            {"ts", logger::nowIso8601Utc()}
        };
        std::cout << payload.dump() << std::endl;
    }
}
