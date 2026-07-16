declare void @foo(i32)

define void @f(i32 %a){
entry:
    %cond = icmp sgt i32 %a, 0
    br i1 %cond, label %left, label %right

left:
    br label %join
right:
    br label %join

join:
    %p = phi i32 [ 7, %left ], [2, %right]
    call void  @foo(i32 %p)
    ret void
}

; CHECK-LABEL: define void @f(i32 %a)
; CHECK: %cond = icmp
; CHECK: br i1 %cond
; CHECK: phi
; CHECK: call
; CHECK: ret void
