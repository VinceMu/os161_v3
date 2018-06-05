#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
struct frame_table_entry {
//        struct addrspace* a_space;
        paddr_t physical_addr;
        //vaddr_t virtual_addr;
        // size_t next_empty;
        bool is_free; 
        //paddr vs manual calculation
};
struct frame_table_entry *frametable = NULL; //change to 0?
paddr_t curr_free_addr;
//paddr_t first_addr;
int total_frames;
void init_frametable(void);
paddr_t getppages(unsigned int npages);

void init_frametable(void)
{
        // might have to use lock!!!
        paddr_t ram_first = 0;
        paddr_t ram_size = ram_getsize();
       // paddr_t temp;
        total_frames = (ram_size - ram_first)/PAGE_SIZE;
        int frametable_size = total_frames * sizeof(struct frame_table_entry); 
       // int entry_num = frametable_size/PAGE_SIZE;
        frametable = (struct frame_table_entry*) PADDR_TO_KVADDR(ram_first);
        curr_free_addr = ram_first + frametable_size;
        curr_free_addr = curr_free_addr + (PAGE_SIZE - (curr_free_addr % PAGE_SIZE));
        //first_addr = ram_first;

        //UNFINISHED
        for(int i = 0; i < total_frames; i++){
                frametable[i].is_free = true;
                frametable[i].physical_addr = curr_free_addr + i * PAGE_SIZE;
                //frametable[i].virtual_addr = -1; //CHECK THIS


                // if(i < entry_num){
                //         frametable[i].next_empty = 0;
                // } else {

                // }

                // temp = (PAGE_SIZE * i) + first_addr;
                // frametable[i].next_empty = temp;
                
        }

}

paddr_t getppages(unsigned int npages){
        paddr_t nextaddr;

        if(frametable == NULL){
                spinlock_acquire(&stealmem_lock);
                nextaddr = ram_stealmem(npages);
                spinlock_release(&stealmem_lock);
        } else {
                if(npages > 1){
                        return 0;
                }
                // maybe acquire a locker here !
         //       int free_index = -1;
                int i = 0;
                for(i=0;i<total_frames;i++){
//                        free_index = i;
                        if(frametable[i].is_free == false){
                                break;
                        }
                }
                frametable[i].is_free = false;
                nextaddr = frametable[i].physical_addr;
                // CHECK IF i > total_frames !?!?!
        }
        bzero((void *)PADDR_TO_KVADDR(nextaddr),PAGE_SIZE);
        return nextaddr;
}

/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t alloc_kpages(unsigned int npages)
{
        paddr_t addr = getppages(npages);
        if(addr == 0){
                return 0;
        }
        return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr)
{
        bool is_done = false;

        for(int i = 0; i < total_frames; i++){
                if(is_done == true){
                        return;
                }
                if(PADDR_TO_KVADDR(frametable[i].physical_addr)== addr){
                        frametable[i].is_free = true;
                        is_done = true;
                }
        }
        //struct issue
}

