/////////////////////////////////////////////////////////////////////////
// $Id: paging.cc 12725 2015-04-21 08:20:28Z sshwarts $
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

// X86 Registers Which Affect Paging:
// ==================================
//
// CR0:
//   bit 31: PG, Paging (386+)
//   bit 16: WP, Write Protect (486+)
//     0: allow   supervisor level writes into user level RO pages
//     1: inhibit supervisor level writes into user level RO pages
//
// CR3:
//   bit 31..12: PDBR, Page Directory Base Register (386+)
//   bit      4: PCD, Page level Cache Disable (486+)
//     Controls caching of current page directory.  Affects only the processor's
//     internal caches (L1 and L2).
//     This flag ignored if paging disabled (PG=0) or cache disabled (CD=1).
//     Values:
//       0: Page Directory can be cached
//       1: Page Directory not cached
//   bit      3: PWT, Page level Writes Transparent (486+)
//     Controls write-through or write-back caching policy of current page
//     directory.  Affects only the processor's internal caches (L1 and L2).
//     This flag ignored if paging disabled (PG=0) or cache disabled (CD=1).
//     Values:
//       0: write-back caching enabled
//       1: write-through caching enabled
//
// CR4:
//   bit 4: PSE, Page Size Extension (Pentium+)
//     0: 4KByte pages (typical)
//     1: 4MByte or 2MByte pages
//   bit 5: PAE, Physical Address Extension (Pentium Pro+)
//     0: 32bit physical addresses
//     1: 36bit physical addresses
//   bit 7: PGE, Page Global Enable (Pentium Pro+)
//     The global page feature allows frequently used or shared pages
//     to be marked as global (PDE or PTE bit 8).  Global pages are
//     not flushed from TLB on a task switch or write to CR3.
//     Values:
//       0: disables global page feature
//       1: enables global page feature
//
//    page size extention and physical address size extention matrix (legacy mode)
//   ==============================================================================
//   CR0.PG  CR4.PAE  CR4.PSE  PDPE.PS  PDE.PS | page size   physical address size
//   ==============================================================================
//      0       X        X       R         X   |   --          paging disabled
//      1       0        0       R         X   |   4K              32bits
//      1       0        1       R         0   |   4K              32bits
//      1       0        1       R         1   |   4M              32bits
//      1       1        X       R         0   |   4K              36bits
//      1       1        X       R         1   |   2M              36bits

//     page size extention and physical address size extention matrix (long mode)
//   ==============================================================================
//   CR0.PG  CR4.PAE  CR4.PSE  PDPE.PS  PDE.PS | page size   physical address size
//   ==============================================================================
//      1       1        X       0         0   |   4K              52bits
//      1       1        X       0         1   |   2M              52bits
//      1       1        X       1         -   |   1G              52bits


// Page Directory/Table Entry Fields Defined:
// ==========================================
// NX: No Execute
//   This bit controls the ability to execute code from all physical
//   pages mapped by the table entry.
//     0: Code can be executed from the mapped physical pages
//     1: Code cannot be executed
//   The NX bit can only be set when the no-execute page-protection
//   feature is enabled by setting EFER.NXE=1, If EFER.NXE=0, the
//   NX bit is treated as reserved. In this case, #PF occurs if the
//   NX bit is not cleared to zero.
//
// G: Global flag
//   Indiciates a global page when set.  When a page is marked
//   global and the PGE flag in CR4 is set, the page table or
//   directory entry for the page is not invalidated in the TLB
//   when CR3 is loaded or a task switch occurs.  Only software
//   clears and sets this flag.  For page directory entries that
//   point to page tables, this flag is ignored and the global
//   characteristics of a page are set in the page table entries.
//
// PS: Page Size flag
//   Only used in page directory entries.  When PS=0, the page
//   size is 4KBytes and the page directory entry points to a
//   page table.  When PS=1, the page size is 4MBytes for
//   normal 32-bit addressing and 2MBytes if extended physical
//   addressing.
//
// PAT: Page-Attribute Table
//   This bit is only present in the lowest level of the page
//   translation hierarchy. The PAT bit is the high-order bit
//   of a 3-bit index into the PAT register. The other two
//   bits involved in forming the index are the PCD and PWT
//   bits.
//
// D: Dirty bit:
//   Processor sets the Dirty bit in the 2nd-level page table before a
//   write operation to an address mapped by that page table entry.
//   Dirty bit in directory entries is undefined.
//
// A: Accessed bit:
//   Processor sets the Accessed bits in both levels of page tables before
//   a read/write operation to a page.
//
// PCD: Page level Cache Disable
//   Controls caching of individual pages or page tables.
//   This allows a per-page based mechanism to disable caching, for
//   those pages which contained memory mapped IO, or otherwise
//   should not be cached.  Processor ignores this flag if paging
//   is not used (CR0.PG=0) or the cache disable bit is set (CR0.CD=1).
//   Values:
//     0: page or page table can be cached
//     1: page or page table is not cached (prevented)
//
// PWT: Page level Write Through
//   Controls the write-through or write-back caching policy of individual
//   pages or page tables.  Processor ignores this flag if paging
//   is not used (CR0.PG=0) or the cache disable bit is set (CR0.CD=1).
//   Values:
//     0: write-back caching
//     1: write-through caching
//
// U/S: User/Supervisor level
//   0: Supervisor level - for the OS, drivers, etc.
//   1: User level - application code and data
//
// R/W: Read/Write access
//   0: read-only access
//   1: read/write access
//
// P: Present
//   0: Not present
//   1: Present
// ==========================================

// Combined page directory/page table protection:
// ==============================================
// There is one column for the combined effect on a 386
// and one column for the combined effect on a 486+ CPU.
// The 386 CPU behavior is not supported by Bochs.
//
// +----------------+-----------------+----------------+----------------+
// |  Page Directory|     Page Table  |   Combined 386 |  Combined 486+ |
// |Privilege  Type | Privilege  Type | Privilege  Type| Privilege  Type|
// |----------------+-----------------+----------------+----------------|
// |User       R    | User       R    | User       R   | User       R   |
// |User       R    | User       RW   | User       R   | User       R   |
// |User       RW   | User       R    | User       R   | User       R   |
// |User       RW   | User       RW   | User       RW  | User       RW  |
// |User       R    | Supervisor R    | User       R   | Supervisor RW  |
// |User       R    | Supervisor RW   | User       R   | Supervisor RW  |
// |User       RW   | Supervisor R    | User       R   | Supervisor RW  |
// |User       RW   | Supervisor RW   | User       RW  | Supervisor RW  |
// |Supervisor R    | User       R    | User       R   | Supervisor RW  |
// |Supervisor R    | User       RW   | User       R   | Supervisor RW  |
// |Supervisor RW   | User       R    | User       R   | Supervisor RW  |
// |Supervisor RW   | User       RW   | User       RW  | Supervisor RW  |
// |Supervisor R    | Supervisor R    | Supervisor RW  | Supervisor RW  |
// |Supervisor R    | Supervisor RW   | Supervisor RW  | Supervisor RW  |
// |Supervisor RW   | Supervisor R    | Supervisor RW  | Supervisor RW  |
// |Supervisor RW   | Supervisor RW   | Supervisor RW  | Supervisor RW  |
// +----------------+-----------------+----------------+----------------+

