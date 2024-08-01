/////////////////////////////////////////////////////////////////////////
// $Id: tasking.cc 12392 2014-07-03 06:40:42Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001-2014  The Bochs Project
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

// Notes:
// ======

  // ======================
  // 286 Task State Segment
  // ======================
  // dynamic item                      | hex  dec  offset
  // 0       task LDT selector         | 2a   42
  // 1       DS selector               | 28   40
  // 1       SS selector               | 26   38
  // 1       CS selector               | 24   36
  // 1       ES selector               | 22   34
  // 1       DI                        | 20   32
  // 1       SI                        | 1e   30
  // 1       BP                        | 1c   28
  // 1       SP                        | 1a   26
  // 1       BX                        | 18   24
  // 1       DX                        | 16   22
  // 1       CX                        | 14   20
  // 1       AX                        | 12   18
  // 1       flag word                 | 10   16
  // 1       IP (entry point)          | 0e   14
  // 0       SS for CPL 2              | 0c   12
  // 0       SP for CPL 2              | 0a   10
  // 0       SS for CPL 1              | 08   08
  // 0       SP for CPL 1              | 06   06
  // 0       SS for CPL 0              | 04   04
  // 0       SP for CPL 0              | 02   02
  //         back link selector to TSS | 00   00


  // ======================
  // 386 Task State Segment
  // ======================
  // |31            16|15                    0| hex dec
  // |I/O Map Base    |000000000000000000000|T| 64  100 static
  // |0000000000000000| LDT                   | 60  96  static
  // |0000000000000000| GS selector           | 5c  92  dynamic
  // |0000000000000000| FS selector           | 58  88  dynamic
  // |0000000000000000| DS selector           | 54  84  dynamic
  // |0000000000000000| SS selector           | 50  80  dynamic
  // |0000000000000000| CS selector           | 4c  76  dynamic
  // |0000000000000000| ES selector           | 48  72  dynamic
  // |                EDI                     | 44  68  dynamic
  // |                ESI                     | 40  64  dynamic
  // |                EBP                     | 3c  60  dynamic
  // |                ESP                     | 38  56  dynamic
  // |                EBX                     | 34  52  dynamic
  // |                EDX                     | 30  48  dynamic
  // |                ECX                     | 2c  44  dynamic
  // |                EAX                     | 28  40  dynamic
  // |                EFLAGS                  | 24  36  dynamic
  // |                EIP (entry point)       | 20  32  dynamic
  // |           CR3 (PDPR)                   | 1c  28  static
  // |000000000000000 | SS for CPL 2          | 18  24  static
  // |           ESP for CPL 2                | 14  20  static
  // |000000000000000 | SS for CPL 1          | 10  16  static
  // |           ESP for CPL 1                | 0c  12  static
  // |000000000000000 | SS for CPL 0          | 08  08  static
  // |           ESP for CPL 0                | 04  04  static
  // |000000000000000 | back link to prev TSS | 00  00  dynamic (updated only when return expected)


  // ==================================================
  // Effect of task switch on Busy, NT, and Link Fields
  // ==================================================

  // Field         jump        call/interrupt     iret
  // ------------------------------------------------------
  // new busy bit  Set         Set                No change
  // old busy bit  Cleared     No change          Cleared
  // new NT flag   No change   Set                No change
  // old NT flag   No change   No change          Cleared
  // new link      No change   old TSS selector   No change
  // old link      No change   No change          No change
  // CR0.TS        Set         Set                Set

  // Note: I checked 386, 486, and Pentium, and they all exhibited
  //       exactly the same behaviour as above.  There seems to
  //       be some misprints in the Intel docs.

void BX_CPU_C::task_switch(bxInstruction_c *i, bx_selector_t *tss_selector,
                 bx_descriptor_t *tss_descriptor, unsigned source,
                 Bit32u dword1, Bit32u dword2, bx_bool push_error, Bit32u error_code)
{
	BX_PANIC(("TASK-SWITCH CALLED"));
}

void BX_CPU_C::task_switch_load_selector(bx_segment_reg_t *seg,
                 bx_selector_t *selector, Bit16u raw_selector, Bit8u cs_rpl)
{
	BX_PANIC(("task_switch_load_selector CALLED"));
}
void BX_CPU_C::get_SS_ESP_from_TSS(unsigned pl, Bit16u *ss, Bit32u *esp)
{
	BX_PANIC(("get_SS_ESP_from_TSS CALLED"));
}

#if BX_SUPPORT_X86_64
Bit64u BX_CPU_C::get_RSP_from_TSS(unsigned pl)
{
	BX_PANIC(("get_RSP_from_TSS CALLED"));
  return -1;
}
#endif  // #if BX_SUPPORT_X86_64
