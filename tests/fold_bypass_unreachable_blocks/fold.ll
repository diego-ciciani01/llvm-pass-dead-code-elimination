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
