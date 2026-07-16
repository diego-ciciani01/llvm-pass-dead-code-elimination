; ModuleID = 'ex1.ll'

@.str = private unnamed_addr constant [15 x i8] c"Hello, world!\0A\00", align 1

define i32 @main(i32 %argc, ptr %argv) #0 {
  %add = add nsw i32 %argc, 42
  %mul = mul nsw i32 %add, 2
  %cmp = icmp sgt i32 %mul, 0
  br i1 %cmp, label %cond.true, label %cond.false

cond.true:                                        ; preds = %entry
  br label %cond.end

cond.false:                                       ; preds = %entry
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %mul, %cond.true ], [ %argc, %cond.false ]
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1


; CHECK-LABEL: define i32 @main(i32 %argc, ptr %argv)
; CHECK-NOT: add = add nsw
; CHECK-NOT: mul = mul nsw
; CHECK-NOT: cmp = icmp sgt
; CHECK-NOT: br i1 %cmp label cond.true, label cond.false
; CHECK: br label %cond.end
; CHECK-NOT: cond = phi i32
; CHECK: call = call i32
; CHECK: ret i32 0
