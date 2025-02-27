// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_types.h"
#include "core/custom_tex_cache.h"
#include "core/frontend/applets/mii_selector.h"
#include "core/frontend/applets/swkbd.h"
#include "core/frontend/image_interface.h"
#include "core/loader/loader.h"
#include "core/memory.h"
#include "core/perf_stats.h"
#include "core/telemetry_session.h"

class ARM_Interface;

namespace Frontend {
class EmuWindow;
}

namespace Memory {
class MemorySystem;
}

namespace AudioCore {
class DspInterface;
}

namespace RPC {
class RPCServer;
}

namespace Service {
namespace SM {
class ServiceManager;
}
namespace FS {
class ArchiveManager;
}
} // namespace Service

namespace Kernel {
class KernelSystem;
}

namespace Cheats {
class CheatEngine;
}

namespace VideoDumper {
class Backend;
}

namespace Core {

class Timing;

class System {
public:
    /**
     * Gets the instance of the System singleton class.
     * @returns Reference to the instance of the System singleton class.
     */
    static System& GetInstance() {
        return s_instance;
    }

    /// Enumeration representing the return values of the System Initialize and Load process.
    enum class ResultStatus : u32 {
        Success,                    ///< Succeeded
        ErrorNotInitialized,        ///< Error trying to use core prior to initialization
        ErrorGetLoader,             ///< Error finding the correct application loader
        ErrorSystemMode,            ///< Error determining the system mode
        ErrorLoader,                ///< Error loading the specified application
        ErrorLoader_ErrorEncrypted, ///< Error loading the specified application due to encryption
        ErrorLoader_ErrorInvalidFormat,     ///< Error loading the specified application due to an
                                            /// invalid format
        ErrorSystemFiles,                   ///< Error in finding system files
        ErrorVideoCore,                     ///< Error in the video core
        ErrorVideoCore_ErrorGenericDrivers, ///< Error in the video core due to the user having
                                            /// generic drivers installed
        ErrorVideoCore_ErrorBelowGL33,      ///< Error in the video core due to the user not having
                                            /// OpenGL 3.3 or higher
        ShutdownRequested,                  ///< Emulated program requested a system shutdown
        ErrorUnknown                        ///< Any other error
    };

    /**
     * Run the core CPU loop
     * This function runs the core for the specified number of CPU instructions before trying to
     * update hardware. This is much faster than SingleStep (and should be equivalent), as the CPU
     * is not required to do a full dispatch with each instruction. NOTE: the number of instructions
     * requested is not guaranteed to run, as this will be interrupted preemptively if a hardware
     * update is requested (e.g. on a thread switch).
     * @param tight_loop If false, the CPU single-steps.
     * @return Result status, indicating whethor or not the operation succeeded.
     */
    ResultStatus RunLoop(bool tight_loop = true);

    /**
     * Step the CPU one instruction
     * @return Result status, indicating whethor or not the operation succeeded.
     */
    ResultStatus SingleStep();

    /// Shutdown the emulated system.
    void Shutdown();

    /// Shutdown and then load again
    void Reset();

    /// Request reset of the system
    void RequestReset() {
        reset_requested = true;
    }

    /// Request shutdown of the system
    void RequestShutdown() {
        shutdown_requested = true;
    }

    /**
     * Load an executable application.
     * @param emu_window Reference to the host-system window used for video output and keyboard
     *                   input.
     * @param filepath String path to the executable application to load on the host file system.
     * @returns ResultStatus code, indicating if the operation succeeded.
     */
    ResultStatus Load(Frontend::EmuWindow& emu_window, const std::string& filepath);

    /**
     * Indicates if the emulated system is powered on (all subsystems initialized and able to run an
     * application).
     * @returns True if the emulated system is powered on, otherwise false.
     */
    bool IsPoweredOn() const {
        return cpu_core != nullptr;
    }

    /**
     * Returns a reference to the telemetry session for this emulation session.
     * @returns Reference to the telemetry session.
     */
    Core::TelemetrySession& TelemetrySession() const {
        return *telemetry_session;
    }

    /// Prepare the core emulation for a reschedule
    void PrepareReschedule();

    PerfStats::Results GetAndResetPerfStats();

    /**
     * Gets a reference to the emulated CPU.
     * @returns A reference to the emulated CPU.
     */
    ARM_Interface& CPU() {
        return *cpu_core;
    }

    /**
     * Gets a reference to the emulated DSP.
     * @returns A reference to the emulated DSP.
     */
    AudioCore::DspInterface& DSP() {
        return *dsp_core;
    }

    /**
     * Gets a reference to the service manager.
     * @returns A reference to the service manager.
     */
    Service::SM::ServiceManager& ServiceManager();

    /**
     * Gets a const reference to the service manager.
     * @returns A const reference to the service manager.
     */
    const Service::SM::ServiceManager& ServiceManager() const;

