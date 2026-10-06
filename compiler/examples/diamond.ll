define i32 @diamond(i1 %c, i32 %x) {
entry:
  br i1 %c, label %left, label %right
left:
  %a = add i32 %x, 1
  br label %join
right:
  %b = sub i32 %x, 1
  br label %join
join:
  %p = phi i32 [ %a, %left ], [ %b, %right ]
  ret i32 %p
}
