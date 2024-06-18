//
// Created by os on 1/12/23.
//

#include "../h/scheduler.h"


thread_t runningThread;
thread_t readyQueue, waitingQueue, sleepingQueue;//FIFO
thread_t schedulerThread;
thread_t timerThread;
thread_t idle_thread;
thread_t consoleThreadOut, consoleThreadIn;
static time_t time_sleep = 0;

void extern swtch(void *trapframeOld, void *trapframeNew);

void extern timer_thread_start();

void remove_waiting() {
    waitingQueue = waitingQueue->next;
}

void add_to_queue(enum state st, thread_t handle) {

    switch (st) {
        case WAITING: {
            if (waitingQueue == 0) {//Ako je red prazan stavi ga kao prvi
                waitingQueue = handle;
            } else {
                //Dodaj nit koja se trenutno izvrsavala na kraj reda
                for (thread_t iter = waitingQueue;; iter = iter->next) {
                    if (iter->next == 0) {
                        iter->next = handle;
                        break;
                    }
                }
            }
            break;
        }
        default: {
            if (readyQueue == 0) {//Ako je red prazan stavi ga kao prvi
                readyQueue = handle;
            } else {
                //Dodaj nit koja se trenutno izvrsavala na kraj reda
                for (thread_t iter = readyQueue;; iter = iter->next) {
                    if (iter->next == 0) {
                        iter->next = handle;
                        return;
                    }
                }
            }
        }
    }
}

void schedule() {


    for (;;) {
        //disableAllTraps
        thread_t next;
        if (readyQueue == 0) {//Nema spremnih niti, postavi idle nit kao trenutnu nit koja radi
            next = idle_thread;
        } else {
            //Izbaci spremnu nit iz reda za cekanje
            next = readyQueue;
            readyQueue = next->next;
            next->next = 0;//On se vise ne nalazi u listi spremnih tako da ne mora da pokazuje na slecedi
        }
        if (runningThread == idle_thread || runningThread == consoleThreadIn || runningThread == consoleThreadOut) {
            runningThread = 0;
        } else if (runningThread == 0) {
            //Ne radi nista
        } else {
            switch (runningThread->th_state) { //Gledam u kom je stanju nit, po defaultu ide u stanje ready
                case WAITING: {
                    //add_to_queue(WAITING, runningThread);
                    break;
                }
                case SLEEPING: {
                    break;
                }
                default: {
                    runningThread->th_state = READY;
                    add_to_queue(READY, runningThread);
                }
            }
        }

        runningThread = next;
        //Zameni nit schedulera sa sledecom niti koja moze da se izvrsava
        yield(schedulerThread, next);
        //enableAllTraps
    }
}

void yield(thread_t old, thread_t newT) {
    //Sacuvaj vrednosti a0 i a1 registara odmah jer ce swtch koristiti njihove vrednosti kao pokazivac gde treba da cuva
    //kontekst stare i nove niti
    asm volatile("sd a0, 0(%0)" : : "r" (&old->trapframe));
    asm volatile("sd a1, 8(%0)" : : "r" (&old->trapframe));
    swtch(&old->trapframe, &newT->trapframe);

    //Odavde će jedino schedulerThread i kasnije timerThread nastavljati
    //jer će jedino one pozivati ovu f-ju
}

//Dispatch radi isto sto i scheduler samo sto on ako ne nađe spremnnu nit ne radi nista
//dok scheduler ceka da se spremna nit pojavi
void dispatch_thread() {

    thread_t pom = runningThread;
    runningThread = 0;
    add_to_queue(READY, pom);
    yield(pom, schedulerThread);

    return;
}

//set_running ce zvati samo init_kthread, zbog toga sto će se jedino tada eksterno stavljati nit koja radi
//inace to radi scheduler
void set_running(thread_t handle) {
    readyQueue = 0;
    runningThread = handle;
}

void add_ready(thread_t handle) {
    //Ako je readyQueue prazan dodaj na pocetak
    if (readyQueue == 0) {
        readyQueue = handle;
        return;
    }
    //Nađi poslednjeg u listi i dodaj na kraj
    for (thread_t iter = readyQueue;; iter = iter->next) {
        if (iter->next == 0) {
            iter->next = handle;
            return;
        }
    }

}

void set_scheduler() {
    schedulerThread = readyQueue;
    readyQueue = 0;
}

thread_t get_running() {
    return runningThread;
}

thread_t get_scheduler_thread() {
    return schedulerThread;
}

void swap_kthread_sthread() {
    thread_t next = readyQueue;
    readyQueue = runningThread;

    runningThread = 0;//Da ne bi stavio kernelsku nit ponov u red za cekanje jer scheduler-ska nit ima svoje mesto a trenutna nit koja radi "ne" postoji

    schedulerThread = next;
    yield(readyQueue, next);
}

void remove_running() {
    runningThread = 0;
}

