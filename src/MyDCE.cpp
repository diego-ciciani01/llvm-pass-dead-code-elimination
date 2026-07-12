#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

#if __has_include("llvm/Plugins/PassPlugin.h")
  #include "llvm/Plugins/PassPlugin.h"   // LLVM >= 22
#else
  #include "llvm/Passes/PassPlugin.h"    // LLVM <= 21
#endif

/* Requred to find the 'include' files from llvm namespace*/
using namespace llvm;

namespace {

  struct MyDCEPass: PassInfoMixin<MyDCEPass>{

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM){

      return PreservedAnalyses::all();
    }

  };
}


extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MyDCE", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "my-dce") {
                    FPM.addPass(MyDCEPass());
                    return true;   // name found
                  }
                  return false;    
                });
          }};
}

