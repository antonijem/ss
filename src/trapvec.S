.align 4
.globl trap
trap:
    li t4, 0x9
    csrr t3, scause
    csrr t6, sepc
    bgt t4, t3, nastavi

    #Moram da povećam pc za povratak za +1 instrukciju

    addi t6, t6, 4
nastavi:
    csrr t5, sstatus


    addi sp, sp, -128
    #Cuvam vrednost ra da bih znao gde da se vratim kada izađem iz prekidne rutine
    #Cuva se kontekst niti
    sd ra, 0(sp)
    sd s0, 8(sp)
    sd s1, 16(sp)
    sd s2, 24(sp)
    sd s3, 32(sp)
    sd s4, 40(sp)
    sd s5, 48(sp)
    sd s6, 56(sp)
    sd s7, 64(sp)
    sd s8, 72(sp)
    sd s9, 80(sp)
    sd s10, 88(sp)
    sd s11, 96(sp)
    #Moram da sacuvam i t6 na steku jer je to povratna vrednost sepc
    #koju pregazim kada pravim nit
    sd t6, 104(sp)
    sd t5, 112(sp) #Cuvam sstatus vrednost

    call syscall_handle

    #Restauriram povratnu adresu sa steka
    #Restauriram vrednost niti
    ld ra, 0(sp)
    ld s0, 8(sp)
    ld s1, 16(sp)
    ld s2, 24(sp)
    ld s3, 32(sp)
    ld s4, 40(sp)
    ld s5, 48(sp)
    ld s6, 56(sp)
    ld s7, 64(sp)
    ld s8, 72(sp)
    ld s9, 80(sp)
    ld s10, 88(sp)
    ld s11, 96(sp)
    ld t6, 104(sp)
    ld t5, 112(sp)
    addi sp, sp, 128


    csrw sstatus, t5 #Reastauriram sstatus vrednost u slucaju da imam ugnjezdene pozive i vratim se iz pogresnog rezima


    li t4, 0x9
    csrr t3, scause

    #bgt t4, t3, timer_things

    csrw sepc, t6

    sret

timer_things:

    sret