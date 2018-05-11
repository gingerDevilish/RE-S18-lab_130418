sub         esp,28h
call 	    $ + 5
pop 	    ebx
add 	    ebx, 1 + 1 + 1 + 1
lea         ecx,[ebx+(40h-23h)]
call 	    $ + 5
pop 	    ebx
add 	    ebx, 1 + 1 + 1 + 1
call        dword[ebx-(25h)]
xor         ecx, ecx
or          edx, 0FFFFFFFFh
test        eax, eax
cmove       ecx, edx
mov         eax, ecx
add         esp, 28h
ret