// Page Fault Error Code Format:
// =============================
//
// bits 31..4: Reserved
// bit  3: RSVD (Pentium Pro+)
//   0: fault caused by reserved bits set to 1 in a page directory
//      when the PSE or PAE flags in CR4 are set to 1
//   1: fault was not caused by reserved bit violation
// bit  2: U/S (386+)
//   0: fault originated when in supervior mode
//   1: fault originated when in user mode
// bit  1: R/W (386+)
//   0: access causing the fault was a read
//   1: access causing the fault was a write
// bit  0: P (386+)
//   0: fault caused by a nonpresent page
//   1: fault caused by a page level protection violation

// Some paging related notes:
// ==========================
//
// - When the processor is running in supervisor level, all pages are both
//   readable and writable (write-protect ignored).  When running at user
//   level, only pages which belong to the user level are accessible;
//   read/write & read-only are readable, read/write are writable.
//
// - If the Present bit is 0 in either level of page table, an
//   access which uses these entries will generate a page fault.
//
// - (A)ccess bit is used to report read or write access to a page
//   or 2nd level page table.
//
// - (D)irty bit is used to report write access to a page.
//
// - Processor running at CPL=0,1,2 maps to U/S=0
//   Processor running at CPL=3     maps to U/S=1

#if BX_SUPPORT_X86_64
  #define BX_INVALID_TLB_ENTRY BX_CONST64(0xffffffffffffffff)
#else
  #define BX_INVALID_TLB_ENTRY 0xffffffff
#endif

// bit [11] of the TLB lpf used for TLB_NoHostPtr valid indication
#define TLB_LPFOf(laddr) AlignedAccessLPFOf(laddr, 0x7ff)

#if BX_CPU_LEVEL >= 4
#  define BX_PRIV_CHECK_SIZE 32
#else
#  define BX_PRIV_CHECK_SIZE 16
#endif

// The 'priv_check' array is used to decide if the current access
// has the proper paging permissions.  An index is formed, based
// on parameters such as the access type and level, the write protect
// flag and values cached in the TLB.  The format of the index into this
// array is:
//
//   |4 |3 |2 |1 |0 |
//   |wp|us|us|rw|rw|
//    |  |  |  |  |
//    |  |  |  |  +---> r/w of current access
//    |  |  +--+------> u/s,r/w combined of page dir & table (cached)
//    |  +------------> u/s of current access
//    +---------------> Current CR0.WP value

/* 0xff0bbb0b */
static const Bit8u priv_check[BX_PRIV_CHECK_SIZE] =
{
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1,
#if BX_CPU_LEVEL >= 4
  1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1
#endif
};

#define BX_PAGING_PHY_ADDRESS_RESERVED_BITS \
    (BX_PHY_ADDRESS_RESERVED_BITS & BX_CONST64(0xfffffffffffff))

#define PAGE_DIRECTORY_NX_BIT (BX_CONST64(0x8000000000000000))

#define BX_CR3_PAGING_MASK    (BX_CONST64(0x000ffffffffff000))

// Each entry in the TLB cache has 3 entries:
//
//   lpf:         Linear Page Frame (page aligned linear address of page)
//     bits 32..12  Linear page frame
//     bit  11      0: TLB HostPtr access allowed, 1: not allowed
//     bit  10...0  Invalidate index
//
//   ppf:         Physical Page Frame (page aligned phy address of page)
//
//   hostPageAddr:
//                Host Page Frame address used for direct access to
//                the mem.vector[] space allocated for the guest physical
//                memory.  If this is zero, it means that a pointer
//                to the host space could not be generated, likely because
//                that page of memory is not standard memory (it might
//                be memory mapped IO, ROM, etc).
//
//   accessBits:
//
//     bit  31:     Page is a global page.
//
//       The following bits are used for a very efficient permissions
//       check.  The goal is to be able, using only the current privilege
//       level and access type, to determine if the page tables allow the
//       access to occur or at least should rewalk the page tables.  On
//       the first read access, permissions are set to only read, so a
//       rewalk is necessary when a subsequent write fails the tests.
//       This allows for the dirty bit to be set properly, but for the
//       test to be efficient.  Note that the CR0.WP flag is not present.
//       The values in the following flags is based on the current CR0.WP
//       value, necessitating a TLB flush when CR0.WP changes.
//
//       The test bit:
//         OK = 1 << ((E<<2) | (W<<1) | U)
//
//       where E:1=Execute, 0=Data;
//             W:1=Write, 0=Read;
//             U:1=CPL3, 0=CPL0-2
//       
//       Thus for reads, it is:
//         OK = 0x01 << (          U )
//       for writes:
//         OK = 0x04 << (          U )
//       for code fetches:
//         OK = 0x10 << (          U )
//
//     bit 5: Execute from User   privilege is OK
//     bit 4: Execute from System privilege is OK
//     bit 3: Write   from User   privilege is OK
//     bit 2: Write   from System privilege is OK
//     bit 1: Read    from User   privilege is OK
//     bit 0: Read    from System privilege is OK
//
//       Note, that the TLB should have TLB_NoHostPtr bit set in the lpf when
//       direct access through host pointer is NOT allowed for the page.
//       A memory operation asking for a direct access through host pointer
//       will not set TLB_NoHostPtr bit in its lpf and thus get TLB miss 
//       result when the direct access is not allowed.
//

#define TLB_NoHostPtr   (0x800) /* set this bit when direct access is NOT allowed */
#define TLB_GlobalPage  (0x80000000)

#define TLB_SysReadOK     (0x01)
#define TLB_UserReadOK    (0x02)
#define TLB_SysWriteOK    (0x04)
#define TLB_UserWriteOK   (0x08)
#define TLB_SysExecuteOK  (0x10)
#define TLB_UserExecuteOK (0x20)

#include "cpustats.h"

// ==============================================================

void BX_CPU_C::TLB_flush(void)
{
	BX_PANIC(("TLB_flush called"));
}

#if BX_CPU_LEVEL >= 6
void BX_CPU_C::TLB_flushNonGlobal(void)
{
	BX_PANIC(("TLB_flushNonGlobal called"));
}
#endif

