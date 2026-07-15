declare void @foo(i32)

define void @f(i32 %a) {
entry:
  %c = icmp sgt i32 %a, 0

  br i1 %c, label %x, label %x
x:
  call void @foo(i32 %a)
  ret void
}
