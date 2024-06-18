//
// Created by os on 1/10/23.
//


#include "../h/thread.h"

static int tid = 0;

static int num_active = 0;

static int free_thread = 0;

_thread list[MAX_NUM_THREADS];

thread_t curr = 0;

extern void new_thread();

extern void dispatch_thread();

extern int thread_exit();

void init_trapframe(thread_t t) {

    t->trapframe.ra = (size_t) new_thread;
    t->trapframe.sp = (size_t) t->sp;

    t->trapframe.a0 = 0;
    t->trapframe.a1 = 0;
    t->trapframe.a2 = 0;
    t->trapframe.a3 = 0;
    t->trapframe.a4 = 0;
    t->trapframe.a5 = 0;
    t->trapframe.a6 = 0;
    t->trapframe.a7 = 0;
    t->trapframe.s0 = 0;

    t->trapframe.s1 = 0;
    t->trapframe.s2 = 0;
    t->trapframe.s3 = 0;
    t->trapframe.s4 = 0;
    t->trapframe.s5 = 0;
    t->trapframe.s6 = 0;
    t->trapframe.s7 = 0;
    t->trapframe.s8 = 0;
    t->trapframe.s9 = 0;
    t->trapframe.s10 = 0;
    t->trapframe.s11 = 0;

    t->trapframe.t0 = 0;
    t->trapframe.t1 = 0;
    t->trapframe.t2 = 0;
    t->trapframe.t3 = 0;
    t->trapframe.t4 = (size_t) t->arg;
    t->trapframe.t5 = (size_t) thread_exit;
    t->trapframe.t6 = (size_t) t->routine;

    t->trapframe.tp = 0;
    t->trapframe.gp = 0;
}


int thread_create_sys(thread_t *handle, void(*start_routine)(void *), void *arg, void *stack_space) {
    //Nema slobodnih niti, vrati gresku
    if (num_active == MAX_NUM_THREADS) {
        return -1;
    }
    //Sigurno ima slobodnih, pronađi jednu
    if (list[tid].th_state != FREE) {
        for (int i = 0; i < MAX_NUM_THREADS; i++) {
            if (list[i].th_state == FREE) {
                free_thread = i;
                break;
            }
        }
    } else {
        free_thread = tid;
    }

    list[free_thread].routine = start_routine;
    //stack_space += DEFAULT_STACK_SIZE;//Stack raste na dole, i onda moram da ga postavim na najvisu adresu

    //Poravnaj stek na 16
    list[free_thread].sp = (size_t) stack_space + DEFAULT_STACK_SIZE;
    list[free_thread].arg = arg;

    init_trapframe(&list[tid]);

    list[free_thread].sp = (size_t) stack_space;
    list[free_thread].th_state = NEW;
    list[free_thread].time_slice = DEFAULT_TIME_SLICE;
    list[free_thread].tid = tid;
    list[free_thread].next = 0;
    *handle = &list[free_thread];
    tid = (tid + 1) % MAX_NUM_THREADS;
    num_active++;

    return 0;
}

void init_list_elem(_thread addr, int i) {
    list[i] = addr;
    list[i].th_state = FREE;
}

void dec_num_of_active_threads() {
    num_active--;
    if (num_active < 0) {
        num_active = 0;
    }
}

int get_num_active_threads() {
    return num_active;
}