void BX_CPU_C::TLB_invlpg(bx_address laddr)
{
	BX_PANIC(("TLB_invlpg called"));
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::INVLPG(bxInstruction_c* i)
{
	BX_PANIC(("INVLPG called"));
}

// error checking order - page not present, reserved bits, protection
#define ERROR_NOT_PRESENT       0x00
#define ERROR_PROTECTION        0x01
#define ERROR_RESERVED          0x08
#define ERROR_CODE_ACCESS       0x10

void BX_CPU_C::page_fault(unsigned fault, bx_address laddr, unsigned user, unsigned rw)
{
	BX_PANIC(("page_fault called"));
}

#define BX_LEVEL_PML4  3
#define BX_LEVEL_PDPTE 2
#define BX_LEVEL_PDE   1
#define BX_LEVEL_PTE   0

static const char *bx_paging_level[4] = { "PTE", "PDE", "PDPE", "PML4" }; // keep it 4 letters

#if BX_CPU_LEVEL >= 6

//                Format of a Long Mode Non-Leaf Entry
// -----------------------------------------------------------
// 00    | Present (P)
// 01    | R/W
// 02    | U/S
// 03    | Page-Level Write-Through (PWT)
// 04    | Page-Level Cache-Disable (PCD)
// 05    | Accessed (A)
// 06    | (ignored)
// 07    | Page Size (PS), must be 0 if no Large Page on the level
// 11-08 | (ignored)
// PA-12 | Physical address of 4-KByte aligned page-directory-pointer table
// 51-PA | Reserved (must be zero)
// 62-52 | (ignored)
// 63    | Execute-Disable (XD) (if EFER.NXE=1, reserved otherwise)
// -----------------------------------------------------------

#define PAGING_PAE_RESERVED_BITS (BX_PAGING_PHY_ADDRESS_RESERVED_BITS)

// in legacy PAE mode bits [62:52] are reserved. bit 63 is NXE
#define PAGING_LEGACY_PAE_RESERVED_BITS \
             (BX_PAGING_PHY_ADDRESS_RESERVED_BITS | BX_CONST64(0x7ff0000000000000))

//       Format of a PDPTE that References a 1-GByte Page
// -----------------------------------------------------------
// 00    | Present (P)
// 01    | R/W
// 02    | U/S
// 03    | Page-Level Write-Through (PWT)
// 04    | Page-Level Cache-Disable (PCD)
// 05    | Accessed (A)
// 06    | (ignored)
// 07    | Page Size, must be 1 to indicate a 1-GByte Page
// 08    | Global (G) (if CR4.PGE=1, ignored otherwise)
// 11-09 | (ignored)
// 12    | PAT (if PAT is supported, reserved otherwise)
// 29-13 | Reserved (must be zero)
// PA-30 | Physical address of the 1-Gbyte Page
// 51-PA | Reserved (must be zero)
// 62-52 | (ignored)
// 63    | Execute-Disable (XD) (if EFER.NXE=1, reserved otherwise)
// -----------------------------------------------------------

#define PAGING_PAE_PDPTE1G_RESERVED_BITS \
    (BX_PAGING_PHY_ADDRESS_RESERVED_BITS | BX_CONST64(0x3FFFE000))

//        Format of a PAE PDE that Maps a 2-MByte Page
// -----------------------------------------------------------
// 00    | Present (P)
// 01    | R/W
// 02    | U/S
// 03    | Page-Level Write-Through (PWT)
// 04    | Page-Level Cache-Disable (PCD)
// 05    | Accessed (A)
// 06    | Dirty (D)
// 07    | Page Size (PS), must be 1 to indicate a 2-MByte Page
// 08    | Global (G) (if CR4.PGE=1, ignored otherwise)
// 11-09 | (ignored)
// 12    | PAT (if PAT is supported, reserved otherwise)
// 20-13 | Reserved (must be zero)
// PA-21 | Physical address of the 2-MByte page
// 51-PA | Reserved (must be zero)
// 62-52 | ignored in long mode, reserved (must be 0) in legacy PAE mode
// 63    | Execute-Disable (XD) (if EFER.NXE=1, reserved otherwise)
// -----------------------------------------------------------

#define PAGING_PAE_PDE2M_RESERVED_BITS \
    (BX_PAGING_PHY_ADDRESS_RESERVED_BITS | BX_CONST64(0x001FE000))

//        Format of a PAE PTE that Maps a 4-KByte Page
// -----------------------------------------------------------
// 00    | Present (P)
// 01    | R/W
// 02    | U/S
// 03    | Page-Level Write-Through (PWT)
// 04    | Page-Level Cache-Disable (PCD)
// 05    | Accessed (A)
// 06    | Dirty (D)
// 07    | PAT (if PAT is supported, reserved otherwise)
// 08    | Global (G) (if CR4.PGE=1, ignored otherwise)
// 11-09 | (ignored)
// PA-12 | Physical address of the 4-KByte page
// 51-PA | Reserved (must be zero)
// 62-52 | ignored in long mode, reserved (must be 0) in legacy PAE mode
// 63    | Execute-Disable (XD) (if EFER.NXE=1, reserved otherwise)
// -----------------------------------------------------------

int BX_CPU_C::check_entry_PAE(const char *s, Bit64u entry, Bit64u reserved, unsigned rw, bx_bool *nx_fault)
{
  BX_PANIC(("check_entry_PAE called"));

  return -1;
}

#if BX_SUPPORT_MEMTYPE
BX_CPP_INLINE Bit32u calculate_pcd_pwt(Bit32u entry)
{
  Bit32u pcd_pwt = (entry >> 3) & 0x3; // PCD, PWT are stored in bits 3 and 4
  return pcd_pwt;
}

// extract PCD, PWT and PAT pat bits from page table entry
BX_CPP_INLINE Bit32u calculate_pat(Bit32u entry, Bit32u lpf_mask)
{
  Bit32u pcd_pwt = calculate_pcd_pwt(entry);
  // PAT is stored in bit 12 for large pages and in bit 7 for small pages
  Bit32u pat = ((lpf_mask < 0x1000) ? (entry >> 7) : (entry >> 12)) & 0x1;
  return pcd_pwt | (pat << 2);
}
#endif

#if BX_SUPPORT_X86_64

// Translate a linear address to a physical address in long mode
bx_phy_address BX_CPU_C::translate_linear_long_mode(bx_address laddr, Bit32u &lpf_mask, unsigned user, unsigned rw)
{
	BX_PANIC(("translate_linear_long_mode called"));
	return -1;
}

#endif

void BX_CPU_C::update_access_dirty_PAE(bx_phy_address *entry_addr, Bit64u *entry, BxMemtype *entry_memtype, unsigned max_level, unsigned leaf, unsigned write)
{
	BX_PANIC(("update_access_dirty_PAE called"));
}

//          Format of Legacy PAE PDPTR entry (PDPTE)
// -----------------------------------------------------------
// 00    | Present (P)
// 02-01 | Reserved (must be zero)
// 03    | Page-Level Write-Through (PWT) (486+), 0=reserved otherwise
// 04    | Page-Level Cache-Disable (PCD) (486+), 0=reserved otherwise
// 08-05 | Reserved (must be zero)
// 11-09 | (ignored)
// PA-12 | Physical address of 4-KByte aligned page directory
// 63-PA | Reserved (must be zero)
// -----------------------------------------------------------

#define PAGING_PAE_PDPTE_RESERVED_BITS \
    (BX_PAGING_PHY_ADDRESS_RESERVED_BITS | BX_CONST64(0xFFF00000000001E6))

bx_bool BX_CPP_AttrRegparmN(1) BX_CPU_C::CheckPDPTR(bx_phy_address cr3_val)
{
	BX_PANIC(("CheckPDPTR called"));
  return 1; /* PDPTRs are fine */
}

#if BX_SUPPORT_VMX >= 2
bx_bool BX_CPP_AttrRegparmN(1) BX_CPU_C::CheckPDPTR(Bit64u *pdptr)
{
  for (unsigned n=0; n<4; n++) {
     if (pdptr[n] & 0x1) {
        if (pdptr[n] & PAGING_PAE_PDPTE_RESERVED_BITS) return 0;
     }
  }

  return 1; /* PDPTRs are fine */
}
#endif

bx_phy_address BX_CPU_C::translate_linear_load_PDPTR(bx_address laddr, unsigned user, unsigned rw)
{
	BX_PANIC(("translate_linear_load_PDPTR called"));
  return -1;
}

// Translate a linear address to a physical address in PAE paging mode
bx_phy_address BX_CPU_C::translate_linear_PAE(bx_address laddr, Bit32u &lpf_mask, unsigned user, unsigned rw)
{
	BX_PANIC((":translate_linear_PAE called"));
  return -1;
}

#endif

//           Format of a PDE that Maps a 4-MByte Page
// -----------------------------------------------------------
// 00    | Present (P)
// 01    | R/W
// 02    | U/S
// 03    | Page-Level Write-Through (PWT)
// 04    | Page-Level Cache-Disable (PCD)
// 05    | Accessed (A)
// 06    | Dirty (D)
// 07    | Page size, must be 1 to indicate 4-Mbyte page
// 08    | Global (G) (if CR4.PGE=1, ignored otherwise)
// 11-09 | (ignored)
// 12    | PAT (if PAT is supported, reserved otherwise)
// PA-13 | Bits PA-32 of physical address of the 4-MByte page
// 21-PA | Reserved (must be zero)
// 31-22 | Bits 31-22 of physical address of the 4-MByte page
// -----------------------------------------------------------

#define PAGING_PDE4M_RESERVED_BITS \
    (((1 << (41-BX_PHY_ADDRESS_WIDTH))-1) << (13 + BX_PHY_ADDRESS_WIDTH - 32))

// Translate a linear address to a physical address in legacy paging mode
bx_phy_address BX_CPU_C::translate_linear_legacy(bx_address laddr, Bit32u &lpf_mask, unsigned user, unsigned rw)
{
	BX_PANIC((":translate_linear_legacy called"));
  return -1;
}

void BX_CPU_C::update_access_dirty(bx_phy_address *entry_addr, Bit32u *entry, BxMemtype *entry_memtype, unsigned leaf, unsigned write)
{
	BX_PANIC(("update_access_dirty called"));
}

// Translate a linear address to a physical address
bx_phy_address BX_CPU_C::translate_linear(bx_TLB_entry *tlbEntry, bx_address laddr, unsigned user, unsigned rw)
{
	BX_PANIC(("translate_lineary called"));

  return -1;
}

const char *get_memtype_name(BxMemtype memtype)
{
	BX_PANIC(("get_memtype_name called"));
	return "";
}

#if BX_SUPPORT_MEMTYPE
BxMemtype BX_CPP_AttrRegparmN(1) BX_CPU_C::memtype_by_mtrr(bx_phy_address pAddr)
{
#if BX_CPU_LEVEL >= 6
  if (is_cpu_extension_supported(BX_ISA_MTRR)) {
    const Bit32u BX_MTRR_DEFTYPE_FIXED_MTRR_ENABLE_MASK = (1 << 10);
    const Bit32u BX_MTRR_ENABLE_MASK = (1 << 11);

    if (BX_CPU_THIS_PTR msr.mtrr_deftype & BX_MTRR_ENABLE_MASK) {
      // fixed range MTRR take priority over variable range MTRR when enabled
      if (pAddr < 0x100000 && (BX_CPU_THIS_PTR msr.mtrr_deftype & BX_MTRR_DEFTYPE_FIXED_MTRR_ENABLE_MASK)) {
        if (pAddr < 0x80000) {
          unsigned index = (pAddr >> 16) & 0x7;
          return (BxMemtype) BX_CPU_THIS_PTR msr.mtrrfix64k.ubyte(index);
        }
        if (pAddr < 0xc0000) {
          unsigned index = ((pAddr - 0x80000) >> 14) & 0xf;
          return (BxMemtype) BX_CPU_THIS_PTR msr.mtrrfix16k[index >> 3].ubyte(index & 0x7);
        }
        else {
          unsigned index =  (pAddr - 0xc0000) >> 12;
          return (BxMemtype) BX_CPU_THIS_PTR msr.mtrrfix4k [index >> 3].ubyte(index & 0x7);
        }
      }

      int memtype = -1;

      for (unsigned i=0; i < BX_NUM_VARIABLE_RANGE_MTRRS; i++) {
        Bit64u base = BX_CPU_THIS_PTR msr.mtrrphys[i*2];
        Bit64u mask = BX_CPU_THIS_PTR msr.mtrrphys[i*2 + 1];
        if ((mask & BX_MTRR_ENABLE_MASK) == 0) continue;
        mask = PPFOf(mask);
        if ((pAddr & mask) == (base & mask)) {
          //
          // Matched variable MTRR, check overlap rules:
          // - if two or more variable memory ranges match and the memory types are identical,
          //   then that memory type is used.
          // - if two or more variable memory ranges match and one of the memory types is UC,
          //   the UC memory type used.
          // - if two or more variable memory ranges match and the memory types are WT and WB,
          //   the WT memory type is used.
          // - For overlaps not defined by the above rules, processor behavior is undefined.
          //
          BxMemtype curr_memtype = BxMemtype(base & 0xff);
          if (curr_memtype == BX_MEMTYPE_UC)
            return BX_MEMTYPE_UC;

          if (memtype == -1) {
            memtype = curr_memtype; // first match
          }
          else if (memtype != (int) curr_memtype) {
            if (curr_memtype == BX_MEMTYPE_WT && memtype == BX_MEMTYPE_WB)
              memtype = BX_MEMTYPE_WT;
            else if (curr_memtype == BX_MEMTYPE_WB && memtype == BX_MEMTYPE_WT)
              memtype = BX_MEMTYPE_WT;
            else
              memtype = BX_MEMTYPE_INVALID;
          }
        }
      }

      if (memtype != -1)
        return BxMemtype(memtype);

      // didn't match any variable range MTRR, return default memory type
      return BxMemtype(BX_CPU_THIS_PTR msr.mtrr_deftype & 0xff);
    }

    // return UC memory type when MTRRs are not enabled
    return BX_MEMTYPE_UC;
  }
#endif

  // return INVALID memory type when MTRRs are not supported
  return BX_MEMTYPE_INVALID;
}

BxMemtype BX_CPP_AttrRegparmN(1) BX_CPU_C::memtype_by_pat(unsigned pat)
{
  return (BxMemtype) BX_CPU_THIS_PTR msr.pat.ubyte(pat);
}

BxMemtype BX_CPP_AttrRegparmN(2) BX_CPU_C::resolve_memtype(BxMemtype mtrr_memtype, BxMemtype pat_memtype)
{
  if (BX_CPU_THIS_PTR cr0.get_CD())
    return BX_MEMTYPE_UC;

  if (mtrr_memtype == BX_MEMTYPE_INVALID) // will result in ignore of MTRR memory type
    mtrr_memtype = BX_MEMTYPE_WB;

  switch(pat_memtype) {
    case BX_MEMTYPE_UC:
    case BX_MEMTYPE_WC:
      return pat_memtype;

    case BX_MEMTYPE_WT:
    case BX_MEMTYPE_WP:
      if (mtrr_memtype == BX_MEMTYPE_WC) return BX_MEMTYPE_UC;
      return (mtrr_memtype < pat_memtype) ? mtrr_memtype : pat_memtype;

    case BX_MEMTYPE_WB:
      return mtrr_memtype;

    case BX_MEMTYPE_UC_WEAK:
      return (mtrr_memtype == BX_MEMTYPE_WC) ? BX_MEMTYPE_WC : BX_MEMTYPE_UC;

    default:
      BX_PANIC(("unexpected PAT memory type: %u", (unsigned) pat_memtype));
  }

  return BX_MEMTYPE_INVALID; // keep compiler happy
}
#endif

#if BX_SUPPORT_SVM

void BX_CPU_C::nested_page_fault(unsigned fault, bx_phy_address guest_paddr, unsigned rw, unsigned is_page_walk)
{
  unsigned isWrite = rw & 1;

  Bit64u error_code = fault | (1 << 2) | (isWrite << 1);
  if (rw == BX_EXECUTE)
    error_code |= ERROR_CODE_ACCESS; // I/D = 1

  if (is_page_walk)
    error_code |= BX_CONST64(1) << 32;
  else
    error_code |= BX_CONST64(1) << 33;

  Svm_Vmexit(SVM_VMEXIT_NPF, error_code, guest_paddr);
}

bx_phy_address BX_CPU_C::nested_walk_long_mode(bx_phy_address guest_paddr, unsigned rw, bx_bool is_page_walk)
{
  bx_phy_address entry_addr[4];
  Bit64u entry[4];
  BxMemtype entry_memtype[4] = { BX_MEMTYPE_INVALID };
  bx_bool nx_fault = 0;
  int leaf;

  SVM_CONTROLS *ctrls = &BX_CPU_THIS_PTR vmcb.ctrls;
  SVM_HOST_STATE *host_state = &BX_CPU_THIS_PTR vmcb.host_state;
  bx_phy_address ppf = ctrls->ncr3 & BX_CR3_PAGING_MASK;
  Bit64u offset_mask = BX_CONST64(0x0000ffffffffffff);
  unsigned combined_access = 0x06;

  Bit64u reserved = PAGING_PAE_RESERVED_BITS;
  if (! host_state->efer.get_NXE())
    reserved |= PAGE_DIRECTORY_NX_BIT;

  for (leaf = BX_LEVEL_PML4;; --leaf) {
    entry_addr[leaf] = ppf + ((guest_paddr >> (9 + 9*leaf)) & 0xff8);
    access_read_physical(entry_addr[leaf], 8, &entry[leaf]);
    BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[leaf], 8, BX_MEMTYPE_INVALID, BX_READ, (BX_PTE_ACCESS + leaf), (Bit8u*)(&entry[leaf]));
    offset_mask >>= 9;

    Bit64u curr_entry = entry[leaf];
    int fault = check_entry_PAE(bx_paging_level[leaf], curr_entry, reserved, rw, &nx_fault);
    if (fault >= 0)
      nested_page_fault(fault, guest_paddr, rw, is_page_walk);

    combined_access &= curr_entry; // U/S and R/W
    ppf = curr_entry & BX_CONST64(0x000ffffffffff000);

    if (leaf == BX_LEVEL_PTE) break;

    if (curr_entry & 0x80) {
      if (leaf > (BX_LEVEL_PDE + !!is_cpu_extension_supported(BX_ISA_1G_PAGES))) {
        BX_DEBUG(("Nested PAE Walk %s: PS bit set !", bx_paging_level[leaf]));
        nested_page_fault(ERROR_RESERVED | ERROR_PROTECTION, guest_paddr, rw, is_page_walk);
      }

      ppf &= BX_CONST64(0x000fffffffffe000);
      if (ppf & offset_mask) {
        BX_DEBUG(("Nested PAE Walk %s: reserved bit is set: 0x" FMT_ADDRX64, bx_paging_level[leaf], curr_entry));
        nested_page_fault(ERROR_RESERVED | ERROR_PROTECTION, guest_paddr, rw, is_page_walk);
      }

      break;
    }
  }

  bx_bool isWrite = (rw & 1); // write or r-m-w

  unsigned priv_index = (1<<3) /* user */ | (combined_access | isWrite);

  if (!priv_check[priv_index] || nx_fault)
    nested_page_fault(ERROR_PROTECTION, guest_paddr, rw, is_page_walk);

  // Update A/D bits if needed
  update_access_dirty_PAE(entry_addr, entry, entry_memtype, BX_LEVEL_PML4, leaf, isWrite);

  // Make up the physical page frame address
  return ppf | (bx_phy_address)(guest_paddr & offset_mask);	
}

