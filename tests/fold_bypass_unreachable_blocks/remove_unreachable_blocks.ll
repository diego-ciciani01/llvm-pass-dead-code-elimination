define i32 @foo(i32 %a){
entry:
    ret i32 %a
dead1:
    %x = add i32 %a, 1
    br label %dead2
dead2:
    ret i32 %x

}

; CHECK-LABEL: define i32 @foo(i32 %a)
; CHECK: ret i32 %a
; CHECK-NOT: %x = add
; CHECK-NOT: br label %dead2
; CHECK-NOT: ret i32 %x

