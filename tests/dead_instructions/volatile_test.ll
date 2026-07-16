define void @foo(ptr %p){

entry:
    %x1 = load i32, ptr %p
    ;should be keeped
    %x2 = load volatile i32,  ptr %p
    ret void
}

; CHECK-LABEL: define void @foo(ptr %p)
; CHECK-NOT: %x1 = load i32, ptr %p
; CHECK: %x2 = load volatile i32, ptr %p
; CHECK: ret void
