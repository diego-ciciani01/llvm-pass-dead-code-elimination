define void @foo(){

entry:
    %x = alloca i32
    store volatile i32 10, ptr %x
    ret void
}
