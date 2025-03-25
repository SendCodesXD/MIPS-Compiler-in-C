.macro do_syscall(%n)
    li $v0, %n
    syscall
.end_macro
.macro exit
    do_syscall(10)
.end_macro
.macro print_str(%label)
    la $a0, %label
    do_syscall(4)
.end_macro
.macro read_str(%label, %len)
    la $a0, %label
    li $a1, %len
    do_syscall(8)
.end_macro
.macro allocate_str(%label, %str)
    %label: .asciiz %str
.end_macro
.macro allocate_bytes(%label, %n)
    %label: .space %n
.end_macro
.macro allocate_word(%label, %n)
    %label: .word %n
.end_macro