bx_phy_address BX_CPU_C::nested_walk_PAE(bx_phy_address guest_paddr, unsigned rw, bx_bool is_page_walk)
{
  bx_phy_address entry_addr[2];
  Bit64u entry[2];
  BxMemtype entry_memtype[2] = { BX_MEMTYPE_INVALID };
  bx_bool nx_fault = 0;
  int leaf;

  unsigned combined_access = 0x06;

  SVM_CONTROLS *ctrls = &BX_CPU_THIS_PTR vmcb.ctrls;
  SVM_HOST_STATE *host_state = &BX_CPU_THIS_PTR vmcb.host_state;
  bx_phy_address ncr3 = ctrls->ncr3 & 0xffffffe0;
  unsigned index = (guest_paddr >> 30) & 0x3;
  Bit64u pdptr;

  bx_phy_address pdpe_entry_addr = (bx_phy_address) (ncr3 | (index << 3));
  access_read_physical(pdpe_entry_addr, 8, &pdptr);
  BX_NOTIFY_PHY_MEMORY_ACCESS(pdpe_entry_addr, 8, BX_MEMTYPE_INVALID, BX_READ, (BX_PDPTR0_ACCESS + index), (Bit8u*) &pdptr);

  if (! (pdptr & 0x1)) {
    BX_DEBUG(("Nested PAE Walk PDPTE%d entry not present !", index));
    nested_page_fault(ERROR_NOT_PRESENT, guest_paddr, rw, is_page_walk);
  }

  if (pdptr & PAGING_PAE_PDPTE_RESERVED_BITS) {
    BX_DEBUG(("Nested PAE Walk PDPTE%d entry reserved bits set: 0x" FMT_ADDRX64, index, pdptr));
    nested_page_fault(ERROR_RESERVED | ERROR_PROTECTION, guest_paddr, rw, is_page_walk);
  }

  Bit64u reserved = PAGING_LEGACY_PAE_RESERVED_BITS;
  if (! host_state->efer.get_NXE())
    reserved |= PAGE_DIRECTORY_NX_BIT;

  bx_phy_address ppf = pdptr & BX_CONST64(0x000ffffffffff000);

  for (leaf = BX_LEVEL_PDE;; --leaf) {
    entry_addr[leaf] = ppf + ((guest_paddr >> (9 + 9*leaf)) & 0xff8);
    access_read_physical(entry_addr[leaf], 8, &entry[leaf]);
    BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[leaf], 8, BX_MEMTYPE_INVALID, BX_READ, (BX_PTE_ACCESS + leaf), (Bit8u*)(&entry[leaf]));

    Bit64u curr_entry = entry[leaf];
    int fault = check_entry_PAE(bx_paging_level[leaf], curr_entry, reserved, rw, &nx_fault);
    if (fault >= 0)
      nested_page_fault(fault, guest_paddr, rw, is_page_walk);

    combined_access &= curr_entry; // U/S and R/W
    ppf = curr_entry & BX_CONST64(0x000ffffffffff000);

    if (leaf == BX_LEVEL_PTE) break;

    // Ignore CR4.PSE in PAE mode
    if (curr_entry & 0x80) {
      if (curr_entry & PAGING_PAE_PDE2M_RESERVED_BITS) {
        BX_DEBUG(("PAE PDE2M: reserved bit is set PDE=0x" FMT_ADDRX64, curr_entry));
        nested_page_fault(ERROR_RESERVED | ERROR_PROTECTION, guest_paddr, rw, is_page_walk);
      }

      // Make up the physical page frame address
      ppf = (bx_phy_address)((curr_entry & BX_CONST64(0x000fffffffe00000)) | (guest_paddr & 0x001ff000));
      break;
    }
  }

  bx_bool isWrite = (rw & 1); // write or r-m-w

  unsigned priv_index = (1<<3) /* user */ | (combined_access | isWrite);

  if (!priv_check[priv_index] || nx_fault)
    nested_page_fault(ERROR_PROTECTION, guest_paddr, rw, is_page_walk);

  // Update A/D bits if needed
  update_access_dirty_PAE(entry_addr, entry, entry_memtype, BX_LEVEL_PDE, leaf, isWrite);

  Bit32u page_offset = PAGE_OFFSET(guest_paddr);
  return ppf | page_offset;
}

