/////////////////////////////////////////////////////////////////////////
// $Id: main.cc 12628 2015-02-01 11:46:55Z vruppert $
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#include <QHash>

#include "bochs.h"
#include "bxversion.h"
#include "param_names.h"
#include "cpu.h"
#include "common.h"
#include "dmsg.h"


void bx_init_options(void);


Bit8u bx_cpu_count;

#if BX_SUPPORT_APIC
Bit32u apic_id_mask; // determinted by XAPIC option
bx_bool simulate_xapic;
#endif

bx_pc_system_c bx_pc_system;
typedef BX_CPU_C *BX_CPU_C_PTR;
BOCHSAPI BX_CPU_C bx_cpu;


int InitSimulator()
{

  bx_init_siminterface();
  bx_init_options();
  bx_cpu_count = SIM->get_param_num(BXPN_CPU_NPROCESSORS)->get() *
      SIM->get_param_num(BXPN_CPU_NCORES)->get() *
      SIM->get_param_num(BXPN_CPU_NTHREADS)->get();

  bx_cpu.initialize();
  bx_cpu.reset( BX_RESET_HARDWARE, 0, 0, 0);

  MSG((0, OUTPUT_BOTH, "bx_cpu_count: %d", bx_cpu_count));

  SIM->set_init_done(1);
  return 0;
}
#if 0
void bx_panic(const char*format, ...)
{
    va_list ap;
    va_start(ap, format);
    dmsg0(format, ap);
    va_end(ap);
    dmsg("\n");

    bx_cpu.StopExec();

}

void bx_debug(const char*format, ...)
{
    va_list ap;
    va_start(ap, format);
    dmsg0(format, ap);
    va_end(ap);
     dmsg("\n");

}

void bx_error(const char*format, ...)
{
    va_list ap;
    va_start(ap, format);
    dmsg0(format, ap);
    va_end(ap);
     dmsg("\n");
}

void bx_info(const char*format, ...)
{
    va_list ap;
    va_start(ap, format);
    dmsg0(format, ap);
    va_end(ap);
    dmsg("\n");
}
#endif
