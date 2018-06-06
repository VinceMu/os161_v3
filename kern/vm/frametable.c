#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <synch.h>
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
struct lock* frametable_lock;

//paddr_t first_addr;
int total_frames;
void init_frametable(void);
paddr_t getppages(unsigned int npages);

void init_frametable(void)
{
        frametable_lock = lock_create("frametable lock");
        // might have to use lock!!!
	spinlock_acquire(&stealmem_lock);
//        paddr_t ram_first = ram_getfirstfree();
        
        paddr_t ram_size = ram_getsize();
        paddr_t ram_first = ram_getfirstfree();

       // paddr_t temp;
        total_frames = (ram_size)/PAGE_SIZE;
        int frametable_size = total_frames * sizeof(struct frame_table_entry); 
       // int entry_num = frametable_size/PAGE_SIZE;
        frametable = (struct frame_table_entry*) PADDR_TO_KVADDR(ram_first);
        curr_free_addr = ram_first + frametable_size;
        curr_free_addr = curr_free_addr + (PAGE_SIZE - (curr_free_addr % PAGE_SIZE));
        //first_addr = ram_first;
	kprintf("num frames is %d\n",total_frames);
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
        //        kprintf("%d\n",i);
        }
	spinlock_release(&stealmem_lock);
	kprintf("finished creating frametable\n");

}

paddr_t getppages(unsigned int npages){
        paddr_t nextaddr;

         if(frametable == NULL){
                spinlock_acquire(&stealmem_lock);
                nextaddr = ram_stealmem(npages);
                spinlock_release(&stealmem_lock);
        } else {
                if(npages > 1){
                        kprintf("npages greater than 1!\n");
                        return 0;
                }
                // maybe acquire a locker here !
		lock_acquire(frametable_lock);
                int i = 0;
                for(i=0;i<total_frames;i++){
//                        free_index = i;
                        if(frametable[i].is_free == true){
                                break;
                        }
                }
                if( i < total_frames){

                	frametable[i].is_free = false;
                  	nextaddr = frametable[i].physical_addr;
			lock_release(frametable_lock);
                }else{
		     kprintf("i greater than total frames!\n");
		     lock_release(frametable_lock);
                     return 0;
                }
                // CHECK IF i > total_frames !?!?!
        }
        //bzero((void *)PADDR_TO_KVADDR(nextaddr),PAGE_SIZE);
        
	/*struct spinlock frame_locker = SPINLOCK_INITIALIZER;
        spinlock_acquire(&frame_locker);
	if(frametable == NULL){
		nextaddr = ram_stealmem(npages);
	}else{
		nextaddr = frametable[curr_free_addr];
		
	}*/
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
		kprintf("addr is 0\n");
                return 0;
        }
        //kprintf("allocating physical address of %d\n",addr);
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