bx_phy_address BX_CPU_C::nested_walk_legacy(bx_phy_address guest_paddr, unsigned rw, bx_bool is_page_walk)
{
  bx_phy_address entry_addr[2];
  Bit32u entry[2];
  BxMemtype entry_memtype[2] = { BX_MEMTYPE_INVALID };
  int leaf;

  SVM_CONTROLS *ctrls = &BX_CPU_THIS_PTR vmcb.ctrls;
  SVM_HOST_STATE *host_state = &BX_CPU_THIS_PTR vmcb.host_state;
  bx_phy_address ppf = ctrls->ncr3 & BX_CR3_PAGING_MASK;
  unsigned combined_access = 0x06;

  for (leaf = BX_LEVEL_PDE;; --leaf) {
    entry_addr[leaf] = ppf + ((guest_paddr >> (10 + 10*leaf)) & 0xffc);
    access_read_physical(entry_addr[leaf], 4, &entry[leaf]);
    BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[leaf], 4, BX_MEMTYPE_INVALID, BX_READ, (BX_PTE_ACCESS + leaf), (Bit8u*)(&entry[leaf]));

    Bit32u curr_entry = entry[leaf];
    if (!(curr_entry & 0x1)) {
      BX_DEBUG(("Nested %s Walk: entry not present", bx_paging_level[leaf]));
      nested_page_fault(ERROR_NOT_PRESENT, guest_paddr, rw, is_page_walk);
    }

    combined_access &= curr_entry; // U/S and R/W
    ppf = curr_entry & 0xfffff000;

    if (leaf == BX_LEVEL_PTE) break;

    if ((curr_entry & 0x80) != 0 && host_state->cr4.get_PSE()) {
      // 4M paging, only if CR4.PSE enabled, ignore PDE.PS otherwise
      if (curr_entry & PAGING_PDE4M_RESERVED_BITS) {
        BX_DEBUG(("Nested PSE Walk PDE4M: reserved bit is set: PDE=0x%08x", entry[BX_LEVEL_PDE]));
        nested_page_fault(ERROR_RESERVED | ERROR_PROTECTION, guest_paddr, rw, is_page_walk);
      }

      // make up the physical frame number
      ppf = (curr_entry & 0xffc00000) | (guest_paddr & 0x003ff000);
#if BX_PHY_ADDRESS_WIDTH > 32
      ppf |= ((bx_phy_address)(curr_entry & 0x003fe000)) << 19;
#endif
      break;
    }
  }

  bx_bool isWrite = (rw & 1); // write or r-m-w

  unsigned priv_index = (1<<3) /* user */ | (combined_access | isWrite);

  if (!priv_check[priv_index])
    nested_page_fault(ERROR_PROTECTION, guest_paddr, rw, is_page_walk);

  update_access_dirty(entry_addr, entry, entry_memtype, leaf, isWrite);

  Bit32u page_offset = PAGE_OFFSET(guest_paddr);
  return ppf | page_offset;
}

