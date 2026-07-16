define void @foo(){
entry:
    %x = alloca i32
    store i32 10, ptr %x
    ret void;
}

; CHECK-LABLE: define void @foo()
; CHECK: entry
; CHECK-NOT: alloca
; CHECK-NEXT: ret void
