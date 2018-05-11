sub         rsp,28h
lea         rcx,[RIP+(38h-1Bh)]
call        qword ptr[RIP-(21h-0)]
xor         ecx, ecx
or          edx, 0FFFFFFFFh
test        rax, rax
cmove       ecx, edx
mov         eax, ecx
add         rsp, 28h
ret