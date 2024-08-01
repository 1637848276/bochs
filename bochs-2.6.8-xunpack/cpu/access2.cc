/////////////////////////////////////////////////////////////////////////
// $Id: access2.cc 12724 2015-04-19 20:47:55Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2008-2015 Stanislav Shwartsman
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
#include <intrin.h>

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "dmsg.h"
#include "gsaccess.h"


#define LOG_THIS BX_CPU_THIS_PTR

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_byte(unsigned s, bx_address laddr, Bit8u data)
{
	if (s == BX_SEG_REG_GS) {

        MSG((thread_index, OUTPUT_BOTH, "write byte GS:[%I64x]<- %x", laddr, data));
		GSWriteByte(laddr, data);

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));

	}
	else {
		writeByte(laddr, data);
	}
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_word(unsigned s, bx_address laddr, Bit16u data)
{
	if (s == BX_SEG_REG_GS) {

        MSG((thread_index, OUTPUT_BOTH, "write word GS:[%I64x]<- %x", laddr, data));
		GSWriteWord(laddr, data);


	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));

	}
	else {
		writeWord(laddr, data);
	}
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_dword(unsigned s, bx_address laddr, Bit32u data)
{
	if (s == BX_SEG_REG_GS) {

        MSG((thread_index, OUTPUT_BOTH, "write Dword GS:[%I64x]<- %x", laddr, data));
		GSWriteDword(laddr, data);

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));

	}
	else {
		writeDword(laddr, data);
	}
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_qword(unsigned s, bx_address laddr, Bit64u data)
{
	if (s == BX_SEG_REG_GS) {

        MSG((thread_index, OUTPUT_BOTH, "write Qword GS:[%I64x]<- %I64x", laddr, data));
		GSWriteQword(laddr, data);

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));

	}
	else {

		writeQword(laddr, data);
	}
}

