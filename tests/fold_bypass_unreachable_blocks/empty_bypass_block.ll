define void @foo(){

entry:
    br label %next

next:
    br label %target

target:
    ret void
}

; CHECK-LABEL: define void @foo()
; CHECK-NOT: br label %next
; CHECK: br label %target
; CHECK: ret void
