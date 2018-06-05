/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *        The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */
void flush_tlb(void){
        int spl = splhigh();
        for (int i = 0; i < NUM_TLB; i++) {
                tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        }
        splx(spl);
}
struct addrspace *
as_create(void)
{
        struct addrspace *as;

        as = kmalloc(sizeof(struct addrspace));
        if (as == NULL) {
                return NULL;
        }
        as->n_regions = 0;
        as->region_head = NULL;
        as->pid  = NULL; //maybe as
        as->as_stackpbase = NULL;
        return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
        struct addrspace newas;
        newas = as_create();

        if (newas==NULL) {
                return ENOMEM;
        }
        struct region* old_region = old->region_head;
        struct region* new_region = newas->region_head;

        while(old_region != NULL){
                as_define_region(newas, 
                                old_region->vbase, 
                                old_region->npages * PAGE_SIZE,
                                old_region->readable,
                                old_region->writeable,
                                old_region->executable);
                old_region = old_region->next_region;
                new_region = new_region->next_region;

                memmove((void *)new_region->vbase,
                        (void )old_region->vbase,
                        PAGE_SIZEold_region->npages);
        }


        *ret = newas;
        return 0;
}

void
as_destroy(struct addrspace *as)
{
        struct as_region * curr, * next;
        if(as->region_head != 0){
                panic("no regions to free!\n");
                return;
        }
        curr = as->region_head;
        while(curr != NULL) {
                next = curr->next_region;
                kfree(curr);
                curr = next;
        }
        kfree(as);
}

void
as_activate(void)
{
        struct addrspace *as;

        as = proc_getas();
        if (as == NULL) {
                /*
                 * Kernel thread without an address space; leave the
                 * prior address space in place.
                 */
                return;
        }
        flush_tlb();
       
}

void
as_deactivate(void)
{
        flush_tlb();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
                 int readable, int writeable, int executable)
{
        /*
         * Write this.
         */
         if(as == NULL){
                 panic("as not initialised!\n");
                 return EINVAL;
        }
        struct region* new_region = (struct region*) kmalloc(sizeof(struct region));
        if(new_region == NULL){
                panic("not enough memory to malloc new region!\n");
                return ENOMEM;
        }

        new_region->vbase = vaddr - vaddr % PAGE_SIZE;
        new_region->npages = (memsize + vaddr % PAGE_SIZE + PAGE_SIZE - 1 ) / PAGE_SIZE ;
        new_region->readable = readable;
        new_region->executable = executable;
        new_region->writeable = writeable;
        new_region->next_region = NULL;

        if(as->region_head == NULL){
                as->region_head = new_region;
                as->n_regions++;
        }
        else{
                struct region* tmp = as->region_head;
                // may need a previous if inserting in the middle. 
                while(tmp->next_region != NULL){
                        tmp = tmp->next_region;
                }

                tmp->next_region = new_region;
                as->n_regions++;
        }
        return 0;
}

int
as_prepare_load(struct addrspace *as)
{
        struct region *tmp = as->region_head;
        while(tmp->next_region != NULL){
                tmp->writeable = 1;
                bzero((void *)tmp->vbase, tmp->npages * PAGE_SIZE);
                tmp = tmp->next_region;
        }
        return 0;
}

int
as_complete_load(struct addrspace *as)
{
        (void)as;
        return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
        as_define_region(as, USERSTACK - 16 * PAGE_SIZE , 16 * PAGE_SIZE, 1, 1, 1);
        /* Initial user-level stack pointer */
        *stackptr = USERSTACK;

        return 0;
}

