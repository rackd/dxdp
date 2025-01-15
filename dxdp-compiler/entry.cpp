#include "dxdp-compiler/compiler.h"
#include "dxdp-ipc/ipc.h"
#include "dxdp-ipc/ipc_message.h"
#include "dxdp/sysutil.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <stdlib.h>

//TODO: COMPILER bug..only stderr works
const char* argv2[] = {"compile", "-triple", "bpf", "-x", "c", "-O3",
    "-g0", "-funroll-loops", "-fomit-frame-pointer"};
int argc2 = 6;

void handler(ipc_packet* packet, void* addl_data) {
    handler_context* context = (handler_context*)addl_data;
    ipc_packet* response = context->response_packet;
    response->message = IPC_MESSAGE_ERROR;

    if(packet->message == IPC_MESSAGE_COMPILE_ME) {
        char* source_code = packet->argv[0];
        response->argc = 2;
        size_t bin_size;
        if(!compile_source(source_code, response->argv[0], &bin_size)) {
            fprintf(stderr, "Compile source failed.\n");
            return;
        }
        memcpy(response->argv[1], &bin_size, sizeof(size_t));
        response->message = IPC_MESSAGE_SUCCESS;
    }
}


int main(int argc, char** argv) {
    const char** argv_ = argv2;
    llvm::InitLLVM X(argc2, argv_);

    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    ipc_srv* srv = ipc_srv_init(IPC_SHM_CIMP_NAME, handler, NULL, "root");
    if (!srv) {
        fprintf(stderr, "Failed to initialize IPC server.\n");
        return 1;
    }

    fprintf(stderr, "Server is running.\n");
    pthread_join(srv->thread, NULL);
    ipc_srv_destroy(srv);
    return 0;
}