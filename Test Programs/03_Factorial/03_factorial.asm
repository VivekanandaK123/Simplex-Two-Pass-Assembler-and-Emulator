; ============================================================
; Recursive Factorial in SIMPLEX Assembly
; Student : Vivekananda Katakam  |  Roll: 2401CS52
; ============================================================
;
; WHAT IT DOES:
;   Computes n! recursively using the fact() function.
;   Multiplication is done inline via repeated addition
;   (SIMPLEX has no multiply instruction).
;
; ALGORITHM:
;   fact(n):
;       if n <= 1: return 1
;       else:      return n * fact(n-1)
;
;   multiply(n, x):         where x = fact(n-1)
;       product = 0
;       repeat n times: product += x
;       return product
;
; INPUT / OUTPUT:
;   count  = n   (data word, set to 5)
;   result = n!  (data word, written by main code)
;   Expected: 5! = 120 = 0x78
;
; CALL CONVENTION:
;   'call' in SIMPLEX sets A = return address, B = old A (the argument).
;   So on entry to fact(): A = ret_addr, B = n.
;   stl 1 saves ret_addr (A → SP+1, A becomes B = n).
;   stl 2 saves n        (A → SP+2, A becomes B = garbage).
;
; STACK FRAME LAYOUT inside fact() (adj -5 reserves 5 slots):
;   SP+0 = product       (accumulator for repeated addition)
;   SP+1 = ret_addr      (return address, saved on entry)
;   SP+2 = n             (argument, saved on entry)
;   SP+3 = counter       (counts down from n during multiply)
;   SP+4 = (n-1)!        (result of recursive call, saved before multiply)
;
; NOTE ON ldl PUSH BEHAVIOUR:
;   Every ldl instruction does: B = A (push), then A = mem[SP+offset].
;   This is exploited in the multiply loop:
;       ldl 0   →  B = product (old A),  A = product (from stack)
;       ldl 4   →  B = product,          A = (n-1)!
;       add     →  A = product + (n-1)!
; ============================================================

; ----- initialise stack pointer to 0x1000 -----
        ldc  0x1000
        a2sp

; ----- load n and call fact(n) -----
        ldc  count          ; A = address of count word
        ldnl 0              ; A = memory[count] = n
        call fact           ; A = ret_addr, B = n  →  returns with A = n!

; ----- store result: memory[result] = n! -----
;   stnl 0 semantics: memory[A] = B
;   After call returns: A = n!, need A = result address, B = n!
        ldc  result         ; B = n!,  A = address of result word
        stnl 0              ; memory[result] = n!
        HALT

; ============================================================
; fact(n)
; Entry:  A = ret_addr  (set by 'call'),  B = n  (caller's A)
; Exit:   A = n!,       B = preserved caller B  (via return)
; ============================================================
fact:   adj  -5             ; allocate 5 local slots (SP+0 .. SP+4)
        stl  1              ; SP+1 = ret_addr,  A := B = n
        stl  2              ; SP+2 = n,          A := B (stale, not used)

; ----- base case: if n <= 1, return 1 -----
;   check n-1 < 0  (i.e. n < 1)  and  n-1 == 0  (i.e. n == 1)
        ldl  2              ; A = n
        adc  -1             ; A = n - 1
        brlz base           ; n - 1 < 0  →  n < 1  →  return 1
        brz  base           ; n - 1 = 0  →  n = 1  →  return 1

; ----- recursive call: fact(n-1) -----
        ldl  2              ; A = n
        adc  -1             ; A = n - 1
        call fact           ; recurse: returns with A = (n-1)!
        stl  4              ; SP+4 = (n-1)!

; ----- multiply: product = n * (n-1)! via repeated addition -----
;   product = 0, counter = n
;   loop: product += (n-1)!  repeated n times
        ldc  0
        stl  0              ; SP+0 = product = 0
        ldl  2
        stl  3              ; SP+3 = counter = n

; ----- multiply loop -----
mloop:  ldl  3              ; A = counter
        brz  mdone          ; counter == 0 → multiplication done

        ldl  3              ; A = counter
        adc  -1             ; A = counter - 1
        stl  3              ; SP+3 = counter--

        ldl  0              ; B = (stale),   A = product      ← push product to B
        ldl  4              ; B = product,   A = (n-1)!       ← key: B=product via push
        add                 ; A = B + A = product + (n-1)!
        stl  0              ; SP+0 = updated product
        br   mloop

; ----- multiply done: return n! -----
mdone:  ldl  0              ; A = final product = n!
        ldl  1              ; B = n!,  A = ret_addr            ← push n! to B
        adj  5              ; deallocate frame
        return              ; PC = A = ret_addr,  A = B = n!

; ----- base case return: fact(0) = fact(1) = 1 -----
base:   ldc  1              ; A = 1
        ldl  1              ; B = 1,  A = ret_addr             ← push 1 to B
        adj  5              ; deallocate frame
        return              ; PC = A = ret_addr,  A = B = 1

; ============================================================
; DATA SECTION
; ============================================================
count:  data 6              
result: data 0              
                            