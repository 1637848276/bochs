/////////////////////////////////////////////////////////////////////////
// $Id: access.cc 12655 2015-02-19 20:23:08Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005-2015  The Bochs Project
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

bx_address bx_asize_mask[] = {
  0xffff,                         // as16 (asize = '00)
  0xffffffff,                     // as32 (asize = '01)
#if BX_SUPPORT_X86_64
  BX_CONST64(0xffffffffffffffff), // as64 (asize = '10)
  BX_CONST64(0xffffffffffffffff)  // as64 (asize = '11)
#endif
};

#if BX_SUPPORT_EVEX
  #define BX_MAX_MEM_ACCESS_LENGTH 64
#else
  #if BX_SUPPORT_AVX
    #define BX_MAX_MEM_ACCESS_LENGTH 32
  #else
    #define BX_MAX_MEM_ACCESS_LENGTH 16
  #endif
#endif

  bx_bool BX_CPP_AttrRegparmN(4)
BX_CPU_C::write_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length, bx_bool align)
{
  return 1;
}

  bx_bool BX_CPP_AttrRegparmN(4)
BX_CPU_C::read_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length, bx_bool align)
{

  return 1;
}

  bx_bool BX_CPP_AttrRegparmN(3)
BX_CPU_C::execute_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length)
{
  return 1;
}

const char *BX_CPU_C::strseg(bx_segment_reg_t *seg)
{
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES]) return("ES");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS]) return("CS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS]) return("SS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS]) return("DS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS]) return("FS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS]) return("GS");
  else {
    BX_PANIC(("undefined segment passed to strseg()!"));
    return("??");
  }
}

int BX_CPU_C::int_number(unsigned s)
{
  if (s == BX_SEG_REG_SS)
    return BX_SS_EXCEPTION;
  else
    return BX_GP_EXCEPTION;
}

  Bit8u BX_CPP_AttrRegparmN(1)
BX_CPU_C::system_read_byte(bx_address laddr)
{
  Bit8u data; 
  data = sreadByte(laddr);
  return data;
}

  Bit16u BX_CPP_AttrRegparmN(1)
BX_CPU_C::system_read_word(bx_address laddr)
{
  Bit16u data;

  data = sreadWord(laddr);

  return data;
}

  Bit32u BX_CPP_AttrRegparmN(1)
BX_CPU_C::system_read_dword(bx_address laddr)
{
  Bit32u data;

  data = sreadDword(laddr);

  return data;
}

  Bit64u BX_CPP_AttrRegparmN(1)
BX_CPU_C::system_read_qword(bx_address laddr)
{
  Bit64u data;

  data = sreadQword(laddr);

  return data;
}

  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::system_write_byte(bx_address laddr, Bit8u data)
{
	swriteByte(laddr, data);
}

  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::system_write_word(bx_address laddr, Bit16u data)
{
	swriteWord(laddr, data);
}

  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::system_write_dword(bx_address laddr, Bit32u data)
{
	swriteDword(laddr, data);
}

  Bit8u* BX_CPP_AttrRegparmN(2)
BX_CPU_C::v2h_read_byte(bx_address laddr, bx_bool user)
{

	BX_PANIC(("vh_read_byte not implemented"));

  return 0;
}

  Bit8u* BX_CPP_AttrRegparmN(2)
BX_CPU_C::v2h_write_byte(bx_address laddr, bx_bool user)
{
  BX_PANIC(("vh_write_byte not implemented"));
  return 0;
}
