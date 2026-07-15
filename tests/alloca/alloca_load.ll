define i32 @foo() {

entry:
    %x = alloca i32
    store i32 10, ptr %x
    %v = load i32, ptr %x
    ret i32 %v
}
