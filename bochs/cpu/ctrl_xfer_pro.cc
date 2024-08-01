////////////////////////////////////////////////////////////////////////
// $Id: ctrl_xfer_pro.cc 12613 2015-01-25 20:55:10Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001-2015  The Bochs Project
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

/* pass zero in check_rpl if no needed selector RPL checking for
   non-conforming segments */
void BX_CPU_C::check_cs(bx_descriptor_t *descriptor, Bit16u cs_raw, Bit8u check_rpl, Bit8u check_cpl)
{
	BX_PANIC(("check_cs called"));
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::load_cs(bx_selector_t *selector, bx_descriptor_t *descriptor, Bit8u cpl)
{
	BX_PANIC(("load_cs called"));
}

void BX_CPU_C::branch_far64(bx_selector_t *selector,
           bx_descriptor_t *descriptor, bx_address rip, Bit8u cpl)
{
	BX_PANIC(("branch_far64 called"));
}
