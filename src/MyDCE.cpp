
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

#if __has_include("llvm/Plugins/PassPlugin.h")
#include "llvm/Plugins/PassPlugin.h"   /* LLVM >= 22 */
#else
#include "llvm/Passes/PassPlugin.h"    /* LLVM <= 21 */
#endif

/* Requred to find the 'include' files from llvm namespace*/
using namespace llvm;

/*
 * Return true if the Instruction is trivialy dead:
 * no users, is not a terminator and has not apparent side effects.
 * 
 * "mayHaveSideEffects()" check: memory write, volatile access, atomic operations, calls etc ...
 */
static bool isTriviallyDead(Instruction &I){
  if (I.use_empty() && !I.isTerminator() && !I.mayHaveSideEffects())
    return true;

  return false;
}

static bool deadCodeElimination(Function &F){
  llvm::SmallVector<Instruction *, 64> worklist;
  bool changed = false; /* Used to control the type of return */
      
  /* SEED phase: check the already dead and fill the stack. */
  for (BasicBlock &BB : F){
    for (Instruction &I : BB)
      if (isTriviallyDead(I))
	worklist.push_back(&I);	
  }
  
  /*
   * Worklist-base dead eliminatio.
   *
   * Initialy used like seed for the first trivialy dead instruction.
   * If an instruction is removed, it's operands may become candidate to be trivialy dead as well
   * and since reconsidered. 
   */
  while(!worklist.empty()){
    /* Get back the value from stack */
    Instruction *I = worklist.pop_back_val();
    SmallVector<Instruction *, 4> operands;
	
    /* 'users' handle the pointer to the operands of isturction, that we're processing. */
    for (Use &Op: I->operands()){
      /* Here we want to check if the operands are "pointers" eligitable, the if statement will return
       * true, otherwise false.
       * Eg.  mul nsw i32 %add, 2 
       */
      if (Instruction *OptInst =  dyn_cast<Instruction>(Op.get()))
	operands.push_back(OptInst);
    }

    /* Remove the current "I" istructtion from it's parents basic block.  */
    I->eraseFromParent();
    changed = true;
	
    /* Now since I it's been eliminated, it's operands could be become dead too*/
    for (Instruction *O : operands){
      if (isTriviallyDead(*O))
	worklist.push_back(O);  
    }	
  }

  return changed;
  
}

/*
 * Fold conditional branches whose true and false successors
 */
static bool foldSameTargetBranches(Function &F){
  bool changed = false;
  
  for (BasicBlock &BB : F){
    if (Instruction *I =  BB.getTerminator()){

      if (BranchInst *BI = dyn_cast<BranchInst>(I)){
	/* Check the case in with branch has the same destination*/
	if (BI->isConditional() && BI->getSuccessor(0) == BI->getSuccessor(1)){
	  if (!BI->getSuccessor(0)->phis().empty())
	    continue;
	  /* now create the new "br"*/
	  BranchInst::Create( BI->getSuccessor(0), BI->getIterator());
	  /* Delete the old incoditional branch */
	  BI->eraseFromParent();
	  changed = true;
	}
	
      }

    }
      
  }

  return changed;
}

/*
 * Remove unrechable basic blocks by performing DFS
 * from the entire block 
 */
static bool removeUnreachableBlocksLocal(Function &F){
 
  SmallVector<BasicBlock*, 16> stack; /* stack with, to delete blocks */
  SmallPtrSet<BasicBlock*, 16> checked; /* set with all 'healty' block */
  
  for (BasicBlock *BB : depth_first(&F.getEntryBlock())) 
    checked.insert(BB);
  
  /* Now check all the nodes in the set, that are not reachable */
  for (BasicBlock &BB : F)
    if (!checked.count(&BB))
      stack.push_back(&BB);

  /* In the case in witch all the block are not reachable, exit*/
  if (stack.empty())
    return false;

  
  /* Remove all the link between the them and cleaning */
  for (BasicBlock *BB : stack){
    for (BasicBlock *suc : successors(BB)){
      /* Situation in witch the successor is an haltly block */
      if (checked.count(suc)){ 
	for (PHINode &PN : suc->phis()){
	  /* Remove the ingress associated with BB
	   * False params, avoid to delete the phi with one ingress. It's will be eliminated in the next loop, by DCE.
	   */
	  PN.removeIncomingValue(BB, false);
	}
      }
    }
    BB->dropAllReferences();
  }

  /* Phisical elimination memory. */
  for (BasicBlock *BB : stack)
    BB->eraseFromParent();
  
  return true;
    
}

