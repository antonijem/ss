//
// Created by os on 1/5/23.
//

#include "../h/trap.h"

static int cnt = 0;

extern void trap();

void init_trap() {
    size_t x = (size_t) trap;
    asm volatile("csrw stvec, %0" : : "r" (x));
}

//Vrši razgranat skok u zavisnosti od toga ko je pozvao ecall
//-------------------------------------------------------------------------
void syscall_handle(size_t num, ...) {
    asm volatile("addi sp, sp, -16");
    asm volatile("csrr t0, sstatus");
    asm volatile("sd t0, 0(sp)");

    //size_t num;
    //asm volatile("mv %0, a0" : "=r" (num));
    //Izvlacim argumente iz A registra u S jer prilikom switch-a koristi neke A registre i brise mi argumente f-ja
    //Za sada 5 argumenata jer je to max pa cu za svaki slucaj povuci sve

    asm volatile("mv t1, a1");
    asm volatile("mv t2, a2");
    asm volatile("mv t3, a3");
    asm volatile("mv t4, a4");
    asm volatile("mv t5, a5");

    size_t scause;
    asm volatile("csrr %0, scause" : "=r" (scause));


    //asm volatile("mv s5, a5");
    //asm volatile("mv s6, a6");
    if (scause == 0x9 || scause == 0x8) {
        switch (num) {
            case SYS_MEM_ALLOC: {
                size_t size = 0;
                asm volatile("mv %0, t1" : "=r" (size));
                __mem_alloc(size);
                break;
            }
            case SYS_MEM_FREE: {
                void *addr;
                asm volatile("mv %0, t1" : "=r" (addr));
                __mem_free(addr);
                break;
            }
            case SYS_THREAD_CREATE: {
                thread_t *handle;
                asm volatile("mv %0, t1" : "=r" (handle));
                void *start_routine;
                asm volatile("mv %0, t2" : "=r" (start_routine));
                void *arg;
                asm volatile("mv %0, t3" : "=r" (arg));
                void *stack_space;
                asm volatile("mv %0, t4" : "=r" (stack_space));
                //void *th;
                //asm volatile("mv %0, s5" : "=r" (th));
                //void *tf;
                //asm volatile("mv %0, t5" : "=r" (tf));

                thread_create_sys(handle, start_routine, arg, stack_space);
                break;
            }
            case SYS_THREAD_EXIT: {
                thread_t handle = get_running();
                handle->th_state = FREE;
                dec_num_of_active_threads();
                //Oslobađam memoriju koju je nit zauzimala
                __mem_free((void *) (handle->sp - MEM_OFFSET));
                //__mem_free((void *) (((size_t) handle->trapframe - MEM_OFFSET)));
                //__mem_free((void *) ((size_t) handle - MEM_OFFSET));

                remove_running();//Trenutna nit koja se izvrsala se sada gasi tako da nema niti koja radi
                //sem scheduler niti ali ona ima svoj handle i nju ne stavljam nikada na running

                //Prebacujem se na stek scheduler niti jer ce na dalje ona raditi
                //asm volatile("mv sp, %0" : : "r" (get_scheduler_thread()->trapframe->sp));
                //Upisujem u ra registar povratnu adresu iz schedulera da bih nasatavio tamo gde je on stao
                //to ce biti u scheduler() f-ji
                //asm volatile("mv ra, %0" : :"r" (get_scheduler_thread()->trapframe->ra));
                //asm volatile("addi sp, sp, -96");
                //asm volatile("sd ra, 24(sp)");
                //asm volatile("sd s0, 16(sp)");
                //asm volatile("sd s1, 8(sp)");
                //return;
                break;
            }
            case SYS_DISPATCH: {
                dispatch_thread();
                break;
            }
            case SYS_SEM_OPEN: {
                sem_t *handle;
                asm volatile("mv %0, t1" : "=r" (handle));
                unsigned int init;
                asm volatile("mv %0, t2" : "=r" (init));
                sem_open_sys(handle, init);

                break;
            }
            case SYS_SEM_SIGNAL: {
                sem_t id;
                asm volatile("mv %0, t1" : "=r" (id));
                sem_signal_sys(id);

                break;
            }
            case SYS_SEM_WAIT: {
                sem_t id;
                asm volatile("mv %0, t1" : "=r" (id));
                sem_wait_sys(id);
                break;
            }
            case SYS_SEM_CLOSE: {
                sem_t handle;
                asm volatile("mv %0, t1" : "=r" (handle));

                //Dokle god ima niti koje cekaju na ovom semaforu stavi ih u readyQueue i izbaci iz waitingQueue
                //nece im state biti postavljen na ready sto ce znaciti da su "ilegalno" presli u readyQueue
                while (handle->waitingQueue) {
                    remove_waiting();
                    add_ready(handle->waitingQueue);
                    handle->waitingQueue = handle->waitingQueue->next;
                }

                handle->sem_status = DEAD;

                //Dealociraj memoriju koju si zauzeo semaforom
                //__mem_free((void *) ((size_t) handle - MEM_OFFSET));
                break;

            }
            case SYS_THREAD_SLEEP: {
                time_t time;
                asm volatile("mv %0, t1" : "=r" (time));
                put_running_to_sleep(time);
                break;
            }
            case SYS_PUT_C: {
                char c;
                asm volatile("mv %0, t1" : "=r" (c));
                __putc(c);
                break;
            }
            case SYS_GET_C: {
                __getc();
                break;
            }
            default: {
                __putc('E');
                __putc('3');
                __putc('1');
                return;
            }
        }
    } else if (scause == 0x8000000000000001L) { //Softverski prekid od tajmera

        //Potvrdi da je prekid obrađen promenom bita u sip registru
        acknowledgeSoftwareTrap

        cnt++;
        //Generise 10 prekida u 1 sekundi, kada cnt == 10 znaci da je jedan period otkucan umanji vreme koriscenja trenutnoj niti
        //ako je isteklo vreme korišćenja trenuntne niti uradi dispatch, ali pre toga prebaci se na nit tajmera da bi odradio sledeće
        //umanji vreme u sleepQueue i potencijalno ih prebaci u readyQueue
        if (cnt == 10) {
            cnt = 0;
            thread_t running = get_running();
            //Proveri da li ima niti koje spavaju, ako da treba da umanjis vreme za koje su spavale
            //ako ne samo nastavi dalje
            if (is_sleepingQueue_emtpy()) {
                thread_t timer = get_timer_thread();
                yield(running, timer);
                timer->trapframe.ra = (size_t) decrement_sleeping_timer;//Postavljam povratnu vrednost timer niti na ovu f-ju kako bi on samo nju pozivao
            }
            running->time_slice--;
            if (running->time_slice == 0) {//Ukoliko je isteklo vreme koriscenja zameni se sa nekom drugom niti
                running->time_slice = DEFAULT_TIME_SLICE;
                yield(running, get_scheduler_thread());
            }

        }
    } else if (scause == 0x8000000000000009L) {

        acknowledgeHardwareTrap
        int interrupt = plic_claim();

        if (interrupt == CONSOLE_IRQ) {//Ukoliko je prekid od console obradi ga ako nije samo ignorisi
            //Ucitaj vrednost CONSOLE_STATUS
            asm volatile("lb s6, 0(%0)" : : "r" (CONSOLE_STATUS));
            int status;
            asm volatile("mv %0, s6 " : "=r" (status));
            //Za svaki slucaj cu ponovo ucitati, mozda se nesto promenilo u toku slanja
            asm volatile("lb s6, 0(%0)" : : "r" (CONSOLE_STATUS));
            asm volatile("mv %0, s6 " : "=r" (status));
            if (status & CONSOLE_RX_STATUS_BIT) {
                if (get_console_thread_in()->th_state != WAITING) {
                    thread_t runn = get_running();
                    swap_running_thread(get_console_thread_in());
                    yield(runn, get_console_thread_in());//Prebaci se na kontekst console niti
                }
            }
            if (status & CONSOLE_TX_STATUS_BIT) { //Da li je spreman za slanje
                //Proveravam da li nit vec ne ceka da joj se upise nesto u baffer
                if (get_console_thread_out()->th_state != WAITING) {
                    thread_t runn = get_running();
                    swap_running_thread(get_console_thread_out());
                    yield(runn, get_console_thread_out());//Prebaci se na kontekst console niti
                }
            }

            plic_complete(interrupt);

        }

    } else {
        __putc('E');
        __putc('3');
        __putc('2');
        if (scause == 5)
            __putc('5');
        else if (scause == 2)
            __putc('2');
        else if (scause == 7)
            __putc('7');

        return;
    }

    asm volatile("csrw scause, %0" : : "r"(scause));
    asm volatile("ld t0, 0(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("csrw sstatus, t0");

}