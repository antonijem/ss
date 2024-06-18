.align 4
.globl new_thread
new_thread:
        #Ako je t4 == 0 znaci da nema argumente i treba da preskoci njihov unos
        bnez t4, args
continue:
        addi sp, sp, -16
        #Cuvam vrednost ra da bih znao gde da se vratim kada izađem iz prekidne rutine
        sd t5, 0(sp)
        addi ra, ra, 20

        jr t6

        #Restauriram povratnu adresu sa steka
        ld ra, 0(sp)
        addi sp, sp, 16

        ret

args:   #Ucitavam arugmente

        mv a0, t4
        ld a1, 8(t4)
        ld a2, 16(t4)
        ld a3, 24(t4)
        ld a4, 32(t4)
        ld a5, 40(t4)
        ld a6, 48(t4)
        ld a7, 56(t4)
        beq a0, a0, continue


.align 4
.globl timer_thread_start
timer_thread_start:

        jr t6

        ret
