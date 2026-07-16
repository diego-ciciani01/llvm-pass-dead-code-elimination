# Dead Code Elimination - LLVM Pass

A dead code elimination pass for llvm, implemented as a loadable plugin for the Pass Menager. The pass implement three families of optimization, that reinforce them self through a fixedpoint driver:

1. **Trivialy Dead Instructions** - remove istruction whose result it's unused and tha has no observable side-effect.
2. **CFG - Semplification** - folds conditional branches with identical target, bypasses empty blocks, and remove unreachable blocks.
3. **Dead "alloca" Istruction** - removes 'alloca' whose address never escape and that are never read.

the pass is registered with the name 'my-dce'.

## Requirements

- LLVM **22** (developed and tested with Homebrew LLVM 22.1.15 on macOS arm64)

## Building
```sh
cmake -B build \
  -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm \
  -DCMAKE_CXX_COMPILER=$(brew --prefix llvm)/bin/clang++
cmake --build build
```
This produce 'build/libMyDCE.so'

Notes:
- `-DLLVM_DIR` points CMake at the `LLVMConfig.cmake` installed by Homebrew, so include path discovered automaticaly.
- `-DCMAKE_CXX_COMPILER` force the Homebrew Clang so that the compiler and the LLVM headers come frome the same toolchain.

## Usage
```sh
    opt -load-pass-plugin build/libMyDCE.so -passes='my-dce' -S input.ll
```
Running the verifier after the pass, raccomanded

```sh
    opt -load-pass-plugin build/libMyDCE.so -passes='my-dce,verify' -S input.ll
```

## Design
The three optimization are not independent: each one can contribute for the otimization of the next for the next others.
Inside of `do while()`, the iterations continue untill `changed` variable does not return `false` value, "nothing left to optimize".
```C
  do{
	changed = deadCodeElimination(F);
	changed |= foldSameTargetBranches(F);
	changed |= removeUnreachableBlocksLocal(F);
	changed |= bypassBlock(F);
	changed |= eliminateDeadAllocas(F);

	if(changed)
	  localyChanged = true;
  }while(changed);
```
This driver work with bitwise operation, in this case 'or'.
`|=` check the boolean result and assign it to `changed`.

`PreservedAnalyses::none()` is returned when anything changed, `all()` otherwise.

### 1. Trivially dead instruction
An instruction is *trivially dead* when it has no users, is not a terminator and has no observable side-effect.
The side-effect are handled by the `mayHaveSideEffects()`, which covers stores, call and volatile/tomic operations.
A **non-volatile load with no users is removed** while a volatile load is correctly kept.

The trasformations uses a **worklist**. A seed scan that collects the istructions that are already dead;
then if the an instruction is correctly erased, the operand of the that instruction could  becoming eligible to be erased in the next iteration.

**Know limitation:** dead PHI cycles (phi that only feed each other across a loop ) are not detected.
Handling ther require a dedicated approch, which is out of scope.

### 2. CFG simplification
There are three separated transformations:

- **Fold same target branch** A conditional branch whose two successors are the same block is replaced by an uncoditional branch.
The condition then becomes dead and is collected by phase 1 on the next iteratin.

- **Remove unreachable blocks** Reachability of blocks is tested with depth-firs-search. Unreachable block may still be referanced, so before erasing them:
(a) `removeIncomingValue` clean the PHI node from any *live* successors thet listed dead block as predecessor, and (b)  `dropAllReferences`
break cross-references between dead blocks, doing this before the earesing of any block.

- **Bypass empty blocks** A block containing only an unconditional branch is removed, by redirectoring it's predecessors to it's
direct successors.

Guards:  the entry block is never bypasses, self-loop are skipped, and the transformation **bails out if the succerros contain PHI nodes**.
Corretly retargeting into a successor with PHI requires updating the incoming values preserving the edge/value correspondence, and
in particular, deduplicating edges when a predecessor of the bypassed block is already a predecessor of the successor with a different values,
a case in witch the trasformation clould be illega.

### 3. Dead stack "allocas"
An `alloca` is eliminates when **every** users are "harmless" store: a `StoreInst` where the alloca is the *pointer* operand,
is *not* the value operand (so it's address not escape, e.g `store ptr %x, ptr %x`) and it's not volatile.
Any other user - a load, a call( the address escapes ), a store that stores the address, a GEP or a bitcast,
make the pass to keep the alloca, it's a conservative-bail out.

When an alloca is removed, its stores are arased first (they use the alloca, so must be empty before it can be erased).
Erasing a store can  make the stored value dead, which phase 1 will collects on the next iteration.

**Know limitation:** allocas whose address flows through a GEP or a bitcast are conservatively kept.


### Future Optimizzation
**Block Merging** On the provided `ex3`, the pass leave an unconditional branch to separate block, this case is suitable for
Block Mering, but was not among the requirements. - TO DO


## Testing
There are LLVM IR files under `tests/`, grouped by the requirement they exercise.
Each was chacked to parse and pass the verifier

### Proviede Integration test `tests/provied/`
The three file attached to the challange.
each collapse to `ret` + `printf`:

- `ex1.ll` - dead instruction chain (Phase 1).
- `ex2.ll` - dead PHI + empty blocks + same-target branch; use the phase 1 + phase 2
- `ex3.ll` -  the full phases starting from: dead allocas -> DCE -> blocks bypass and branch folding.

### Unit tests
- ***Phase 1** (tests/dead_instructions/): a side-effect test(store, call, and their result-consumers survive while a dead arithmetic instruction is removed)
a dead volatile test (a non-volatile unused load is removed, a volatile one is kept).
- **Phase 2** (tests/fold_bypass_unreachable_blocks/): same - target branch fold, two empty blocks whose common successor holds a live PHI (the bypass must leave theam untached), an unreachable-block test with cross referances with two dead blocks
and an empty-block bypass (fold.ll, bypass_phi_bail.ll, remove_unreachable_blocks.ll, empty_bypass_block.ll).
- **Phase 3** (tests/alloca/): five cases - store-only (removed), read by a load (kept), passed to a call (kept), address stored as a value operand (kept), and volatile store (kept).

### Test Structure
Each test files carries, comment like: `CHECK:` and `CHECK-NOT:` are directives, used by `FileCheck` tool.
Example:

```ll
declare void @foo(i32)

define void @f(i32 %a) {
entry:
  %c = icmp sgt i32 %a, 0

  br i1 %c, label %x, label %x
x:
  call void @foo(i32 %a)
  ret void
}

; CHECK-LABEL: define void @f(i32 %a)
; CHECK-NOT: %c = icmp sgt i32 %a, 0
; CHECK: br label %x
; CHECK: call void @foo(i32 %a)
; CHECK: ret void

```

Note: the execution of this test can be done also through helper script, that run all the tests.
```sh
./run_tests.sh
```