void remove_scheduler() {
    schedulerThread = 0;
}

thread_t get_timer_thread() {
    return timerThread;
}

int is_sleepingQueue_emtpy() {
    if (sleepingQueue == 0) {
        return 0;
    }
    return 1;
}

int put_running_to_sleep(time_t time) {
    if (time != 0) {
        runningThread->time_slice = time; //Time_slice sada predstavlja koliko dugo ce nit biti uspavana
        runningThread->th_state = SLEEPING;
        if (sleepingQueue == 0) { //Provera da li je prazan sleepingQueue
            sleepingQueue = runningThread;
            time_sleep = time;

        } else if (sleepingQueue->time_slice >
                   time) {//Da li prvi element sleepingQueue treba da ceka duze od tekuce niti
            runningThread->next = sleepingQueue;
            sleepingQueue->time_slice = sleepingQueue->time_slice - time;
            sleepingQueue = runningThread;
            time_sleep = time;

        } else {
            runningThread->time_slice = time - time_sleep;
            if (sleepingQueue->next == 0) {//Da li u sleepingQueue ima samo jedna nit
                sleepingQueue->next = runningThread;
                runningThread->time_slice = time - time_sleep;

            } else { //Ako ima vise niti u sleepingQueue
                thread_t curr = sleepingQueue->next;
                thread_t prev = sleepingQueue;
                while (curr) { //Pronađi njegovo mesto u listi
                    if (curr->time_slice > runningThread->time_slice) {
                        prev->next = runningThread;
                        runningThread->next = curr;
                        break;
                    } else if (curr->time_slice == runningThread->time_slice) { //Imaju isto vreme spavanja
                        runningThread->time_slice = 0;
                        runningThread->next = curr->next;
                        curr->next = runningThread;
                        break;
                    } else {//Treba da bude smesten posle ove niti
                        prev = curr;
                        curr = curr->next;
                        if (curr && curr->time_slice == 0) {//Da li niti posle ove imaju isto vreme cekanja
                            while (curr && curr->time_slice == 0) {//Preskoci ih
                                prev = curr;
                                curr = curr->next;
                            }
                        }
                    }
                }
                if (curr == 0) {//Ako si dosao ovde znaci da treba nit da ide na kraj liste
                    prev->next = runningThread;
                }
            }
        }
    } else {
        runningThread->time_slice = DEFAULT_TIME_SLICE;
    }
    yield(runningThread, schedulerThread);
    return 0;
}

void decrement_sleeping_timer() {
    time_sleep = time_sleep - 1;
    sleepingQueue->time_slice = sleepingQueue->time_slice - 1;
    if (time_sleep == 0) { //Ukoliko je proslo vreme za koje je nit trebala da spava prebaci je u red spremnih
        while (sleepingQueue !=
               0) { //Morao da odvojim while i if jer ako je sleepingQueue NULL onda ako pokusam da citam time_slice citaću sa adrese 0
            if (sleepingQueue->time_slice == 0) {//i bacice interrupt 5 (citanje sa nedozvoljene adrese)
                thread_t ready = sleepingQueue;
                sleepingQueue = sleepingQueue->next;
                ready->time_slice = DEFAULT_TIME_SLICE;
                ready->th_state = READY;
                ready->next = 0;
                add_ready(ready);
            } else {
                break;
            }
        }
        if (sleepingQueue != 0) { //Postavi novo vreme spavanja na prvu sledecu nit u redu, ako red nije prazan
            time_sleep = sleepingQueue->time_slice;
        }
    }
    timerThread->trapframe.ra = (size_t) decrement_sleeping_timer;
    yield(timerThread, runningThread);
}

void set_timer_thread() {
    timerThread = readyQueue;
    readyQueue = 0;
    timerThread->trapframe.ra = (size_t) timer_thread_start;
}

void infinite_loop() {//Sluzi za idle thread
    for (;;) {
        enableAllTraps
    }
}

void set_idle_thread() {
    idle_thread = readyQueue;
    readyQueue = 0;
}

thread_t get_console_thread_out() {
    return consoleThreadOut;
}

void set_console_out_thread(thread_t th) {
    consoleThreadOut = th;
    readyQueue = 0;
}

void swap_running_thread(thread_t th) {
    add_ready(get_running());
    runningThread = th;
    //Ako se nalazi u readyQueue izbaci ga
    if (th == readyQueue) {
        readyQueue = th->next;
        th->next = 0;
    } else {
        thread_t curr = readyQueue->next;
        thread_t prev = readyQueue;
        while (curr) {
            if (curr == th) {
                prev->next = th->next;
                th->next = 0;
            }
            curr = curr->next;
            prev = prev->next;
        }
    }
}

thread_t get_console_thread_in() {
    return consoleThreadIn;
}

void set_console_in_thread(thread_t th) {
    consoleThreadIn = th;
    readyQueue = 0;
}


