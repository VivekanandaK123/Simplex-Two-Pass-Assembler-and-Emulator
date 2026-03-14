; ============================================================
; Binary Search on a Sorted Array
; Student : Vivekananda Katakam  |  Roll: 2401CS52
; ============================================================
;
; WHAT IT DOES:
;   Searches a sorted integer array for a target value using
;   binary search.  Writes the found index to 'result', or
;   -1 if the target is not in the array.
;
; ALGORITHM:
;   low  = 0
;   high = length - 1
;   while low <= high:
;       mid = (low + high) >> 1    ; right-shift = divide by 2
;       v   = arr[mid]
;       if v == target : return mid
;       if v <  target : low  = mid + 1
;       else           : high = mid - 1
;   return -1
;
; WHY shr FOR DIVISION?
;   SIMPLEX has no divide instruction.  For non-negative
;   integers, (x >> 1) == (x / 2), so shr 1 gives the
;   midpoint cheaply and correctly.
;
; TEST VALUES (sorted ascending):
;   arr    = { 3, 7, 12, 19, 25, 38, 46, 57, 68, 74 }
;   length = 10
;
;   target = 25  →  result = 4   (0-based index, found)
;   target = 99  →  result = -1  (not in array)
;   target = 3   →  result = 0   (first element)
;   target = 74  →  result = 9   (last element)
;
; STACK FRAME LAYOUT (adj -3 gives 3 local slots):
;   SP+0 = low
;   SP+1 = high
;   SP+2 = mid   (recomputed each iteration)
; ============================================================

; ----- initialise stack pointer to 0x1000 -----
        ldc     0x1000
        a2sp

; ----- allocate 3 locals: low, high, mid -----
        adj     -3

; ----- low = 0 -----
        ldc     0
        stl     0           ; SP+0 = low = 0

; ----- high = length - 1 -----
        ldc     length      ; A = address of length word
        ldnl    0           ; A = 10
        adc     -1          ; A = 9
        stl     1           ; SP+1 = high = 9

; Loop: while (low <= high)
;   Equivalent check: exit if (high - low) < 0
loop:
        ldl     1           ; A = high
        ldl     0           ; B = high, A = low   [ldl pushes old A to B]
        sub                 ; A = B - A = high - low
        brlz    notfound   ; high - low < 0  →  low > high  →  not found

; ---- mid = (low + high) >> 1 -----
;   Step 1: A = low + high
        ldl     0           ; A = low
        ldl     1           ; B = low,  A = high
        add                 ; A = low + high   [add: A = B + A]
;   Step 2: mid = A >> 1
;   shr computes: A = B >> A, so we need B = (low+high), A = 1
        ldc     1           ; B = (low+high),  A = 1
        shr                 ; A = (low+high) >> 1 = mid
        stl     2           ; SP+2 = mid

; ----- v = arr[mid] -----
;   Address of arr[mid] = arr + mid
        ldc     arr         ; A = base address of array
        ldl     2           ; B = arr,  A = mid
        add                 ; A = arr + mid
        ldnl    0           ; A = memory[arr + mid] = arr[mid]
;   Save arr[mid] into a temp on the stack for comparison.
;   We have no free local slot, so use adj to open one:
        adj     -1          ; open temp slot → SP+0 = temp
                            ;   old locals shift: low=SP+1, high=SP+2, mid=SP+3
        stl     0           ; SP+0 = arr[mid]  (saved)

; ----- load target value -----
        ldc     target      ; A = address of target word
        ldnl    0           ; A = target value

; ----- compare: target - arr[mid] -----
;   sub computes A = B - A.  We need target - arr[mid].
;   So: B must = arr[mid], A must = target.
;   Load arr[mid] from SP+0 (it was just stored there):
        ldl     0           ; B = target,  A = arr[mid]
;   Oops: ldl pushes old A (target) into B, gives A = arr[mid].
;   Now A = arr[mid], B = target.  sub gives target - arr[mid]. 
        sub                 ; A = target - arr[mid]

; ----- restore stack (close temp slot) -----
        adj     1           ; SP back to 3-local layout
                            ;   low=SP+0, high=SP+1, mid=SP+2

; ----- branch on comparison result -----
        brz     found       ; arr[mid] == target → found
        brlz    goleft    ; target - arr[mid] < 0 → target < arr[mid]

; ----- arr[mid] < target: low = mid + 1 -----
goright:
        ldl     2           ; A = mid
        adc     1           ; A = mid + 1
        stl     0           ; low = mid + 1
        br      loop

; ----- arr[mid] > target: high = mid - 1 -----
goleft:
        ldl     2           ; A = mid
        adc     -1          ; A = mid - 1
        stl     1           ; high = mid - 1
        br      loop


; Found: result = mid
found:
        ldl     2           ; A = mid (found index)
        ldc     result
        stnl    0           ; memory[result] = mid
        adj     3           ; restore stack
        HALT

; Not found: result = -1
notfound:
        ldc     -1
        ldc     result
        stnl    0           ; memory[result] = -1
        adj     3           ; restore stack
        HALT

; Data section
length: data    10          ; array length

target: data    19          ; search target  

result: data    0           ; output written here by the program

; Sorted array — must be strictly ascending for binary search:
arr:    data    3           ; index 0
        data    7           ; index 1
        data    12          ; index 2
        data    19          ; index 3
        data    25          ; index 4  
        data    38          ; index 5
        data    46          ; index 6
        data    57          ; index 7
        data    68          ; index 8
        data    74          ; index 9