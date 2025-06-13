#include "EventListener.h"
#include "Hooks.h"
#include "Settings.h"

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void MessageHandler(SKSE::MessagingInterface::Message* message)
{
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Hooks::Install();
        EventListener::Register();
    }
}

SKSEPluginInfo(
        .Version = Plugin::VERSION,
        .Name = Plugin::NAME.data(),
        .Author = "GabonZ",
        .StructCompatibility = SKSE::StructCompatibility::Independent,
        .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
#ifdef WAIT_FOR_DEBUGGER
    // Pause execution until the debugger is attached.
    while (!IsDebuggerPresent()) Sleep(1000);
#endif

    std::shared_ptr<spdlog::logger> log;

    if (IsDebuggerPresent()) {
        // Use MSVC sink for debug output.
        log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
    } else {
        // Set up file logging.
        // ReSharper disable once CppLocalVariableMayBeConst
        auto path = SKSE::log::log_directory();
        if (!path) {
            stl::report_and_fail("Failed to find standard logging directory"sv);
        }

        *path /= std::format("{}.log", Plugin::NAME);
        log = std::make_shared<spdlog::logger>(
            "Global",
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
    }

#ifdef NDEBUG
    spdlog::level::level_enum logLevel = spdlog::level::info;
#else
    spdlog::level::level_enum logLevel = spdlog::level::debug;
#endif

    bool loadError = false;
    try {
        const auto settings = Settings::GetSingleton();
        settings->Load();

        if (settings->debugLogging) {
            logLevel = spdlog::level::debug;
        }
    } catch (...) {
        loadError = true;
    }
    log->set_level(logLevel);
    log->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    if (loadError) {
        logger::warn("Exception caught when loading settings! Default settings will be used");
    }

    SKSE::Init(a_skse, false);
    SKSE::AllocTrampoline(64);

    if (!SKSE::GetMessagingInterface()->RegisterListener(MessageHandler)) {
        stl::report_and_fail("Unable to register message listener.");
    }

    return true;
}
