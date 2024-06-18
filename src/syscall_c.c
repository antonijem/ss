//
// Created by os on 1/5/23.
//
#include "../h/syscall_c.h"

static thread_t kernel_thread;
static thread_t scheduler_thread;
static thread_t timer_thread;
static thread_t idle_thread;

extern void main();

extern void schedule();

extern void console_output();

extern void console_input();

size_t syscall(size_t num, ...) {
    size_t ret;

    asm volatile("ecall");

    asm volatile("mv %0, a0":"=r" (ret));
    return ret;
}

void *mem_alloc(size_t size) {
    size += MEM_INFO; //Moram da povećam za MEM_INFO jer je to velicina koju zauzimaju podaci potrebni za memorijeske blokove
    //Posto je size u bajtovima moram da odredim koliko cu celih blokova zauzeti
    size_t num_blks = size / MEM_BLOCK_SIZE;
    //Provera ako nije ceo broj da se zauzme još jedan blok vise kako bi sve stalo
    if (size % MEM_BLOCK_SIZE != 0) {
        num_blks++;
    }
    syscall(SYS_MEM_ALLOC, num_blks);
    void *ret;
    asm volatile("mv %0, a0" : "=r" (ret));
    return ret +
           MEM_OFFSET;//Dodajem MEM_OFFSET da bi pokazivalo na memoriju posle pokazivaca i info o memorijskom bloku
}

int mem_free(void *addr) {
    syscall(SYS_MEM_FREE, addr - MEM_OFFSET);
    size_t ret;
    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

int thread_create(thread_t *handle, void (*start_routine)(void *), void *arg) {
    //Cuvam handle, start_routine i arg na stek jer prilikom dole poziva mem_alloc one ce biti pregazene
    int ret;
    asm volatile("addi sp, sp, -40");

    asm volatile("sd a1, 8(sp)");
    asm volatile("sd a2, 16(sp)");

    //*handle = (thread_t ) mem_alloc(sizeof(thread_t));
    asm volatile("sd a0, 0(sp)");

    //Kako izracunam vrednost odmah cuvam na stek da bih mogao na kraju da ih raspodelim u odgovarajuci A registar
    //jer ce prilikom switch-a da koristi neke A registre i obrise mi argumente
    void *stack_space = mem_alloc(DEFAULT_STACK_SIZE);
    if (stack_space != 0) {
        asm volatile("mv a0, %0" : : "r" (stack_space));
        asm volatile("sd a0, 24(sp)");
    } else {
        ret = -1;//Nema memoriju nece uspeti da napravi nit
        return ret;
    }

    //Prenos preko steka
    asm volatile("ld a1, 0(sp)");
    asm volatile("ld a2, 8(sp)");
    asm volatile("ld a3, 16(sp)");
    asm volatile("ld a4, 24(sp)");
    asm volatile("ld a5, 32(sp)");
    //asm volatile("ld a6, 40(sp)");
    asm volatile("addi sp, sp, 40");

    syscall(SYS_THREAD_CREATE);

    asm volatile("mv %0, a0" : "=r" (ret));

    add_ready(*handle);

    return ret;
}

void thread_dispatch() {
    syscall(SYS_DISPATCH);
    if (get_running() != kernel_thread) {
        asm volatile("csrr t0, sstatus");
        asm volatile("addi t0, t0, 2");
        asm volatile("csrw sstatus, t0");
    }
    asm volatile("ld ra, 8(sp)");
    asm volatile("ld s0, 0(sp)");
}

void init_kthread() {
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        thread_t pom = mem_alloc(sizeof(_thread));
        init_list_elem(*pom, i);
    }
    for (int i = 0; i < MAX_NUM_SEM; i++) {
        sem_t pom = mem_alloc(sizeof(_sem));
        init_sem_list(*pom, i);
    }

    thread_create(&kernel_thread, main, 0);
    set_running(kernel_thread);
    //asm volatile("mv t0, sp");
    //asm volatile("mv %0, t0" : "=r"(kernel_thread->sp));
    //asm volatile("mv %0, t0" : "=r"(kernel_thread->trapframe.sp));
    asm volatile("ld ra, 328(sp)");
    asm volatile("ld s0, 320(sp)");
    //asm volatile("ld s1, 312(sp)");
    asm volatile("addi sp, sp, 336");
    asm volatile("ld %0, 8(sp)" : "=r" (kernel_thread->sp));
    asm volatile("mv sp, %0" : : "r" (kernel_thread->trapframe.sp));//Odavde koristim stack kernelske niti
    //Ovde cuvam vrednosti za zavrsetak main f-je i to cu cuvati odmah na pocetku stack-a kernelske niti
    //asm volatile("addi sp,sp,-16");
    //asm volatile("sd t6, 8(sp)");
    asm volatile("ret");
    return;
}

