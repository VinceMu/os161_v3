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
        size_t next_empty;
        //paddr vs manual calculation
};
struct frame_table_entry *frametable = 0;
paddr_t free_addr;
paddr_t first_addr;
int total_frames

void init_frametable(void)
{
        paddr_t ram_first = 0;
        paddr_t ram_size = ram_getsize();
        paddr_t temp;
        total_frames = (ram_size - ram_first)/PAGE_SIZE;
        int frametable_size = total_frames * sizeof(struct frame_table_entry);
        int entry_num = frametable_size/PAGE_SIZE;
        frametable = (struct frame_table_entry*) PADDR_TO_KVADDR(ram_first);
        free_addr = ram_first + frametable_size;
        first_addr = ram_first;

        //UNFINISHED
        for(i = 0; i < total_frames; i++){
                if(i < entry_num){
                        frametable[i].next_empty = 0;
                } else {

                }

                temp = (PAGE_SIZE * i) + first_addr;
                frametable[i].next_empty = temp;
                
        }

}

paddr_t getppages(unsigned int npages){
        paddr_t nextaddr;

        if(frametable == 0){
                spinlock_acquire(&stealmem_lock);
                nextaddr = ram_stealmem(npages);
                spinlock_release(&stealmem_lock);
        } else {
                if(npages > 1){
                        return 0;
                }

                //UNFINISHED

        }

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
        //struct issue
}

