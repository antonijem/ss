//
// Created by os on 12/27/22.
//
//Sluzi za rad sa memorijom
//-------------------------------------------------------------------------

#include "../h/mem.h"

typedef struct mem_blk{
    struct mem_blk* prev;
    struct mem_blk* next;
    size_t size;
}mem_blk;

 static mem_blk* free_mem_blks = 0;
static  mem_blk* used_mem_blks = 0;

//Inicijalizuje blok za zadatu adresu i velicinu
//-------------------------------------------------------------------------
mem_blk* init_blk(void* addr, size_t size){
    mem_blk* memBlk = (mem_blk*) addr;
    memBlk->size = size;
    memBlk->next = memBlk->prev = 0;
    return memBlk;
}

//Izbacuje zadati blok iz liste,
// prethodno po potrebi vrsi prevezivanje blokova oko bloka koji se izbacuje
//-------------------------------------------------------------------------
void kick_blk(mem_blk* blk){
    if(blk->next){
        if(blk->prev){ //Ako ima i sledećeg i prethodnog povezi ih
            blk->prev->next = blk->next;
            blk->next->prev = blk->prev;
            blk->next = 0;
            blk->prev = 0;
            return;
        }else{ //Ako ima sledeceg ali nema prethodnog onda samo postavi da sledeci nema prethodnog vise
            blk->next->prev = 0;
            blk->next = 0;
            return;
        }
    }else if(blk->prev){ //Ako nema sledeceg ali ima prethodnog
        blk->prev->next = 0;
        blk->prev = 0;
        return;
    }else{ // Ukoliko nema ni prethodni ni sledeci znaci da je to jedini element u listi i neophodno je videti da li se radi o free ili used i staviti ga na NULL
        if(free_mem_blks == blk){
            free_mem_blks = 0;
        }else if(used_mem_blks == blk){
            used_mem_blks = 0;
        }
    }
}

//Ulancava zauzetu memoriju tako što ubaci zauzeti blok u niz
//-------------------------------------------------------------------------
void ch_used_mem(mem_blk* blk){
    //Ako nema nicega u listi postavi ga da bude prvi
    if(used_mem_blks == 0){
        used_mem_blks = blk;
        return;
    }

    //Ako je na nizoj adresi od prvog u listi postavi njega da bude prvi
    if(used_mem_blks > blk){
        blk->next = used_mem_blks;
        used_mem_blks->prev = blk;
        used_mem_blks = blk;
    }else {//Pronađi prvi blok posle kog ima veću adresu i ulančaj ga tu
        mem_blk *iter = used_mem_blks;
        for (; iter->next != 0; iter = iter->next) {
            if (iter > blk) {
                blk->prev = iter->prev;
                iter->prev->next = blk;
                iter->prev = blk;
                blk->next = iter;
                return;
            }
        }
        //Proverava preptoslednji blok u listi
        if (iter > blk) {
            blk->prev = iter->prev;
            iter->prev->next = blk;
            iter->prev = blk;
            blk->next = iter;
            return;
        }

        //Ukoliko u prethodnoj petlji nije našao mesto znači da ide na kraj liste
        iter->next = blk;
        blk->prev = iter;
    }

    return;
}