bx_phy_address BX_CPU_C::nested_walk(bx_phy_address guest_paddr, unsigned rw, bx_bool is_page_walk)
{
  SVM_HOST_STATE *host_state = &BX_CPU_THIS_PTR vmcb.host_state;

  BX_DEBUG(("Nested walk for guest paddr 0x" FMT_PHY_ADDRX, guest_paddr));

  if (host_state->efer.get_LMA())
    return nested_walk_long_mode(guest_paddr, rw, is_page_walk);
  else if (host_state->cr4.get_PAE())
    return nested_walk_PAE(guest_paddr, rw, is_page_walk);
  else
    return nested_walk_legacy(guest_paddr, rw, is_page_walk);
}

#endif

#if BX_SUPPORT_VMX >= 2

/* EPT access type */
#define BX_EPT_READ    0x01
#define BX_EPT_WRITE   0x02
#define BX_EPT_EXECUTE 0x04

/* EPT access mask */
#define BX_EPT_ENTRY_NOT_PRESENT        0x00
#define BX_EPT_ENTRY_READ_ONLY          0x01
#define BX_EPT_ENTRY_WRITE_ONLY         0x02
#define BX_EPT_ENTRY_READ_WRITE         0x03
#define BX_EPT_ENTRY_EXECUTE_ONLY       0x04
#define BX_EPT_ENTRY_READ_EXECUTE       0x05
#define BX_EPT_ENTRY_WRITE_EXECUTE      0x06
#define BX_EPT_ENTRY_READ_WRITE_EXECUTE 0x07

#define BX_SUPPRESS_EPT_VIOLATION_EXCEPTION BX_CONST64(0x8000000000000000)