#if BX_CPU_LEVEL >= 6

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_xmmword(unsigned s, bx_address laddr, const BxPackedXmmRegister *data)
{
	BX_PANIC(("write_linear_xmmword not implemented"));
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_xmmword_aligned(unsigned s, bx_address laddr, const BxPackedXmmRegister *data)
{
	BX_PANIC(("write_linear_xmmword_aligned not implemented"));
}

#if BX_SUPPORT_AVX
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_ymmword(unsigned s, bx_address laddr, const BxPackedYmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 31);
  Bit64u lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x04 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 32, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*) data);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      pageWriteStampTable.decWriteStamp(pAddr, 32);
      for (unsigned n = 0; n < 4; n++) {
        WriteHostQWordToLittleEndian(hostAddr+n, data->ymm64u(n));
      }
      return;
    }
  }

  if (access_write_linear(laddr, 32, CPL, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_ymmword_aligned(unsigned s, bx_address laddr, const BxPackedYmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  Bit64u lpf = AlignedAccessLPFOf(laddr, 31);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x04 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 32, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*) data);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      pageWriteStampTable.decWriteStamp(pAddr, 32);
      for (unsigned n = 0; n < 4; n++) {
        WriteHostQWordToLittleEndian(hostAddr+n, data->ymm64u(n));
      }
      return;
    }
  }

  if (laddr & 31) {
    BX_ERROR(("write_linear_ymmword_aligned(): #GP misaligned access"));
    exception(BX_GP_EXCEPTION, 0);
  }

  if (access_write_linear(laddr, 32, CPL, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}
#endif

#if BX_SUPPORT_EVEX
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_zmmword(unsigned s, bx_address laddr, const BxPackedZmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 63);
  Bit64u lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x04 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 64, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*) data);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      pageWriteStampTable.decWriteStamp(pAddr, 64);
      for (unsigned n = 0; n < 8; n++) {
        WriteHostQWordToLittleEndian(hostAddr+n, data->zmm64u(n));
      }
      return;
    }
  }

  if (access_write_linear(laddr, 64, CPL, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_linear_zmmword_aligned(unsigned s, bx_address laddr, const BxPackedZmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  Bit64u lpf = AlignedAccessLPFOf(laddr, 63);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x04 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      bx_phy_address pAddr = tlbEntry->ppf | pageOffset;
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, pAddr, 64, tlbEntry->get_memtype(), BX_WRITE, (Bit8u*) data);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      pageWriteStampTable.decWriteStamp(pAddr, 64);
      for (unsigned n = 0; n < 8; n++) {
        WriteHostQWordToLittleEndian(hostAddr+n, data->zmm64u(n));
      }
      return;
    }
  }

  if (laddr & 63) {
    BX_ERROR(("write_linear_zmmword_aligned(): #GP misaligned access"));
    exception(BX_GP_EXCEPTION, 0);
  }

  if (access_write_linear(laddr, 64, CPL, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}
#endif

#endif

  Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_byte(unsigned s, bx_address laddr)
{
  Bit8u data;

  if (s == BX_SEG_REG_GS) {

	  Bit8u data = GSReadByte(laddr);
      MSG((thread_index, OUTPUT_BOTH, "read Byte GS:[%llx]=%x", laddr, data));
	  return data;


  }
  else if (s == BX_SEG_REG_FS) {
	  BX_PANIC(("Seg FS accessed."));
	  return 0;

  }
  else {
	  return readByte(laddr);
  }

}

  Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_word(unsigned s, bx_address laddr)
{
	if (s == BX_SEG_REG_GS) {

		Bit16u data = GSReadWord(laddr);
        MSG((thread_index, OUTPUT_BOTH, "read Word GS:[%llx]=%x", laddr, data));
		return data;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readWord(laddr);
	}
}

  Bit32u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_dword(unsigned s, bx_address laddr)
{
	if (s == BX_SEG_REG_GS) {

		Bit32u data = GSReadDword(laddr);
        MSG((thread_index, OUTPUT_BOTH, "read Dword GS:[%llx]=%x", laddr, data));
		return data;


	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readDword(laddr);
	}
}

  Bit64u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_linear_qword(unsigned s, bx_address laddr)
{
	if (s == BX_SEG_REG_GS) {

		Bit64u data = GSReadQword(laddr);
        if(laddr == 0x30) {
            MSG((thread_index, OUTPUT_BOTH, "TEB accessed: %llx", _TEB));
            data = _TEB;
        }
        if(laddr == 0x10) {   // stack
            data = bx_cpu._stack;
            MSG((thread_index, OUTPUT_BOTH, "Stack limit accessed: %llx", data));

        }
        MSG((thread_index, OUTPUT_BOTH, "read Qword GS:[%llx]=%I64x", laddr, data));

#if 0
        static int cnt = 0;
        if(thread_index == 2)  cnt++;
        if(cnt == 1 && thread_index == 2) {
            MSG((0, OUTPUT_DEBUG," TP1..."));
            SetDebug(1);
        }
        if(cnt == 2 && thread_index == 2) {
            SetDebug(0);
            MSG((0, OUTPUT_DEBUG," TP2..."));
        }
#endif
        return data;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readQword(laddr);
	}
}

#if BX_CPU_LEVEL >= 6

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_xmmword(unsigned s, bx_address laddr, BxPackedXmmRegister *data)
{
	BX_PANIC(("read_linear_xmmword not implemented"));
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_xmmword_aligned(unsigned s, bx_address laddr, BxPackedXmmRegister *data)
{
	BX_PANIC(("read_linear_xmmword_aligned not implemented"));
}

#if BX_SUPPORT_AVX
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_ymmword(unsigned s, bx_address laddr, BxPackedYmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 31);
  Bit64u lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (0x01 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      for (unsigned n=0; n < 4; n++) {
        ReadHostQWordFromLittleEndian(hostAddr+n, data->ymm64u(n));
      }
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 32, tlbEntry->get_memtype(), BX_READ, (Bit8u*) data);
      return;
    }
  }

  if (access_read_linear(laddr, 32, CPL, BX_READ, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_ymmword_aligned(unsigned s, bx_address laddr, BxPackedYmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  Bit64u lpf = AlignedAccessLPFOf(laddr, 31);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (0x01 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      for (unsigned n=0; n < 4; n++) {
        ReadHostQWordFromLittleEndian(hostAddr+n, data->ymm64u(n));
      }
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 32, tlbEntry->get_memtype(), BX_READ, (Bit8u*) data);
      return;
    }
  }

  if (laddr & 31) {
    BX_ERROR(("read_linear_ymmword_aligned(): #GP misaligned access"));
    exception(BX_GP_EXCEPTION, 0);
  }

  if (access_read_linear(laddr, 32, CPL, BX_READ, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}
#endif

#if BX_SUPPORT_EVEX
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_zmmword(unsigned s, bx_address laddr, BxPackedZmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 63);
  Bit64u lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (0x01 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      for (unsigned n=0; n < 8; n++) {
        ReadHostQWordFromLittleEndian(hostAddr+n, data->zmm64u(n));
      }
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 64, tlbEntry->get_memtype(), BX_READ, (Bit8u*) data);
      return;
    }
  }

  if (access_read_linear(laddr, 64, CPL, BX_READ, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_linear_zmmword_aligned(unsigned s, bx_address laddr, BxPackedZmmRegister *data)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  Bit64u lpf = AlignedAccessLPFOf(laddr, 63);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (0x01 << USER_PL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
      for (unsigned n=0; n < 8; n++) {
        ReadHostQWordFromLittleEndian(hostAddr+n, data->zmm64u(n));
      }
      BX_NOTIFY_LIN_MEMORY_ACCESS(laddr, (tlbEntry->ppf | pageOffset), 64, tlbEntry->get_memtype(), BX_READ, (Bit8u*) data);
      return;
    }
  }

  if (laddr & 63) {
    BX_ERROR(("read_linear_zmmword_aligned(): #GP misaligned access"));
    exception(BX_GP_EXCEPTION, 0);
  }

  if (access_read_linear(laddr, 64, CPL, BX_READ, 0x0, (void *) data) < 0)
    exception(int_number(s), 0);
}
#endif

