; ============================================================
; Bubble Sort in SIMPLEX Assembly
; Student : Vivekananda Katakam  |  Roll: 2401CS52
; ============================================================
;
; WHAT IT DOES:
;   Sorts an integer array in ascending order using bubble sort.
;   Each outer pass bubbles the largest unsorted element to the end.
;   Inner pass compares adjacent elements and swaps if out of order.
;
; ALGORITHM:
;   for i = 0 to n-1:
;       for j = 0 to (n-i-1)-1:
;           if arr[j] > arr[j+1]: swap(arr[j], arr[j+1])
;
; TEST VALUES:
;   arr   = { 42, 7, 19, 3, 55, 1, 28, 14 }
;   count = 8
;   Expected output (ascending): { 1, 3, 7, 14, 19, 28, 42, 55 }
;
; STACK FRAME LAYOUT (adj -7 gives 7 local slots):
;   SP+0 = arr base address
;   SP+1 = n (array length)
;   SP+2 = i (outer loop counter)
;   SP+3 = j (inner loop counter)
;   SP+4 = arr[j]   (cached for comparison and swap)
;   SP+5 = arr[j+1] (cached for comparison and swap)
;   SP+6 = temp address (used as scratch during stnl writes)
; ============================================================

; ----- initialise stack pointer to 0x1000 -----
        ldc 0x1000
        a2sp

; ----- allocate 7 locals -----
        adj -7

; ----- SP+0 = base address of array -----
        ldc arr
        stl 0

; ----- SP+1 = n = count (loaded from memory via ldnl) -----
        ldc count           ; A = address of count word
        ldnl 0              ; A = memory[count] = 8
        stl 1

; ----- SP+2 = i = 0 (outer loop counter) -----
        ldc 0
        stl 2

; ============================================================
; OUTER LOOP: repeat while i < n
;   Exit condition: (n - i) <= 0
; ============================================================
outer:
        ldl 1               ; A = n        (goes to B on next ldl)
        ldl 2               ; B = n, A = i
        sub                 ; A = n - i
        brz  done           ; n - i == 0 → all passes done
        brlz done           ; n - i <  0 → shouldn't happen, guard anyway

; ----- SP+3 = j = 0 (inner loop counter, reset each outer pass) -----
        ldc 0
        stl 3

; ============================================================
; INNER LOOP: repeat while j < (n - i - 1)
;   Exit condition: (n - i - 1 - j) <= 0
; ============================================================
inner:
        ldl 1               ; A = n
        ldl 2               ; B = n,     A = i
        sub                 ; A = n - i
        adc -1              ; A = n - i - 1   (upper bound for j)
        ldl 3               ; B = n-i-1, A = j
        sub                 ; A = (n-i-1) - j
        brz  nextouter      ; j reached upper bound → next outer pass
        brlz nextouter      ; j exceeded upper bound → next outer pass

; ----- cache arr[j] into SP+4 -----
;   address of arr[j] = arr_base + j
        ldl 0               ; A = arr base
        ldl 3               ; B = arr base, A = j
        add                 ; A = arr base + j
        ldnl 0              ; A = memory[arr + j] = arr[j]
        stl 4               ; SP+4 = arr[j]

; ----- cache arr[j+1] into SP+5 -----
;   address of arr[j+1] = arr_base + j + 1
        ldl 0               ; A = arr base
        ldl 3               ; B = arr base, A = j
        adc 1               ; A = j + 1
        add                 ; A = arr base + j + 1
        ldnl 0              ; A = memory[arr + j + 1] = arr[j+1]
        stl 5               ; SP+5 = arr[j+1]

; ----- compare arr[j] vs arr[j+1] -----
;   sub computes B - A, so load arr[j] first (→ B), arr[j+1] second (→ A)
;   result = arr[j] - arr[j+1]
;     < 0 : arr[j] < arr[j+1]  → already in order, no swap
;     = 0 : arr[j] == arr[j+1] → equal, no swap needed
;     > 0 : arr[j] > arr[j+1]  → out of order, swap!
        ldl 4               ; A = arr[j]   (pushed to B on next ldl)
        ldl 5               ; B = arr[j],  A = arr[j+1]
        sub                 ; A = arr[j] - arr[j+1]
        brlz noswap         ; arr[j] < arr[j+1], already ordered
        brz  noswap         ; arr[j] == arr[j+1], no swap needed
                            ; fall through: arr[j] > arr[j+1] → swap

; ============================================================
; SWAP arr[j] and arr[j+1]
;   Step 1: arr[j]   = arr[j+1]  (write SP+5 value to addr arr+j)
;   Step 2: arr[j+1] = arr[j]    (write SP+4 value to addr arr+j+1)
;   stnl 0 semantics: memory[A] = B
;     → load value first (goes to B), then load address (stays in A)
; ============================================================

; ----- Step 1: arr[j] = arr[j+1] -----
        ldl 0               ; A = arr base
        ldl 3               ; B = arr base, A = j
        add                 ; A = arr base + j  (destination address)
        stl 6               ; SP+6 = destination address (arr + j)
        ldl 5               ; A = arr[j+1]  (value to write, goes to B on next)
        ldl 6               ; B = arr[j+1], A = destination address (arr + j)
        stnl 0              ; memory[arr + j] = arr[j+1]  ✓

; ----- Step 2: arr[j+1] = arr[j] -----
        ldl 0               ; A = arr base
        ldl 3               ; B = arr base, A = j
        adc 1               ; A = j + 1
        add                 ; A = arr base + j + 1  (destination address)
        stl 6               ; SP+6 = destination address (arr + j + 1)
        ldl 4               ; A = arr[j]  (original value, goes to B on next)
        ldl 6               ; B = arr[j], A = destination address (arr + j + 1)
        stnl 0              ; memory[arr + j + 1] = arr[j]  ✓

; ----- increment j and repeat inner loop -----
noswap:
        ldl 3               ; A = j
        adc 1               ; A = j + 1
        stl 3               ; SP+3 = j++
        br inner

; ----- increment i and repeat outer loop -----
nextouter:
        ldl 2               ; A = i
        adc 1               ; A = i + 1
        stl 2               ; SP+2 = i++
        br outer

; ----- all passes done, array is sorted -----
done:
        HALT

; ============================================================
; DATA SECTION
; ============================================================
count:  data 8              ; number of elements in the array

arr:    data 42             
        data 7              
        data 19             
        data 3              
        data 55             
        data 1              
        data 28             
        data 14             
                            