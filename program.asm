ldi A 10
mov M A 0
ldi A 20
mov M A 1
mov A M 0
push A
mov A M 1
pop B
cmp A B
jnz IF_END_0
mov A M 0
push A
mov A M 1
pop B
add A B
mov M A 2
IF_END_0:
