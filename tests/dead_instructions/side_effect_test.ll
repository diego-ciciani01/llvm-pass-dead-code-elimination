declare void @foo1(i32)
declare i32 @foo2()

define void @f(ptr %p){
entry:
    %x = add i32 1, 2

    store i32 42, ptr %p
    %c = call i32 @foo2()
    call void @foo1(i32 %c)
    ret void
}
