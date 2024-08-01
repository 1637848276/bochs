////////////////////////////////////////////////////////////////////////
// $Id: call_far.cc 12497 2014-10-12 18:59:10Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2005-2012 Stanislav Shwartsman
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
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::call_protected(bxInstruction_c *i, Bit16u cs_raw, bx_address disp)
{
	BX_PANIC(("call_prtected called"));
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::call_gate(bx_descriptor_t *gate_descriptor)
{
	BX_PANIC(("call_gate called"));
}

#if BX_SUPPORT_X86_64
void BX_CPP_AttrRegparmN(1) BX_CPU_C::call_gate64(bx_selector_t *gate_selector)
{
	BX_PANIC(("call_gate64 called"));
}
#endif
