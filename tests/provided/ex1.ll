define i32 @main(i32 %argc, ptr %argv) {
  %add = add nsw i32 %argc, 42
  %mul = mul nsw i32 %add, 2
  ret i32 0
}


; CHECK-LEBEL: define i32 @main(i32 %argc, ptr %argv)
; CHECK-NOT: %add = add nsw
; CHECK-NOT: %mul = mul nsw
; CHECK: ret i32 0