#define BX_VMX_EPT_ACCESS_DIRTY_ENABLED (BX_CPU_THIS_PTR vmcs.eptptr & 0x40)

//                   Format of a EPT Entry
// -----------------------------------------------------------
// 00    | Read access
// 01    | Write access
// 02    | Execute Access
// 05-03 | EPT Memory type (for leaf entries, reserved otherwise)
// 06    | Ignore PAT memory type (for leaf entries, reserved otherwise)
// 07    | Page Size, must be 1 to indicate a Large Page
// 08    | Accessed bit (if supported, ignored otherwise)
// 09    | Dirty bit (for leaf entries, if supported, ignored otherwise)
// 11-10 | (ignored)
// PA-12 | Physical address
// 51-PA | Reserved (must be zero)
// 63-52 | (ignored)
// -----------------------------------------------------------

#define PAGING_EPT_RESERVED_BITS (BX_PAGING_PHY_ADDRESS_RESERVED_BITS)

bx_phy_address BX_CPU_C::translate_guest_physical(bx_phy_address guest_paddr, bx_address guest_laddr, bx_bool guest_laddr_valid, bx_bool is_page_walk, unsigned rw)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;
  bx_phy_address entry_addr[4], ppf = LPFOf(vm->eptptr);
  Bit64u entry[4];
  int leaf;

#if BX_SUPPORT_MEMTYPE
  // The MTRRs have no effect on the memory type used for an access to an EPT paging structures.
  BxMemtype eptptr_memtype = BX_CPU_THIS_PTR cr0.get_CD() ? (BX_MEMTYPE_UC) : BxMemtype(vm->eptptr & 0x7);
#endif

  Bit32u combined_access = 0x7, access_mask = 0;
  Bit64u offset_mask = BX_CONST64(0x0000ffffffffffff);

  BX_DEBUG(("EPT walk for guest paddr 0x" FMT_PHY_ADDRX, guest_paddr));

  // when EPT A/D enabled treat guest page table accesses as writes
  if (BX_VMX_EPT_ACCESS_DIRTY_ENABLED && is_page_walk && guest_laddr_valid)
    rw = BX_WRITE;

  if (rw == BX_EXECUTE) access_mask |= BX_EPT_EXECUTE;
  if (rw & 1) access_mask |= BX_EPT_WRITE; // write or r-m-w
  if (rw == BX_READ) access_mask |= BX_EPT_READ;

  Bit32u vmexit_reason = 0;

  for (leaf = BX_LEVEL_PML4;; --leaf) {
    entry_addr[leaf] = ppf + ((guest_paddr >> (9 + 9*leaf)) & 0xff8);
    access_read_physical(entry_addr[leaf], 8, &entry[leaf]);
    BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[leaf], 8, MEMTYPE(eptptr_memtype), BX_READ, (BX_EPT_PTE_ACCESS + leaf), (Bit8u*)(&entry[leaf]));

    offset_mask >>= 9;
    Bit64u curr_entry = entry[leaf];
    Bit32u curr_access_mask = curr_entry & 0x7;

    combined_access &= curr_access_mask;

    if (curr_access_mask == BX_EPT_ENTRY_NOT_PRESENT) {
      BX_DEBUG(("EPT %s: not present", bx_paging_level[leaf]));
      vmexit_reason = VMX_VMEXIT_EPT_VIOLATION;
      break;
    }

    if (curr_access_mask == BX_EPT_ENTRY_WRITE_ONLY || curr_access_mask == BX_EPT_ENTRY_WRITE_EXECUTE) {
      BX_DEBUG(("EPT %s: EPT misconfiguration mask=%d", bx_paging_level[leaf], curr_access_mask));
      vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
      break;
    }

    extern bx_bool isMemTypeValidMTRR(unsigned memtype);
    if (! isMemTypeValidMTRR((curr_entry >> 3) & 7)) {
      BX_DEBUG(("EPT %s: EPT misconfiguration memtype=%d",
        bx_paging_level[leaf], (unsigned)((curr_entry >> 3) & 7)));
      vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
      break;
    }

    if (curr_entry & PAGING_EPT_RESERVED_BITS) {
      BX_DEBUG(("EPT %s: reserved bit is set 0x" FMT_ADDRX64, bx_paging_level[leaf], curr_entry));
      vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
      break;
    }

    ppf = curr_entry & BX_CONST64(0x000ffffffffff000);

    if (leaf == BX_LEVEL_PTE) break;

    if (curr_entry & 0x80) {
      if (leaf > (BX_LEVEL_PDE + !!is_cpu_extension_supported(BX_ISA_1G_PAGES))) {
        BX_DEBUG(("EPT %s: PS bit set !", bx_paging_level[leaf]));
        vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
        break;
      }

      ppf &= BX_CONST64(0x000fffffffffe000);
      if (ppf & offset_mask) {
         BX_DEBUG(("EPT %s: reserved bit is set: 0x" FMT_ADDRX64, bx_paging_level[leaf], curr_entry));
         vmexit_reason = VMX_VMEXIT_EPT_MISCONFIGURATION;
         break;
      }

      // Make up the physical page frame address
      ppf += (bx_phy_address)(guest_paddr & offset_mask);
      break;
    }
  }

  if (!vmexit_reason && (access_mask & combined_access) != access_mask) {
    vmexit_reason = VMX_VMEXIT_EPT_VIOLATION;
  }

  if (vmexit_reason) {
    BX_ERROR(("VMEXIT: EPT %s for guest paddr 0x" FMT_PHY_ADDRX " laddr 0x" FMT_ADDRX,
       (vmexit_reason == VMX_VMEXIT_EPT_VIOLATION) ? "violation" : "misconfig", guest_paddr, guest_laddr));

    Bit32u vmexit_qualification = 0;

    if (vmexit_reason == VMX_VMEXIT_EPT_VIOLATION) {
      // no VMExit qualification for EPT Misconfiguration VMExit
      vmexit_qualification = access_mask | (combined_access << 3);
      if (guest_laddr_valid) {
        vmexit_qualification |= (1<<7);
        if (! is_page_walk) vmexit_qualification |= (1<<8);
      }
      if (BX_CPU_THIS_PTR nmi_unblocking_iret)
        vmexit_qualification |= (1 << 12);

      if (SECONDARY_VMEXEC_CONTROL(VMX_VM_EXEC_CTRL3_EPT_VIOLATION_EXCEPTION)) {
        if ((entry[leaf] & BX_SUPPRESS_EPT_VIOLATION_EXCEPTION) == 0)
          Virtualization_Exception(vmexit_qualification, guest_paddr, guest_laddr);
      }
    }

    VMwrite64(VMCS_64BIT_GUEST_PHYSICAL_ADDR, guest_paddr);
    VMwrite_natural(VMCS_GUEST_LINEAR_ADDR, guest_laddr);
    VMexit(vmexit_reason, vmexit_qualification);
  }

  if (BX_VMX_EPT_ACCESS_DIRTY_ENABLED) {
    update_ept_access_dirty(entry_addr, entry, MEMTYPE(eptptr_memtype), leaf, rw & 1);
  }

  Bit32u page_offset = PAGE_OFFSET(guest_paddr);
  return ppf | page_offset;
}