#endif

//////////////////////////////////////////////////////////////
// special Read-Modify-Write operations                     //
// address translation info is kept across read/write calls //
//////////////////////////////////////////////////////////////

  Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_linear_byte(unsigned s, bx_address laddr)
{
	RMW_laddr = laddr;

	if (s == BX_SEG_REG_GS) {

		BX_PANIC(("Seg GS accessed."));
		return 0;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readByte(RMW_laddr);
	}
}

  Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_linear_word(unsigned s, bx_address laddr)
{
	RMW_laddr = laddr;
	if (s == BX_SEG_REG_GS) {

		BX_PANIC(("Seg GS accessed."));
		return 0;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readWord(RMW_laddr);
	}
}

  Bit32u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_linear_dword(unsigned s, bx_address laddr)
{
	RMW_laddr = laddr;
	if (s == BX_SEG_REG_GS) {

		BX_PANIC(("Seg GS accessed."));
		return 0;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readDword(RMW_laddr);
	}
}

  Bit64u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_linear_qword(unsigned s, bx_address laddr)
{
	RMW_laddr = laddr;
	if (s == BX_SEG_REG_GS) {

		BX_PANIC(("Seg GS accessed."));
		return 0;

	}
	else if (s == BX_SEG_REG_FS) {
		BX_PANIC(("Seg FS accessed."));
		return 0;

	}
	else {
		return readQword(RMW_laddr);
	}
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_linear_byte(Bit8u val8)
{
	writeByte(RMW_laddr, val8);
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_linear_word(Bit16u val16)
{
    writeWord(RMW_laddr, val16);
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_linear_dword(Bit32u val32)
{
	writeDword(RMW_laddr, val32);
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_linear_qword(Bit64u val64)
{
	writeQword(RMW_laddr, val64);
}

#if BX_SUPPORT_X86_64

void BX_CPU_C::read_RMW_linear_dqword_aligned_64(unsigned s, bx_address laddr, Bit64u *hi, Bit64u *lo)
{
	RMW_laddr = laddr;
	*lo = readQword(RMW_laddr);
	*hi = readQword(RMW_laddr + 8);
}

void BX_CPU_C::write_RMW_linear_dqword(Bit64u hi, Bit64u lo)
{
	writeQword(RMW_laddr, lo);
	writeQword(RMW_laddr + 8, hi);
}

#endif

//
// Write data to new stack, these methods are required for emulation
// correctness but not performance critical.
//

void BX_CPU_C::write_new_stack_word(bx_address laddr, unsigned curr_pl, Bit16u data)
{
	writeWord_ns(laddr, data);
}

void BX_CPU_C::write_new_stack_dword(bx_address laddr, unsigned curr_pl, Bit32u data)
{
	writeDword_ns(laddr, data);
}

void BX_CPU_C::write_new_stack_qword(bx_address laddr, unsigned curr_pl, Bit64u data)
{
	writeQword_ns(laddr, data);
}

// assuming the write happens in 32-bit mode
void BX_CPU_C::write_new_stack_word(bx_segment_reg_t *seg, Bit32u offset, unsigned curr_pl, Bit16u data)
{
	BX_PANIC(("new stack word 32bit not implemented\n"));

}

// assuming the write happens in 32-bit mode
void BX_CPU_C::write_new_stack_dword(bx_segment_reg_t *seg, Bit32u offset, unsigned curr_pl, Bit32u data)
{
	BX_PANIC(("new stack dword 32bit not implemented\n"));
}

// assuming the write happens in 32-bit mode
void BX_CPU_C::write_new_stack_qword(bx_segment_reg_t *seg, Bit32u offset, unsigned curr_pl, Bit64u data)
{
	BX_PANIC(("new stack qword 32bit not implemented\n"));
}
