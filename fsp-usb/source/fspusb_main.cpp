#include "fspusb_service.hpp"
#include "impl/fspusb_usb_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = { 0x0100000000000BEF };

    namespace result {

        bool CallFatalOnResultAssertion = true;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::SetVersionForLibnx();

    sm::DoWithSession([&]() {
        /*
        R_ASSERT(fsInitialize());
        R_ASSERT(fsdevMountSdmc());
        */
        ::Result rc;
        do {
            rc = fspusb::impl::InitializeManager();
        } while(R_FAILED(rc));
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /*
    fsdevUnmountAll();
    fsExit();
    */
    fspusb::impl::FinalizeManager();
}

namespace {

    constexpr sm::ServiceName ServiceName = sm::ServiceName::Encode("fsp-usb");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x800;
            static constexpr size_t MaxDomains = 0x40;
            static constexpr size_t MaxDomainObjects = 0x4000;
        };
        /* Same options as fsp-srv, this will be a service with similar behaviour */

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 61;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

}

int main(int argc, char **argv) {
    
    ams::os::Thread usb_thread;
    R_ASSERT(usb_thread.Initialize(&fspusb::impl::ManagerUpdateThread, nullptr, 0x4000, 0x15));
    R_ASSERT(usb_thread.Start());

    R_ASSERT(g_server_manager.RegisterServer<fspusb::Service>(ServiceName, MaxSessions));

    g_server_manager.LoopProcess();

    return 0;
}