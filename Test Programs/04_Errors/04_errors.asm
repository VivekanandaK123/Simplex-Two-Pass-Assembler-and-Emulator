; ============================================================
; Sum of integers from 1 to N
; Student : Vivekananda Katakam  |  Roll: 2401CS52
; ============================================================

; Purpose : Test error AND warning detection in the assembler.
;           Sections group cases by error/warning class.
;           Expected diagnostic noted on each line.

; ─────────────────────────────────────────────────────────────
; SECTION 1 : DUPLICATE LABEL DEFINITIONS  (errors)
; ─────────────────────────────────────────────────────────────
alpha:
alpha:          ; error: duplicate label 'alpha'
beta: ldc 0
beta:           ; error: duplicate label 'beta'
gamma: ldc 1
gamma: ldc 2    ; error: duplicate label 'gamma'

; ─────────────────────────────────────────────────────────────
; SECTION 2 : INVALID / BOGUS LABEL NAMES  (errors)
; ─────────────────────────────────────────────────────────────
0start: ldc 0   ; error: bogus label name (starts with digit)
99bad:  ldc 1   ; error: bogus label name (starts with digit)
_foo:   ldc 2   ; error: bogus label name (starts with underscore)
bad-lbl: ldc 3  ; error: bogus label name (contains hyphen)

; ─────────────────────────────────────────────────────────────
; SECTION 3 : UNKNOWN / BOGUS MNEMONICS  (errors)
; ─────────────────────────────────────────────────────────────
fibble          ; error: unknown mnemonic 'fibble'
loadit 5        ; error: unknown mnemonic 'loadit'
MOV 1           ; error: unknown mnemonic 'MOV'
nop             ; error: unknown mnemonic 'nop'

; ─────────────────────────────────────────────────────────────
; SECTION 4 : MISSING OPERAND  (errors)
; ─────────────────────────────────────────────────────────────
ldc             ; error: 'ldc'  requires an operand
adc             ; error: 'adc'  requires an operand
ldl             ; error: 'ldl'  requires an operand
br              ; error: 'br'   requires an operand
brz             ; error: 'brz'  requires an operand

; ─────────────────────────────────────────────────────────────
; SECTION 5 : UNEXPECTED OPERAND  (errors)
; ─────────────────────────────────────────────────────────────
add 5           ; error: 'add'  takes no operand
sub 3           ; error: 'sub'  takes no operand
a2sp 1          ; error: 'a2sp' takes no operand
HALT 0          ; error: 'HALT' takes no operand

; ─────────────────────────────────────────────────────────────
; SECTION 6 : INVALID NUMBER FORMATS  (errors)
; ─────────────────────────────────────────────────────────────
ldc 08ge        ; error: invalid number '08ge'
adc 3.14        ; error: invalid number '3.14' (no floats)
ldl 0xGG        ; error: invalid number '0xGG'
stl 12abc       ; error: invalid number (trailing garbage)

; ─────────────────────────────────────────────────────────────
; SECTION 7 : UNDEFINED LABELS  (errors)
; ─────────────────────────────────────────────────────────────
br   nonesuch   ; error: undefined label 'nonesuch'
ldc  missing    ; error: undefined label 'missing'
brz  ghost      ; error: undefined label 'ghost'
brlz phantom    ; error: undefined label 'phantom'

; ─────────────────────────────────────────────────────────────
; SECTION 8 : EXTRA / TRAILING TOKENS ON A LINE  (errors)
; ─────────────────────────────────────────────────────────────
ldc 5, 6        ; error: extra text after operand
adc 3 4         ; error: extra text after operand
stl 1 2 3       ; error: extra text after operand
br  0 0         ; error: extra text after operand

; ─────────────────────────────────────────────────────────────
; SECTION 9 : UNUSED LABELS  (warnings)
; These labels are defined but never referenced anywhere.
; ─────────────────────────────────────────────────────────────
unusedA:        ; warning: label 'unusedA' defined but never used
unusedB: ldc 0  ; warning: label 'unusedB' defined but never used
unusedC: data 0 ; warning: label 'unusedC' defined but never used

; ─────────────────────────────────────────────────────────────
; SECTION 10 : data WITHOUT A LABEL  (warnings)
; data with no label can never be referenced -- suspicious.
; ─────────────────────────────────────────────────────────────
data 42         ; warning: data has no label and cannot be referenced
data 0xFF       ; warning: data has no label and cannot be referenced
data -1         ; warning: data has no label and cannot be referenced

; ─────────────────────────────────────────────────────────────
; SECTION 11 : SET WITHOUT A LABEL  (error)
; SET is meaningless without a label to assign the value to.
; ─────────────────────────────────────────────────────────────
SET 10          ; error: SET must have a label
SET 0x20        ; error: SET must have a label
SET -5          ; error: SET must have a label