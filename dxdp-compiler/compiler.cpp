#include "dxdp-compiler/compiler.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Basic/LangStandard.h"

#define DXDP_TARGET "bpf-unknown-linux-gnu"

const char* argv[] = {"compile", "-triple", "bpf", "-x", "c", "-O3",
    "-g0", "-funroll-loops", "-fomit-frame-pointer"};
int argc = 6;

class DiagConsumer : public clang::DiagnosticConsumer {
public:
    DiagConsumer() {}
    void HandleDiagnostic(clang::DiagnosticsEngine::Level diagLevel,
        const clang::Diagnostic &diag) override {
        llvm::SmallString<128> message;
        diag.FormatDiagnostic(message);
        llvm::errs() << "Error: " << message << "\n";
    }
};

bool compile_source(const char* source_code, void* _out_buf, size_t* _bin_size) {


    std::unique_ptr<clang::CompilerInstance>
        compInst(new clang::CompilerInstance());
    std::shared_ptr<clang::DiagnosticOptions> diagOpts =
        std::make_shared<clang::DiagnosticOptions>();
    compInst->createDiagnostics(new DiagConsumer(), true);

    llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> vfs
        = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> rfs
        = llvm::vfs::getRealFileSystem();
    llvm::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> ofs
        = llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(rfs);
    ofs->pushOverlay(vfs);
    if (std::error_code err = vfs->setCurrentWorkingDirectory("/")) {
        std::cerr << "setCurrentWorkingDirectory() failed" << std::endl;
        return false;
    }
    vfs->addFile("/in.c", 0,
        llvm::MemoryBuffer::getMemBufferCopy(source_code));
    compInst->createFileManager(ofs);

    std::vector<const char*> args;
    for(int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    clang::CompilerInvocation::CreateFromArgs(
        compInst->getInvocation(),
        args,
        compInst->getDiagnostics()
        );

    compInst->getInvocation().getFrontendOpts().Inputs.clear();
    compInst->getInvocation().getFrontendOpts().Inputs.push_back(
        clang::FrontendInputFile("in.c", clang::Language::C));
    compInst->getInvocation().getFrontendOpts().ProgramAction =
        clang::frontend::EmitBC;
   // compInst->getCodeGenOpts().setDebugInfo(
       // llvm::codegenoptions::FullDebugInfo);
    compInst->getTargetOpts().Triple = DXDP_TARGET;
    assert(!compInst->getInvocation().getFrontendOpts().DisableFree);

    compInst->getHeaderSearchOpts().AddPath("/usr/include",
        clang::frontend::System, false, false);
    compInst->getHeaderSearchOpts().AddPath("/usr/local/include",
        clang::frontend::System, false, false);
    compInst->getHeaderSearchOpts().AddPath("/usr/lib/clang/18/include",
        clang::frontend::System, false, false);

    if(!compInst->createTarget()) {
        return false;
    }

    compInst->getCodeGenOpts().CodeModel = "medium";
    clang::SmallString<256> svBuffer;
    compInst->setOutputStream(std::make_unique<llvm::raw_svector_ostream>
        (svBuffer));


    clang::EmitBCAction action;
    if(!action.BeginSourceFile(*compInst, compInst->getInvocation()
        .getFrontendOpts().Inputs.front())) {
        return false;
    }
    if(llvm::Error err = action.Execute()) {
        return false;
    }
    action.EndSourceFile();

    // Bitcode buffer size
    if(svBuffer.size() <= 0) {
        return false;
    }

    auto BitcodeBuffer = llvm::MemoryBuffer::getMemBufferCopy(
        llvm::StringRef(svBuffer.data(), svBuffer.size()));

    llvm::LLVMContext llvmContext;
    llvm::Expected<std::unique_ptr<llvm::Module>> modOrError =
        llvm::parseBitcodeFile(BitcodeBuffer->getMemBufferRef(), llvmContext);
    if (!modOrError) {
        return false;
    }
    std::unique_ptr<llvm::Module> llvmModule = std::move(*modOrError);

    std::string s_targetLookupError;
    const llvm::Target* Target =
        llvm::TargetRegistry::lookupTarget(DXDP_TARGET, s_targetLookupError);
    if (!Target) {
        return false;
    }
    llvmModule->setTargetTriple(DXDP_TARGET);

    llvm::TargetOptions Options;
    auto RM = std::optional<llvm::Reloc::Model>();
    llvm::TargetMachine* TheTargetMachine = Target->createTargetMachine(
        DXDP_TARGET,
        "generic",
        "",
        Options,
        std::optional<llvm::Reloc::Model>(),
        std::optional<llvm::CodeModel::Model>(),
        llvm::CodeGenOptLevel::Default,
        false);

    clang::SmallString<256> ObjectMemoryBuffer;
    llvm::raw_svector_ostream OS(ObjectMemoryBuffer);
    llvm::legacy::PassManager PM;
    bool success = TheTargetMachine->addPassesToEmitFile(PM, OS, nullptr,
        llvm::CodeGenFileType::ObjectFile,
        true,
        (llvm::MachineModuleInfoWrapperPass*)nullptr);

    if (success) {
        std::cerr << "The TargetMachine cannot emit a file of this type."
        << std::endl;
        return false;
    }

    PM.run(*llvmModule);

    if(ObjectMemoryBuffer.size() <= 0) {
        return false;
    }

    llvm::llvm_shutdown();

    // Compiled bin size too big for buff
    if(ObjectMemoryBuffer.size() > MAX_COMPILED_BUFFER_SIZE) {
        return false;
    }

    memcpy(_out_buf, ObjectMemoryBuffer.data(), ObjectMemoryBuffer.size());
    *_bin_size = ObjectMemoryBuffer.size();
    return true;
}
