#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <spl.h>
#include <proc.h>

struct page_entry *pagetable = NULL; //swap to 0 if needed!
int num_pages = 0;
/* Place your page table functions here */

int create_pagetable(void){
	struct spinlock creation_lock = SPINLOCK_INITIALIZER;
        spinlock_acquire(&creation_lock);

     	 num_pages = (ram_getsize()/PAGE_SIZE) * 2;
        int pagetable_size = 1 + num_pages * sizeof(struct page_entry)/PAGE_SIZE;

        //struct spinlock creation_lock = SPINLOCK_INITIALIZER;
        //spinlock_acquire(&creation_lock);
        pagetable = (struct page_entry *)PADDR_TO_KVADDR(ram_stealmem(pagetable_size));
        //pagetable = (struct page_entry *)kmalloc(pagetable_size);
	//spinlock_release(&creation_lock);
        if(pagetable == 0){
                return ENOMEM;
        }
        for(int i=0;i<num_pages;i++){
                pagetable[i].frame_address = 0;
                pagetable[i].page_address = 0;
                pagetable[i].pid = NULL; //might have to be 0'ed
                pagetable[i].next_page = NULL; //might have to be  0'ed 
        }
        kprintf("initialised pagetable \n");
        spinlock_release(&creation_lock);
	return 0;
}

int insert_page(struct addrspace* as,vaddr_t page_address){
        //int current_page_index = page_address / (num_pages/2);
        int current_page_index = hpt_hash(as, page_address);

        int free_frame_index = -1; 
	//kprintf("started insert_page\n");
        //find the index of the last page in the (possible) collision chain.  
        while(pagetable[current_page_index].next_page != NULL){
                //POSSIBLE POE
                current_page_index = pagetable[current_page_index].next_page->page_address / num_pages;
        }
        //kprintf("finished finding current_page_index \n");

        int external_index = num_pages / 2;
        //find free empty page. 
        while(external_index < num_pages){
                if(pagetable[external_index].frame_address == 0){
                        free_frame_index = external_index;
                        break;
                }
                external_index ++;
        }
        //kprintf("found external index\n");
        if(external_index >= num_pages-1 || free_frame_index == -1){
                return ENOMEM;
        }
        //HASH FUNCTION USED BEFORE HERE!!!
        //linking the last page to the empty page through next. 
        pagetable[current_page_index].next_page = &pagetable[free_frame_index];

        // checking if in the first half of the page table.
        if (current_page_index < num_pages / 2){
                free_frame_index = current_page_index;        
        }

        pagetable[free_frame_index].frame_address = alloc_kpages(1);
        bzero((void *)pagetable[free_frame_index].frame_address,  PAGE_SIZE);

        struct addrspace *curr_as;
        curr_as = proc_getas();

        pagetable[free_frame_index].next_page = 0;
        pagetable[free_frame_index].page_address = page_address - page_address % PAGE_SIZE;
        pagetable[free_frame_index].pid = curr_as;
	       uint32_t elo,ehi;
        int spl = splhigh();

	//int spl = splhigh();
        elo = KVADDR_TO_PADDR(pagetable[free_frame_index].frame_address)| TLBLO_DIRTY | TLBLO_VALID;
        ehi = page_address & TLBHI_VPAGE;
        tlb_random(ehi, elo | TLBLO_VALID | TLBLO_DIRTY);
//        int spl = splhigh();
        splx(spl);
        //kprintf("finished insert_page\n");

        return 0;
}

//change to bool !!! if needed.
int lookup_pagetable(vaddr_t lookup_address,struct addrspace *pid){
        // int current_page_index = lookup_address / num_pages;
        //kprintf("started lookup_pagetable\n");
        int current_page_index = hpt_hash(pid,lookup_address);
        while(pagetable[current_page_index].pid != pid || pagetable[current_page_index].page_address != (lookup_address - lookup_address % PAGE_SIZE)){
                //POSSIBLE POE
                if(pagetable[current_page_index].next_page == NULL){
                        //panic("no next page\n"); ENTERS THIS FOR FAULTER - GOOD
			return -1;
                }
                current_page_index = pagetable[current_page_index].next_page->page_address / num_pages;
        }
        //kprintf("found current page index in lookup\n");
        if( pagetable[current_page_index].frame_address== 0 || pagetable[current_page_index].pid != pid){                
                return -1;
        }	
	uint32_t elo,ehi;
        int spl = splhigh();
	
        elo = KVADDR_TO_PADDR(pagetable[current_page_index].frame_address)| TLBLO_DIRTY | TLBLO_VALID;
        ehi = lookup_address & TLBHI_VPAGE;
        tlb_random(ehi, elo | TLBLO_VALID | TLBLO_DIRTY);
	
        //int spl = splhigh();
        splx(spl);
        //kprintf("finished lookup page table\n");
        return 0;
}

int lookup_region(vaddr_t lookup_address,struct addrspace *as){
        struct region *curr = as->region_head;
	
	//panic("looking up region for %d\n",lookup_address);
        //kprintf("started lookup region\n");
        //loop through all regions after the first one.
        while(curr != NULL){
	//	panic("just entered while\n");
                if(lookup_address >= curr->vbase && 
                lookup_address <= curr->vbase + curr->npages * PAGE_SIZE ){
                        for (unsigned int i = 0; i< as->region_head->npages; i++ ){
			//	panic("inserting page\n"); //does not even reach here for faulter!!!
//				kprintf("for loop\n");
                                insert_page(as,lookup_address + i * PAGE_SIZE);
				break;
			}
//			panic("found address %d\n",lookup_address);
                        return 0;
                }
//		panic("about to get the next region\n");
//		kprintf("error spot?\n");
                curr = curr->next_region;
		
		

        }
//	panic("address not found in any region %d\n",lookup_address);
        return -1;
}

void vm_bootstrap(void)
{
	create_pagetable();
	init_frametable();
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
        if(faulttype == VM_FAULT_READONLY){
                return EFAULT;
        }

        struct addrspace *as = NULL;
        as = proc_getas();
        struct addrspace *pid = NULL;
	pid = as->pid;
	
	if(as == NULL) return EFAULT;
	
        int err = lookup_pagetable(faultaddress, pid);

        if(err == -1){
                err = lookup_region(faultaddress, pid);
                if(err == -1){
                        return EFAULT;
                }
        }
        return 0;
}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
        (void)ts;
        panic("vm tried to do tlb shootdown?!\n");
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr){
        uint32_t index;
        index = (((uint32_t )as) ^ (faultaddr >> PAGE_BITS)) % num_pages ;
        return index;
}

