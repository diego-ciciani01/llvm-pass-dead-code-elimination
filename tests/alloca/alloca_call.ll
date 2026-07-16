declare void @foo(ptr)

define void @f(){
entry:
    %x = alloca i32
    store i32 10, ptr %x
    call void @foo(ptr %x)
    ret void
}

; CHECK-LABEL: void @f()
; CHECK: %x = alloca i32
; CHECK: store i32 10, ptr %x
; CHECK-NEXT: call void @foo(ptr %x)
; CHECK-NEXT: ret void

