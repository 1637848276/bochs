/////////////////////////////////////////////////////////////////////////
// $Id: exception.cc 12570 2014-12-18 19:45:03Z sshwarts $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001-2013  The Bochs Project
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

#include "param_names.h"

#if BX_SUPPORT_X86_64==0
// Make life easier merging cpu64 & cpu code.
#define RIP EIP
#define RSP ESP
#endif

#if BX_SUPPORT_X86_64
void BX_CPU_C::long_mode_int(Bit8u vector, unsigned soft_int, bx_bool push_error, Bit16u error_code)
{
	BX_PANIC(("long_mode_int called"));
}
#endif

void BX_CPU_C::protected_mode_int(Bit8u vector, unsigned soft_int, bx_bool push_error, Bit16u error_code)
{

	BX_PANIC(("protected_mode_int called"));
}

void BX_CPU_C::real_mode_int(Bit8u vector, bx_bool push_error, Bit16u error_code)
{
	BX_PANIC(("real_mode_int called"));
}

void BX_CPU_C::interrupt(Bit8u vector, unsigned type, bx_bool push_error, Bit16u error_code)
{
	BX_PANIC(("interrupt_mode_int called"));
}

/* Exception classes.  These are used as indexes into the 'is_exception_OK'
 * array below, and are stored in the 'exception' array also
 */
#define BX_ET_BENIGN       0
#define BX_ET_CONTRIBUTORY 1
#define BX_ET_PAGE_FAULT   2

#define BX_ET_DOUBLE_FAULT 10

static const bx_bool is_exception_OK[3][3] = {
    { 1, 1, 1 }, /* 1st exception is BENIGN */
    { 1, 0, 1 }, /* 1st exception is CONTRIBUTORY */
    { 1, 0, 0 }  /* 1st exception is PAGE_FAULT */
};

#define BX_EXCEPTION_CLASS_TRAP  0
#define BX_EXCEPTION_CLASS_FAULT 1
#define BX_EXCEPTION_CLASS_ABORT 2

struct BxExceptionInfo exceptions_info[BX_CPU_HANDLED_EXCEPTIONS] = {
  /* DE */ { BX_ET_CONTRIBUTORY, BX_EXCEPTION_CLASS_FAULT, 0 },
  /* DB */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 02 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 }, // NMI
  /* BP */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_TRAP,  0 },
  /* OF */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_TRAP,  0 },
  /* BR */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* UD */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* NM */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* DF */ { BX_ET_DOUBLE_FAULT, BX_EXCEPTION_CLASS_FAULT, 1 },
             // coprocessor segment overrun (286,386 only)
  /* 09 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* TS */ { BX_ET_CONTRIBUTORY, BX_EXCEPTION_CLASS_FAULT, 1 },
  /* NP */ { BX_ET_CONTRIBUTORY, BX_EXCEPTION_CLASS_FAULT, 1 },
  /* SS */ { BX_ET_CONTRIBUTORY, BX_EXCEPTION_CLASS_FAULT, 1 },
  /* GP */ { BX_ET_CONTRIBUTORY, BX_EXCEPTION_CLASS_FAULT, 1 },
  /* PF */ { BX_ET_PAGE_FAULT,   BX_EXCEPTION_CLASS_FAULT, 1 },
  /* 15 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 }, // reserved
  /* MF */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* AC */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 1 },
  /* MC */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_ABORT, 0 },
  /* XM */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* VE */ { BX_ET_PAGE_FAULT,   BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 21 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 22 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 23 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 24 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 25 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 26 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 27 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 28 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 29 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 },
  /* 30 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 }, // FIXME: SVM #SF
  /* 31 */ { BX_ET_BENIGN,       BX_EXCEPTION_CLASS_FAULT, 0 }
};

// vector:     0..255: vector in IDT
// error_code: if exception generates and error, push this error code
void BX_CPU_C::exception(unsigned vector, Bit16u error_code)
{
	BX_PANIC(("exception(%u): not implemented", vector));
}
