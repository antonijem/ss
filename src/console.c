//
// Created by os on 2/1/23.
//

#include "../h/console.h"


buffer_t output_buffer, input_buffer;
sem_t output_buff_empty, output_buff_full, input_buff_empty, input_buff_full;
static thread_t cout_thread;
static thread_t cin_thread;


void __putc(char chr) {
    //Ukoliko je buffer pun blokiraj se
    if (output_buffer->size == BUFFER_SIZE) {
        sem_wait(output_buff_empty);
    }

    output_buffer->elements[output_buffer->write] = chr;
    output_buffer->write = (output_buffer->write + 1) % BUFFER_SIZE;
    if (output_buff_full->waitingQueue != 0) { //Samo ako je size bio 0 i neko ceka na semaforu ti signaliziraj
        sem_signal(output_buff_full);
    }
    output_buffer->size++;
}

void console_output() {
    for (;;) {//Radi dokle god radi i ceo sistem
        if (output_buffer->size == 0) {
            sem_wait(output_buff_full);

        }
        while (output_buffer->size != 0) {
            asm volatile("lb s6, 0(%0)" : : "r" (CONSOLE_STATUS));
            int status;
            asm volatile("mv %0, s6 " : "=r" (status));
            if (status & CONSOLE_TX_STATUS_BIT) {
                asm volatile("mv s5, %0" : :"r"(CONSOLE_TX_DATA));
                asm volatile("sd %0, 0(s5)" : : "r"(output_buffer->elements[output_buffer->read]));
                output_buffer->read = (output_buffer->read + 1) % BUFFER_SIZE;
                if (output_buff_empty->waitingQueue != 0) {
                    sem_signal(output_buff_empty);

                }
                output_buffer->size--;
            } else {
                //Ako bit za slanje u status nije 1 prebaci se na neku drugu nit
                yield(get_running(), get_scheduler_thread());
            }
        }
    }
}

void
set_output_elements(sem_t oempty, sem_t ofull, buffer_t obuff, sem_t iempty, sem_t ifull, buffer_t ibuff) {
    output_buff_empty = oempty;
    output_buff_full = ofull;
    output_buffer = obuff;
    output_buffer->size = 0;
    output_buffer->write = 0;
    output_buffer->read = 0;

    input_buff_empty = iempty;
    input_buff_full = ifull;
    input_buffer = ibuff;
    input_buffer->size = 0;
    input_buffer->write = 0;
    input_buffer->read = 0;
}

char __getc() {
    if (input_buffer->size == 0) {
        sem_wait(input_buff_full);
    }
    char c = input_buffer->elements[input_buffer->read];
    input_buffer->read = (input_buffer->read + 1) % BUFFER_SIZE;
    if (input_buff_empty->waitingQueue != 0) {
        sem_signal(input_buff_empty);
    }
    input_buffer->size--;
    return c;
}

void console_input() {
    for (;;) {

        if (input_buffer->size == BUFFER_SIZE) {
            sem_wait(input_buff_empty);
        }
        while (input_buffer->size != BUFFER_SIZE) {
            asm volatile("lb s6, 0(%0)" : : "r" (CONSOLE_STATUS));
            int status;
            asm volatile("mv %0, s6 " : "=r" (status));
            if (status & CONSOLE_RX_STATUS_BIT) {
                asm volatile("mv s5, %0" : :"r"(CONSOLE_RX_DATA));
                asm volatile("lb %0, 0(s5)" :  "=r"(input_buffer->elements[input_buffer->write]));
                input_buffer->write = (input_buffer->write + 1) % BUFFER_SIZE;
                if (input_buff_full->waitingQueue != 0) {
                    sem_signal(input_buff_full);

                }
                input_buffer->size++;
            } else {
                yield(get_running(), get_scheduler_thread());
            }
        }
    }
}


void init_console() {
    //Moze da ide u red spremnih niti jer svakako ako je prazan bafer ili ako nije dobio prekid tj status bit za slanje nije 1
    //ova nit ce se prebaciti na kontekst schedulera i zameniti sa nekom drugom

    thread_create(&cout_thread, console_output, 0);
    set_console_out_thread(cout_thread);

    sem_t out_buff_e;
    sem_open(&out_buff_e, 0);

    sem_t out_buff_f;
    sem_open(&out_buff_f, 0);

    buffer *buff = (buffer *) mem_alloc(sizeof(buffer));


    thread_create(&cin_thread, console_input, 0);
    set_console_in_thread(cin_thread);

    sem_t in_buff_e;
    sem_open(&in_buff_e, 0);

    sem_t in_buff_f;
    sem_open(&in_buff_f, 0);

    buffer *buff2 = (buffer *) mem_alloc(sizeof(buffer));

    set_output_elements(out_buff_e, out_buff_f, buff, in_buff_e, in_buff_f, buff2);

    //asm volatile("ld ra, 8(sp)");
    //asm volatile("ld s0, 0(sp)");
    //asm volatile("addi sp, sp, 16");
    //asm volatile("ret");
    return;
}

int get_output_num() {
    return output_buffer->size;
}

