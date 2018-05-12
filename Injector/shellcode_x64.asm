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

sub    esp,40h
push   ebx
push   esi
push   edi
call   $ + 5
pop    ebx
add    ebx,0x4
lea    eax,[ebx + (40h - 1Ch)]
call   $ + 5
pop    ebx
add    ebx,0x4
push   eax
call   DWORD PTR[ebx-25h]
xor    ecx,ecx
or	   edx,0xffffffff
test   eax,eax
cmove  ecx,edx
mov    eax,ecx
pop    edi
pop    esi
pop    ebx
add    esp,0x40h
ret