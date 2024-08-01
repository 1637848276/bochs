/////////////////////////////////////////////////////////////////////////
// $Id: siminterface.h 12698 2015-03-29 14:27:32Z vruppert $
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////

// base value for generated new parameter id
#define BXP_NEW_PARAM_ID 1001

enum {
#define bx_define_cpudb(model) bx_cpudb_##model,
#include "cpudb.h"
  bx_cpudb_model_last
};
#undef bx_define_cpudb

#if BX_SUPPORT_SMP
  #define BX_SMP_PROCESSORS (bx_cpu_count)
#else
  #define BX_SMP_PROCESSORS 1
#endif


#include "paramtree.h"

enum {
	BX_CPUID_SUPPORT_NOSSE,
	BX_CPUID_SUPPORT_SSE,
	BX_CPUID_SUPPORT_SSE2,
	BX_CPUID_SUPPORT_SSE3,
	BX_CPUID_SUPPORT_SSSE3,
	BX_CPUID_SUPPORT_SSE4_1,
	BX_CPUID_SUPPORT_SSE4_2,
#if BX_SUPPORT_AVX
	BX_CPUID_SUPPORT_AVX,
	BX_CPUID_SUPPORT_AVX2,
#if BX_SUPPORT_EVEX
	BX_CPUID_SUPPORT_AVX512
#endif
#endif
};

enum {
	BX_CPUID_SUPPORT_LEGACY_APIC,
	BX_CPUID_SUPPORT_XAPIC,
#if BX_CPU_LEVEL >= 6
	BX_CPUID_SUPPORT_XAPIC_EXT,
	BX_CPUID_SUPPORT_X2APIC
#endif
};

class BOCHSAPI bx_simulator_interface_c {
public:
  bx_simulator_interface_c() {}
  virtual ~bx_simulator_interface_c() {}

  virtual int get_init_done() { return 0; }
  virtual int set_init_done(int n) {return 0;}
  virtual void reset_all_param() {}
  // new param methods
  virtual bx_param_c *get_param(const char *pname, bx_param_c *base=NULL) {return NULL;}
  virtual bx_param_num_c *get_param_num(const char *pname, bx_param_c *base=NULL) {return NULL;}
  virtual bx_param_string_c *get_param_string(const char *pname, bx_param_c *base=NULL) {return NULL;}
  virtual bx_param_bool_c *get_param_bool(const char *pname, bx_param_c *base=NULL) {return NULL;}
  virtual bx_param_enum_c *get_param_enum(const char *pname, bx_param_c *base=NULL) {return NULL;}
  virtual unsigned gen_param_id() {return 0;}

  virtual bx_list_c *get_bochs_root() { return NULL; }

};

BOCHSAPI extern bx_simulator_interface_c *SIM;

extern void bx_init_siminterface();

#if defined(__WXMSW__) || defined(WIN32)
// Just to provide HINSTANCE, etc. in files that have not included bochs.h.
// I don't like this at all, but I don't see a way around it.
#include <windows.h>
#endif

