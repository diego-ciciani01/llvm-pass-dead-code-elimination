define void @foo(){

entry:
    br label %next

next:
    br label %target

target:
    ret void
}