// Access bit 8, Dirty bit 9
void BX_CPU_C::update_ept_access_dirty(bx_phy_address *entry_addr, Bit64u *entry, BxMemtype eptptr_memtype, unsigned leaf, unsigned write)
{
  // Update A bit if needed
  for (unsigned level=BX_LEVEL_PML4; level > leaf; level--) {
    if (!(entry[level] & 0x100)) {
      entry[level] |= 0x100;
      access_write_physical(entry_addr[level], 8, &entry[level]);
      BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[level], 8, MEMTYPE(eptptr_memtype), BX_WRITE, (BX_EPT_PTE_ACCESS + level), (Bit8u*)(&entry[level]));
    }
  }

  // Update A/D bits if needed
  if (!(entry[leaf] & 0x100) || (write && !(entry[leaf] & 0x200))) {
    entry[leaf] |= (0x100 | (write<<9)); // Update A and possibly D bits
    access_write_physical(entry_addr[leaf], 8, &entry[leaf]);
    BX_NOTIFY_PHY_MEMORY_ACCESS(entry_addr[leaf], 8, MEMTYPE(eptptr_memtype), BX_WRITE, (BX_EPT_PTE_ACCESS + leaf), (Bit8u*)(&entry[leaf]));
  }
}

#endif

#if BX_DEBUGGER || BX_DISASM || BX_INSTRUMENTATION || BX_GDBSTUB

#if BX_DEBUGGER

void dbg_print_paging_pte(int level, Bit64u entry)
{
  dbg_printf("%4s: 0x%08x%08x", bx_paging_level[level], GET32H(entry), GET32L(entry));

  if (entry & BX_CONST64(0x8000000000000000))
    dbg_printf(" XD");
  else
    dbg_printf("   ");

  if (level == BX_LEVEL_PTE) {
    dbg_printf("    %s %s %s",
      (entry & 0x0100) ? "G" : "g",
      (entry & 0x0080) ? "PAT" : "pat",
      (entry & 0x0040) ? "D" : "d");
  }
  else {
    if (entry & 0x80) {
      dbg_printf(" PS %s %s %s",
        (entry & 0x0100) ? "G" : "g",
        (entry & 0x1000) ? "PAT" : "pat",
        (entry & 0x0040) ? "D" : "d");
    }
    else {
      dbg_printf(" ps        ");
    }
  }

  dbg_printf(" %s %s %s %s %s %s\n",
    (entry & 0x20) ? "A" : "a",
    (entry & 0x10) ? "PCD" : "pcd",
    (entry & 0x08) ? "PWT" : "pwt",
    (entry & 0x04) ? "U" : "S",
    (entry & 0x02) ? "W" : "R",
    (entry & 0x01) ? "P" : "p");
}

#if BX_SUPPORT_VMX >= 2
void dbg_print_ept_paging_pte(int level, Bit64u entry)
{
  dbg_printf("EPT %4s: 0x%08x%08x", bx_paging_level[level], GET32H(entry), GET32L(entry));

  if (level != BX_LEVEL_PTE && (entry & 0x80))
    dbg_printf(" PS");
  else
    dbg_printf("   ");

  dbg_printf(" %s %s %s",
    (entry & 0x04) ? "E" : "e",
    (entry & 0x02) ? "W" : "w",
    (entry & 0x01) ? "R" : "r");

  if (level == BX_LEVEL_PTE || (entry & 0x80)) {
    dbg_printf(" %s %s\n",
      (entry & 0x40) ? "IGNORE_PAT" : "ignore_pat",
      get_memtype_name(BxMemtype((entry >> 3) & 0x7)));
  }
  else {
    dbg_printf("\n");
  }
}
#endif

#endif // BX_DEBUGGER

#if BX_SUPPORT_VMX >= 2
bx_bool BX_CPU_C::dbg_translate_guest_physical(bx_phy_address guest_paddr, bx_phy_address *phy, bx_bool verbose)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;
  bx_phy_address pt_address = LPFOf(vm->eptptr);
  Bit64u offset_mask = BX_CONST64(0x0000ffffffffffff);

  for (int level = 3; level >= 0; --level) {
    Bit64u pte;
    pt_address += ((guest_paddr >> (9 + 9*level)) & 0xff8);
    offset_mask >>= 9;
    BX_MEM(0)->readPhysicalPage(BX_CPU_THIS, pt_address, 8, &pte);
#if BX_DEBUGGER
    if (verbose)
      dbg_print_ept_paging_pte(level, pte);
#endif
    switch(pte & 7) {
    case BX_EPT_ENTRY_NOT_PRESENT:
    case BX_EPT_ENTRY_WRITE_ONLY:
    case BX_EPT_ENTRY_WRITE_EXECUTE:
      return 0;
    }
    if (pte & BX_PAGING_PHY_ADDRESS_RESERVED_BITS)
      return 0;

    pt_address = bx_phy_address(pte & BX_CONST64(0x000ffffffffff000));

    if (level == BX_LEVEL_PTE) break;

    if (pte & 0x80) {
       if (level > (BX_LEVEL_PDE + !!is_cpu_extension_supported(BX_ISA_1G_PAGES)))
         return 0;

        pt_address &= BX_CONST64(0x000fffffffffe000);
        if (pt_address & offset_mask) return 0;
        break;
      }
  }

  *phy = pt_address + (bx_phy_address)(guest_paddr & offset_mask);
  return 1;
}
#endif

bx_bool BX_CPU_C::dbg_xlate_linear2phy(bx_address laddr, bx_phy_address *phy, bx_bool verbose)
{
	BX_PANIC(("dbg_xlate_linear2phy called"));
  return 1;
}
#endif

int BX_CPU_C::access_write_linear(bx_address laddr, unsigned len, unsigned curr_pl, Bit32u ac_mask, void *data)
{
	BX_PANIC((":access_write_linear called"));
  return 0;
}

int BX_CPU_C::access_read_linear(bx_address laddr, unsigned len, unsigned curr_pl, unsigned xlate_rw, Bit32u ac_mask, void *data)
{
	BX_PANIC(("access_read_linear called"));
  return 0;
}

void BX_CPU_C::access_write_physical(bx_phy_address paddr, unsigned len, void *data)
{
	BX_PANIC(("access_write_physical called"));
}

void BX_CPU_C::access_read_physical(bx_phy_address paddr, unsigned len, void *data)
{
	BX_PANIC(("access_read_physical called"));
}

bx_hostpageaddr_t BX_CPU_C::getHostMemAddr(bx_phy_address paddr, unsigned rw)
{
	BX_PANIC(("getHostMemAddr called"));
    return 0;
}

#if BX_LARGE_RAMFILE
bx_bool BX_CPU_C::check_addr_in_tlb_buffers(const Bit8u *addr, const Bit8u *end)
{
   BX_PANIC(("check_addr_in_tlb_buffers called"));
  return false;
}
#endif
