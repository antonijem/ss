//
// Created by os on 1/21/23.
//

#include "../h/semaphore.h"


static _sem list_sem[MAX_NUM_SEM];
static int num_of_active = 0;
static int id = 0;

void add_to_queue_sem(sem_t queue, thread_t handle) {
    if (queue->waitingQueue == 0) {//Ako je red prazan stavi ga kao prvi
        queue->waitingQueue = handle;
    } else {
        //Dodaj nit koja se trenutno izvrsavala na kraj reda
        for (thread_t iter = queue->waitingQueue;; iter = iter->next) {
            if (iter->next == 0) {
                iter->next = handle;
                break;
            }
        }
    }
}

int sem_open_sys(sem_t *handle, unsigned int init) {
    if (num_of_active == MAX_NUM_SEM) {
        return -1;
    }
    if (list_sem[id].sem_status == ALIVE) {
        for (int i = 0; i < MAX_NUM_SEM; i++) {
            if (list_sem[i].sem_status != ALIVE) {
                id = i;
                break;
            }
        }
    }

    list_sem[id].waitingQueue = 0;
    list_sem[id].val = init;
    list_sem[id].sem_status = ALIVE;

    *handle = &list_sem[id];
    id++;
    num_of_active++;
    return 0;
}

int sem_signal_sys(sem_t handle) {
    int ret = 0;
    //Provera da li semafor nije ugasen
    if (handle->sem_status != ALIVE) {
        ret = -2;
        return ret;
    }
    //Ako ima nit koja ceka u redu, onda je samo stavi kao spremnu bez povecavanja vrednosti semafora
    if (handle->waitingQueue != 0) {
        thread_t ready = handle->waitingQueue;
        handle->waitingQueue = ready->next;
        ready->next = 0;
        ready->th_state = READY;
        //remove_waiting();
        add_ready(ready);
    } else {//Ako nema blokiranih niti onda postavi vrendost semafora na 1
        handle->val++;
    }

    return 0;
}

int sem_wait_sys(sem_t handle) {
    int ret = 0;

    //Provera da li semafor nije ugasen
    if (handle->sem_status != ALIVE) {
        ret = -2;
        return ret;
    }
    //Ukoliko je vrednost semafora 1 samo je umanji
    if (handle->val > 0) {
        handle->val--;
    } else { // Ako neko vec ceka na semaforu blokiraj se
        //Dohvati trenuntu nit i postavi joj stanje da ceka
        thread_t waiting = get_running();
        waiting->th_state = WAITING;
        add_to_queue_sem(handle, waiting);
        yield(waiting, get_scheduler_thread());
        //Provera da li je semafor zatvoren pre nego sto su sve niti prosle
        if (waiting->th_state == WAITING)
            ret = -1;
    }
    return ret;
}

void init_sem_list(_sem addr, int i) {
    list_sem[i] = addr;
    list_sem[i].sem_status = DEAD;
    list_sem[i].val = 0;
    list_sem[i].waitingQueue = 0;
}
