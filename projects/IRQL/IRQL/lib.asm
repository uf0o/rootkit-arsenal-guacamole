.CODE
public NOP_FUNC 
public SHARED_FUNC

NOP_FUNC PROC
    nop
    ret
NOP_FUNC ENDP

SHARED_FUNC PROC
    xchg rax, rbx
    xchg rbx, rax
    xchg rax, rbx
    xchg rbx, rax
    xchg rax, rbx
    xchg rbx, rax
    xchg rax, rbx
    xchg rbx, rax
    ret
SHARED_FUNC ENDP
END