void init_scheduler() {
    thread_create(&scheduler_thread, (void (*)(void *)) schedule, 0);
    //Scheduler nit ce samo jednom biti u readyQueue posle toga ide u beskonacnu petlju, gde radi svoj posao
    set_scheduler();
    asm volatile("ld ra, 8(sp)");
    asm volatile("ld s0, 0(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("ret");
    return;
}

void init_timer_thread() {
    thread_create(&timer_thread, decrement_sleeping_timer, 0);
    set_timer_thread();
    asm volatile("ld ra, 8(sp)");
    asm volatile("ld s0, 0(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("ret");
    return;
}

int thread_exit() {
    //Ideja, pozovi get running, dealociraj joj memoriju ( u celosti) postavi u ra registar vrednost sched f-je koja ce samo prebacivati na
    //kontekst scheduler niti i onda zavrsi f-ju, tako ce vratiti vrednost 0 i skociti na sched
    //implementiraj sched!!!
    int ret;

    if (get_running() != kernel_thread) {
        asm volatile("mv sp, %0" : : "r" (get_scheduler_thread()->trapframe.sp));
        syscall(SYS_THREAD_EXIT);
    } else {
        ret = -1;
    }

    //thread_t handle = get_running();
    //Oslobađam memoriju koju je nit zauzimala
    //mem_free((void *)(handle->sp-DEFAULT_STACK_SIZE));
    //mem_free(handle->trapframe);
    //mem_free(handle);
    //
    //remove_running();//Trenutna nit koja se izvrsala se sada gasi tako da nema niti koja radi
    //sem scheduler niti ali ona ima svoj handle i nju ne stavljam nikada na running
    //
    //Prebacujem se na stek scheduler niti jer ce na dalje ona raditi
    //asm volatile("mv sp, %0" : : "r" (get_scheduler_thread()->trapframe->sp));
    //Upisujem u ra registar povratnu adresu iz schedulera da bih nasatavio tamo gde je on stao
    //to ce biti u scheduler() f-ji
    //asm volatile("mv ra, %0" : :"r" (get_scheduler_thread()->trapframe->ra));
    //asm volatile("addi sp, sp, -32");
    //asm volatile("sd ra, 24(sp)");
    //asm volatile("sd s0, 16(sp)");
    //asm volatile("sd s1, 8(sp)");

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;

}

void kill_scheduler() {
    thread_t handle = get_scheduler_thread();
    mem_free((void *) (handle->sp - DEFAULT_STACK_SIZE));
    mem_free(handle);

    remove_scheduler();
    return;
}

void kill_kthread() {
    thread_t handle = get_running();
    mem_free((void *) (handle->sp - DEFAULT_STACK_SIZE));
    mem_free(handle);

    remove_running();

    asm volatile("ld ra, 24(sp)");
    asm volatile("ld s0, 16(sp)");
    asm volatile("addi sp,sp,32");
    asm volatile("ld t6, 8(sp)");//SP glavnog programa
    asm volatile("mv sp, t6");
    asm volatile("ret");
}

int sem_open(sem_t *handle, unsigned int init) {
    int ret;
    syscall(SYS_SEM_OPEN, handle, init);

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

int sem_close(sem_t handle) {
    int ret;
    syscall(SYS_SEM_CLOSE, handle);

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

int sem_signal(sem_t id) {
    int ret;
    syscall(SYS_SEM_SIGNAL, id);

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

int sem_wait(sem_t id) {
    int ret;
    syscall(SYS_SEM_WAIT, id);

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

int time_sleep(time_t time) {
    int ret;
    syscall(SYS_THREAD_SLEEP, time);

    asm volatile("mv %0, a0" : "=r" (ret));
    return ret;
}

void init_idle_thread() {
    thread_create(&idle_thread, infinite_loop, 0);
    set_idle_thread();
    asm volatile("ld ra, 8(sp)");
    asm volatile("ld s0, 0(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("ret");
    return;
}

void putc(char c) {

    syscall(SYS_PUT_C, c);

    return;
}


char getc() {
    syscall(SYS_GET_C);
    char c;
    asm volatile("mv %0, a0" : "=r" (c));
    return c;
}


void shutdown() {
    while (get_num_active_threads() > 6 && get_output_num() != 0) {
        yield(kernel_thread, get_scheduler_thread());
    }
    asm volatile("mv ra, %0"::"r"(kernel_thread->sp));
    asm volatile("ret");
}

void userMode() {
    size_t scounteren;
    asm volatile("csrr %0, scounteren" : "=r" (scounteren));
    scounteren = scounteren + 7;
    asm volatile("csrw scounteren, %0" : : "r"(scounteren));


    size_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r" (sstatus));
    sstatus = sstatus + 2;
    asm volatile("csrw sstatus, %0" : : "r" (sstatus));
}

