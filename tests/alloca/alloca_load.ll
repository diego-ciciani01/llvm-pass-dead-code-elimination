define i32 @foo() {

entry:
    %x = alloca i32
    store i32 10, ptr %x
    %v = load i32, ptr %x
    ret i32 %v
}

; CHECK-LABEL: i32 @foo()
; CHECK: %x = alloca i32
; CHECK: store i32 10, ptr %x
; CHECK: %v = load i32, ptr %x
; CHECK-NEXT: ret i32 %v

