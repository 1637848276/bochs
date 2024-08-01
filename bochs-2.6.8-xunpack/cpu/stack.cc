/////////////////////////////////////////////////////////////////////////
// $Id: stack.cc 12667 2015-02-22 21:26:26Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012-2015 Stanislav Shwartsman
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

#include "cpustats.h"

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stackPrefetch(bx_address offset, unsigned len)
{
	BX_ERROR(("stackPrefetch called"));
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stack_write_byte(bx_address offset, Bit8u data)
{

    write_virtual_byte(BX_SEG_REG_SS, offset, data);
  
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stack_write_word(bx_address offset, Bit16u data)
{
    write_virtual_word(BX_SEG_REG_SS, offset, data);
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stack_write_dword(bx_address offset, Bit32u data)
{
    write_virtual_dword(BX_SEG_REG_SS, offset, data);
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::stack_write_qword(bx_address offset, Bit64u data)
{
    write_virtual_qword(BX_SEG_REG_SS, offset, data);
}

Bit8u BX_CPP_AttrRegparmN(1) BX_CPU_C::stack_read_byte(bx_address offset)
{
    return read_virtual_byte(BX_SEG_REG_SS, offset);
}

Bit16u BX_CPP_AttrRegparmN(1) BX_CPU_C::stack_read_word(bx_address offset)
{

    return read_virtual_word(BX_SEG_REG_SS, offset);

}

Bit32u BX_CPP_AttrRegparmN(1) BX_CPU_C::stack_read_dword(bx_address offset)
{
    return read_virtual_dword(BX_SEG_REG_SS, offset);  
}

Bit64u BX_CPP_AttrRegparmN(1) BX_CPU_C::stack_read_qword(bx_address offset)
{
    return read_virtual_qword(BX_SEG_REG_SS, offset);

}