/*
 * Conservatly avoid bypassing blocks whose successors contain PHI node.
 *
 * Preserved the corrispondence between incoming CFG edges and PHI operands 
 */
static bool bypassBlock(Function &F){
  bool changed = false;
  
  for (BasicBlock &BB : F){
    /* Check if the block contains only the terminator istruction
     * Uncoditional branch, and eventualy PHI
    */
    if (BB.size() != 1)
      continue;

    /* Here there is not previus block ... */
    if (&BB == &F.getEntryBlock())
      continue;

    
    /*Get last istruction of block, and cast it in "BranchInst" */
    BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator());
    if (!BI || !BI->isUnconditional())
      continue;

    /* Successor of the unconditional branch */
    BasicBlock *suc = BI->getSuccessor(0);
    
    /* Check self loop*/
    if (suc == &BB)
      continue;

    /* bail-out to prevent problem with SSA semantic
     * if the successor has PHI node
     */
    if (!suc->phis().empty())
      continue;

    SmallVector<BasicBlock *, 16> preds(predecessors(&BB));
        
    /* Redirect predecessors */
    for (BasicBlock *pred : preds)
      pred->getTerminator()->replaceSuccessorWith(&BB, suc);

    changed = true;
  }
  
  return changed;
}


/*
 * Eliminate the stack slot addres whose never escape, and
 * are never read from.
 *
 * Conservately accpet only stores and writing directly into allca.
 * Any other cases are trated like escape.
 */
static bool eliminateDeadAllocas(Function &F){
  SmallVector<AllocaInst *, 16> allocas;
  bool changed = false;

  for (BasicBlock &BB : F)
    for (Instruction &I : BB )
      if (AllocaInst *AI = dyn_cast<AllocaInst>(&I))
	allocas.push_back(AI);
  
  for (AllocaInst *AI : allocas){
    bool removable = true;
    SmallVector<StoreInst *, 8> stores;

    for (User *U : AI->users()){
      if (StoreInst *SI = dyn_cast<StoreInst>(U)){
	
	/* Only acceptable user
	 * Check - if the i-th (SI) value, respect:
	 * 1) destination of writing.
	 * 2) we're not self saving the i-th value, eg -> store ptr %x, ptr %x
	 * 3) it's not volatile. 
	 */
	if (SI->getPointerOperand() == AI && SI->getValueOperand() != AI && !SI->isVolatile()){
	  stores.push_back(SI);
	  continue;
	}
      }

      /* Anything else: load, call, GEP, store-escape, or store where alloca is value operand,
       * that make the adddres observable - bail - out conservative. 
       */
      removable = false;
      break;
    }
    
    if (!removable)
      continue;

    for(StoreInst *SI : stores)
      SI->eraseFromParent();

    AI->eraseFromParent();

    changed = true;
  }

  return changed; 
}

namespace {
  /* 'PassInfoMixin': is a required pass, with CRTP pattern. */
  struct MyDCEPass: PassInfoMixin<MyDCEPass>{

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM){
      bool changed = false ;
      bool localyChanged = false;
      /* Main loop
       * 
       * Iterate until a fixed point is reached
       */
      do{
	changed = deadCodeElimination(F);
	changed |= foldSameTargetBranches(F);
	changed |= removeUnreachableBlocksLocal(F);
	changed |= bypassBlock(F);
	changed |= eliminateDeadAllocas(F);
	
	if(changed)
	  localyChanged = true;
      }while(changed);

                  
      return localyChanged ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
    static bool isRequired() { return true; }
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
                    return true;  
                  }
                  return false;    
                });
          }};
}

