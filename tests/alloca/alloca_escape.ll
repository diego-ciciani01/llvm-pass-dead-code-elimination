define void @foo(ptr %out){

entry:
    %x = alloca i32
    store ptr %x, ptr %out
    ret void
}
