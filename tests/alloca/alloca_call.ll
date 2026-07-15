; RUN: opt -load-pass-plugin %plugin -passes='my-dce,verify' -S %s | FileCheck %s
declare void @foo(ptr)

define void @f(){
entry:
    %x = alloca i32
    store i32 10, ptr %x
    call void @foo(ptr %x)
    ret void
}