//Ulancava slobodnu memoriju(i oduzima od nje)
// tako što nađe zadati blok i smanji ga za size
//-------------------------------------------------------------------------
void ch_free_mem_take(mem_blk* blk, size_t size){
    //Slucaj kada je blok za ulancavanje prvi u nizu
    if(blk == free_mem_blks){
       if(free_mem_blks->size == size){
           if(free_mem_blks->next != 0) {
               free_mem_blks->next->prev = 0;
               free_mem_blks = free_mem_blks->next;
           }else{
               free_mem_blks = 0;
           }
       }else {
           mem_blk *pom = init_blk((void *) blk + size, free_mem_blks->size - size);
           if (free_mem_blks->next != 0)
               pom->next = free_mem_blks->next;
           free_mem_blks = pom;
           return;
       }
    }

    //Kada nije prvi u nizu
    for(mem_blk* iter = free_mem_blks->next; iter; iter = iter->next){
        if(iter == blk){
            //Ako je potreban ceo slobodan blok izbaci ga iz liste
            if(iter->size == size) {
                kick_blk(iter);
                return;
            }else{
                mem_blk* pom = init_blk((void*) blk + size, iter->size - size);
                if(iter->next){
                    pom->next = iter->next;
                    iter->next->prev = pom;
                }
                if(iter->prev){
                    pom->prev = iter->prev;
                    iter->prev->next = pom;
                }
                return;
            }
        }
    }
}
//Alocira blok velicine size
//-------------------------------------------------------------------------
void* __mem_alloc(size_t size){
    if(size <= 0)
        return 0;
    size_t in_bytes = size * MEM_BLOCK_SIZE;
    mem_blk* blk = 0;

    if(free_mem_blks == 0) {
        __putc('E');
        __putc('1');
        __putc('1');
        return 0; // Nema dovoljno prostora obradi kasnije
    }

    //Pronađi slobodan blok (best-fit)
    for(mem_blk* iter = free_mem_blks; iter; iter = iter->next){
        if(iter->size >= in_bytes){
            if(blk == 0) {
                blk = iter;
                continue;
            }
            if(blk->size > iter->size)
                blk = iter;
        }
    }

    if(blk == 0){ //Nije pronašao dovoljno veliki blok
        __putc('E');
        __putc('1');
        __putc('3');
        return 0; // Nema dovoljno prostora obradi kasnije
    }
    //void* addr = (void*)blk;

    //Ulancaj (po potrebi odseci visak) slobodnu memoriju
    ch_free_mem_take(blk, in_bytes);

    blk = init_blk((void*)blk, in_bytes);

    ch_used_mem(blk);

    //blk = (mem_blk*)addr;

    return blk;

}
//Pokušava da spoji dva susedna bloka u nizu,
// blk1 je na nizoj adresi od blk2, vraca 1 u slucaju uspeha,
// 0 u slucjanu neuspeha
//-------------------------------------------------------------------------
int merge(mem_blk* blk1, mem_blk* blk2){
    //Kraj adrese blk1 se poklapa sa pocetkom adrese blk2, mogu da se spoje
    if((void*)blk1 + blk1->size == blk2){
        if(blk1->next == blk2){
            if(blk2->next){
                blk1->next = blk2->next;
                blk2->next->prev = blk1;
                blk2->next = 0;
            }else{
                blk1->next = 0;
            }
        }
        if(blk2->next){
            blk1->next = blk2->next;
            blk2->next->prev = blk1;
            blk2->next = 0;
        }
        if(blk2->prev) {
            if (blk2->prev != blk1) {
                blk1->prev = blk2->prev;
                blk2->prev->next = blk1;
            }
            blk2->prev = 0;
        }
        blk1->size = blk1->size+blk2->size;
        return 1;
    }
    return 0;
}

//Ulancava novi slobodni blok, poređani po adresama rastuce
//-------------------------------------------------------------------------
void ch_free_mem_insert(mem_blk* blk){
    //Ako nema slobodnih blokova onda je ovo prvi
    //NIJE TESTIRANO!!!
    //Ne bi trebalo nikada da dođe do ovoga jer se ukupan broj blokova zavrsava sa 0.5
    // tako da ce uvek postojati makar 0.5 slobodnih blokova
    if(free_mem_blks == 0){
        free_mem_blks = blk;
        return;
    }
    //Ako ima nizu adresu od prvog u listi
    if(free_mem_blks > blk){
        if(1 == merge(blk, free_mem_blks)){
            free_mem_blks = blk;
            return;
        }
        blk->next = free_mem_blks;
        free_mem_blks->prev = blk;
        free_mem_blks = blk;
        return;

    }
    //Pronađi u nizu mesto na koje treba da se stavi
    for(mem_blk* iter = free_mem_blks->next; iter; iter = iter->next){
        if(iter > blk){
            if(1 == merge(blk, iter)){
                merge(blk->prev, blk);
                return;
            }
            blk->next = iter;
            iter->prev->next = blk;
            blk->prev = iter->prev;
            iter->prev = blk;
            merge(blk->prev, blk);
            return;
        }
    }
}

//Oslobađa blok zadat adresom ptr
//-------------------------------------------------------------------------
int __mem_free(void* ptr){
    mem_blk* iter = used_mem_blks;
    //Ako je prvi
    if((void*)used_mem_blks == ptr){
        if(used_mem_blks->next != 0) {
            used_mem_blks->next->prev = 0;
            used_mem_blks = used_mem_blks->next;
        }
        kick_blk(iter);
    }else {//Pronađi blok u nizu zauzetih i izbaci ga iz niza
        iter = iter->next;
        for (; iter; iter = iter->next) {
            if ((void *) iter == ptr) {
                kick_blk(iter);
                break;
            }
        }
    }

    //Ako je iter NULL onda znaci da nije pronasao blok
    if(iter == 0){
        __putc('E');
        __putc('1');
        __putc('2');
        return -12;
    }

    ch_free_mem_insert(iter);
    return 0;
}

void free_mem_init(mem_blk* blk,size_t size){
    blk->next = blk->prev = 0;
    blk->size = size;
    free_mem_blks = blk;
}

void init_mem(){
    free_mem_init((mem_blk *)HEAP_START_ADDR, (size_t)(HEAP_END_ADDR-HEAP_START_ADDR));
}

void test(void* t){
    free_mem_blks->size = free_mem_blks->size + 10;
}
