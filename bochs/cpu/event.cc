/////////////////////////////////////////////////////////////////////////
// $Id: event.cc 11804 2013-09-05 18:40:14Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2011-2013 Stanislav Shwartsman
//          Written by Stanislav Shwartsman [sshwarts at sourceforge net]
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA B 02110-1301 USA
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR


bx_bool BX_CPU_C::handleWaitForEvent(void)
{
    BX_TICKN(10); // when in HLT run time faster for single CPU

  BX_PANIC(("handleWaitForEvent CALLED"));

  return 0;
}

void BX_CPU_C::InterruptAcknowledge(void)
{

  BX_PANIC(("InterruptAcknowledge CALLED"));

}

#if BX_SUPPORT_SVM
void BX_CPU_C::VirtualInterruptAcknowledge(void)
{
  Bit8u vector = SVM_V_INTR_VECTOR;

  if (SVM_INTERCEPT(SVM_INTERCEPT0_VINTR)) Svm_Vmexit(SVM_VMEXIT_VINTR);

  clear_event(BX_EVENT_SVM_VIRQ_PENDING);

  BX_CPU_THIS_PTR EXT = 1; /* external event */

  BX_INSTR_HWINTERRUPT(BX_CPU_ID, vector, 
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, RIP);
  interrupt(vector, BX_EXTERNAL_INTERRUPT, 0, 0);

  BX_CPU_THIS_PTR prev_rip = RIP; // commit new RIP
}
#endif

bx_bool BX_CPU_C::handleAsyncEvent(void)
{
     BX_PANIC(("handleAsyncEvent CALLED"));
    return 0;
}

// Certain instructions inhibit interrupts, some debug exceptions and single-step traps.
void BX_CPU_C::inhibit_interrupts(unsigned mask)
{
    BX_INFO(("### inhibit_interrupts"));
}

bx_bool BX_CPU_C::interrupts_inhibited(unsigned mask)
{
  //return (get_icount() <= BX_CPU_THIS_PTR inhibit_icount) && (BX_CPU_THIS_PTR inhibit_mask & mask) == mask;
    BX_INFO(("### interrupts_inhibited called"));
    return 0;
}

void BX_CPU_C::deliver_SIPI(unsigned vector)
{

    BX_INFO(("### deliver_SIPI called"));
}

void BX_CPU_C::deliver_INIT(void)
{
        BX_INFO(("### deliver_INIT called"));
}

void BX_CPU_C::deliver_NMI(void)
{
   BX_INFO(("### deliver__NMI called"));
}

void BX_CPU_C::deliver_SMI(void)
{
 BX_INFO(("### deliver__SMI called"));
}

void BX_CPU_C::raise_INTR(void)
{
  BX_INFO(("### raise_INTR called"));
}

void BX_CPU_C::clear_INTR(void)
{
  BX_INFO(("### clear_INTR called"));
}

#if BX_DEBUGGER

void BX_CPU_C::dbg_take_dma(void)
{
  // NOTE: similar code in ::cpu_loop()
  if (BX_HRQ) {
    BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
    DEV_dma_raise_hlda();
  }
}

#endif  // #if BX_DEBUGGER
