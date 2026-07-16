define void @foo(ptr %out){

entry:
    %x = alloca i32
    store ptr %x, ptr %out
    ret void
}

; CHECK-LABEL: void @foo(ptr %out)
; CHECK: %x = alloca i32
; CHECK: store ptr %x, ptr %out
; CHECK-NEXT: ret void