    /// Gets a reference to the archive manager
    Service::FS::ArchiveManager& ArchiveManager();

    /// Gets a const reference to the archive manager
    const Service::FS::ArchiveManager& ArchiveManager() const;

    /// Gets a reference to the kernel
    Kernel::KernelSystem& Kernel();

    /// Gets a const reference to the kernel
    const Kernel::KernelSystem& Kernel() const;

    /// Gets a reference to the timing system
    Timing& CoreTiming();

    /// Gets a const reference to the timing system
    const Timing& CoreTiming() const;

    /// Gets a reference to the memory system
    Memory::MemorySystem& Memory();

    /// Gets a const reference to the memory system
    const Memory::MemorySystem& Memory() const;

    /// Gets a reference to the cheat engine
    Cheats::CheatEngine& CheatEngine();

    /// Gets a const reference to the cheat engine
    const Cheats::CheatEngine& CheatEngine() const;

    /// Gets a reference to the custom texture cache system
    Core::CustomTexCache& CustomTexCache();

    /// Gets a const reference to the custom texture cache system
    const Core::CustomTexCache& CustomTexCache() const;

    /// Gets a reference to the video dumper backend
    VideoDumper::Backend& VideoDumper();

    /// Gets a const reference to the video dumper backend
    const VideoDumper::Backend& VideoDumper() const;

    std::unique_ptr<PerfStats> perf_stats;
    FrameLimiter frame_limiter;

    void SetStatus(ResultStatus new_status, const char* details = nullptr) {
        status = new_status;
        if (details) {
            status_details = details;
        }
    }

    const std::string& GetStatusDetails() const {
        return status_details;
    }

    Loader::AppLoader& GetAppLoader() const {
        return *app_loader;
    }

    /// Frontend Applets

    void RegisterMiiSelector(std::shared_ptr<Frontend::MiiSelector> mii_selector);

    void RegisterSoftwareKeyboard(std::shared_ptr<Frontend::SoftwareKeyboard> swkbd);

    std::shared_ptr<Frontend::MiiSelector> GetMiiSelector() const {
        return registered_mii_selector;
    }

    std::shared_ptr<Frontend::SoftwareKeyboard> GetSoftwareKeyboard() const {
        return registered_swkbd;
    }

    /// Image interface

    void RegisterImageInterface(std::shared_ptr<Frontend::ImageInterface> image_interface);

    std::shared_ptr<Frontend::ImageInterface> GetImageInterface() const {
        return registered_image_interface;
    }

private:
    /**
     * Initialize the emulated system.
     * @param emu_window Reference to the host-system window used for video output and keyboard
     *                   input.
     * @param system_mode The system mode.
     * @return ResultStatus code, indicating if the operation succeeded.
     */
    ResultStatus Init(Frontend::EmuWindow& emu_window, u32 system_mode);

    /// Reschedule the core emulation
    void Reschedule();

    /// AppLoader used to load the current executing application
    std::unique_ptr<Loader::AppLoader> app_loader;

    /// ARM11 CPU core
    std::shared_ptr<ARM_Interface> cpu_core;

    /// DSP core
    std::unique_ptr<AudioCore::DspInterface> dsp_core;

    /// When true, signals that a reschedule should happen
    bool reschedule_pending{};

    /// Telemetry session for this emulation session
    std::unique_ptr<Core::TelemetrySession> telemetry_session;

    /// Service manager
    std::shared_ptr<Service::SM::ServiceManager> service_manager;

    /// Frontend applets
    std::shared_ptr<Frontend::MiiSelector> registered_mii_selector;
    std::shared_ptr<Frontend::SoftwareKeyboard> registered_swkbd;

    /// Cheats manager
    std::unique_ptr<Cheats::CheatEngine> cheat_engine;

    /// Custom texture cache system
    std::unique_ptr<Core::CustomTexCache> custom_tex_cache;

    /// Image interface
    std::shared_ptr<Frontend::ImageInterface> registered_image_interface;

    /// Video dumper backend
    std::unique_ptr<VideoDumper::Backend> video_dumper;

    /// RPC Server for scripting support
    std::unique_ptr<RPC::RPCServer> rpc_server;

    std::unique_ptr<Service::FS::ArchiveManager> archive_manager;

    std::unique_ptr<Memory::MemorySystem> memory;
    std::unique_ptr<Kernel::KernelSystem> kernel;
    std::unique_ptr<Timing> timing;

private:
    static System s_instance;

    ResultStatus status = ResultStatus::Success;
    std::string status_details = "";
    /// Saved variables for reset
    Frontend::EmuWindow* m_emu_window;
    std::string m_filepath;

    std::atomic<bool> reset_requested;
    std::atomic<bool> shutdown_requested;
};

inline ARM_Interface& CPU() {
    return System::GetInstance().CPU();
}

inline AudioCore::DspInterface& DSP() {
    return System::GetInstance().DSP();
}

} // namespace Core
