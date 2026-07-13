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

/*
 * This function is used to verify if the istruction is basicaly dead
 * 'mayHaveSideEffects()' -> this single method can cover: volatile operations and atomic
 */
bool isTriviallyDead(Instruction &I){
  if (I.use_empty() && !I.isTerminator() && !I.mayHaveSideEffects())
    return true;

  return false;
}

namespace {
  /* 'PassInfoMixin': is a required pass, with CRTP pattern */
  struct MyDCEPass: PassInfoMixin<MyDCEPass>{

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM){
      llvm::SmallVector<Instruction *, 64> worklist;
      bool changed = false; /* Used to control the type of return */
      
      /* SEED phase: check the already dead and fill the stack. */
      for (BasicBlock &BB : F){
	for (Instruction &I : BB)
	  if (isTriviallyDead(I))
	    worklist.push_back(&I);	
      }

      /*
       * The apporach used is the worklist.
       * Means save in this stak each eligible dead instrcution.
       * WorkList approch has invariant property: every instruction enters the worklist at most once.
       *
       */

      while(!worklist.empty()){
	/* Get back the value from stack */
	Instruction *I = worklist.pop_back_val();
	SmallVector<Instruction *, 4> operands;
	
	/* 'users' handle the pointer to the operands of isturction, that we're processing. */
	for (Use &user : I->operands()){
	  /* Here we want to check if the operands are "pointers" eligitable, the if statement will return
	   * true, otherwise false.
	   * Eg.  mul nsw i32 %add, 2 
	   */
	  if (Instruction *Option =  dyn_cast<Instruction>(user.get())){
	    if (Option != nullptr)
	      operands.push_back(Option);
	  }
	}

	/* Deallocate the current I istructtion from graph and free the memory */
	I->eraseFromParent();
	changed = true;
	
	/* Now since I it's been eliminated, it's operands could be become dead too*/
	for (Instruction *O : operands){
	  if (isTriviallyDead(*O)){ /* Check also if is not present in the worklist*/
	      worklist.push_back(O);
	    
	  }
	  
	}
	
      }
	
      return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

  };
}


extern "C" LLVM_ATTRIBUTE_WEAK  PassPluginLibraryInfo llvmGetPassPluginInfo() {
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

