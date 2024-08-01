/////////////////////////////////////////////////////////////////////////
// $Id: config.cc 12681 2015-03-06 22:54:30Z vruppert $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002-2015  The Bochs Project
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

#include "bochs.h"
#include "bxversion.h"
#include "param_names.h"
#include <assert.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif


#define LOG_THIS genlog->

extern bx_debug_t bx_dbg;


void bx_init_options()
{
  int i;
  //bx_list_c *menu;


  bx_param_c *root_param = SIM->get_param(".");


#if BX_SUPPORT_SMP
  #define BX_CPU_PROCESSORS_LIMIT 255
  #define BX_CPU_CORES_LIMIT 8
  #define BX_CPU_HT_THREADS_LIMIT 4
#else
  #define BX_CPU_PROCESSORS_LIMIT 1
  #define BX_CPU_CORES_LIMIT 1
  #define BX_CPU_HT_THREADS_LIMIT 1
#endif

  // cpu subtree
  bx_list_c *cpu_param = new bx_list_c(root_param, "cpu", "CPU Options");

  static const char *cpu_names[] = {
#define bx_define_cpudb(model) #model,
#include "cpudb.h"
    NULL
  };
#undef bx_define_cpudb

  new bx_param_enum_c(cpu_param,
      "model", "CPU configuration",
      "Choose pre-defined CPU configuration",
      cpu_names, 0, 0);

  // cpu options
  bx_param_num_c *nprocessors = new bx_param_num_c(cpu_param,
      "n_processors", "Number of processors in SMP mode",
      "Sets the number of processors for multiprocessor emulation",
      1, BX_CPU_PROCESSORS_LIMIT,
      1);
  nprocessors->set_enabled(BX_CPU_PROCESSORS_LIMIT > 1);
  nprocessors->set_options(bx_param_c::CI_ONLY);
  bx_param_num_c *ncores = new bx_param_num_c(cpu_param,
      "n_cores", "Number of cores in each processor in SMP mode",
      "Sets the number of cores per processor for multiprocessor emulation",
      1, BX_CPU_CORES_LIMIT,
      1);
  ncores->set_enabled(BX_CPU_CORES_LIMIT > 1);
  ncores->set_options(bx_param_c::CI_ONLY);
  bx_param_num_c *nthreads = new bx_param_num_c(cpu_param,
      "n_threads", "Number of HT threads per each core in SMP mode",
      "Sets the number of HT (Intel(R) HyperThreading Technology) threads per core for multiprocessor emulation",
      1, BX_CPU_HT_THREADS_LIMIT,
      1);
  nthreads->set_enabled(BX_CPU_HT_THREADS_LIMIT > 1);
  nthreads->set_options(bx_param_c::CI_ONLY);
  new bx_param_num_c(cpu_param,
      "ips", "Emulated instructions per second (IPS)",
      "Emulated instructions per second, used to calibrate bochs emulated time with wall clock time.",
      BX_MIN_IPS, BX_MAX_BIT32U,
      4000000);
#if BX_SUPPORT_SMP
  new bx_param_num_c(cpu_param,
      "quantum", "Quantum ticks in SMP simulation",
      "Maximum amount of instructions allowed to execute before returning control to another CPU.",
      BX_SMP_QUANTUM_MIN, BX_SMP_QUANTUM_MAX,
      16);
#endif
  new bx_param_bool_c(cpu_param,
      "reset_on_triple_fault", "Enable CPU reset on triple fault",
      "Enable CPU reset if triple fault occured (highly recommended)",
      1);
#if BX_CPU_LEVEL >= 5
  new bx_param_bool_c(cpu_param,
      "ignore_bad_msrs", "Ignore RDMSR / WRMSR to unknown MSR register",
      "Ignore RDMSR/WRMSR to unknown MSR register",
      1);
#endif
  new bx_param_bool_c(cpu_param,
      "cpuid_limit_winnt", "Limit max CPUID function to 3",
      "Limit max CPUID function reported to 3 to workaround WinNT issue",
      0);
#if BX_SUPPORT_MONITOR_MWAIT
  new bx_param_bool_c(cpu_param,
      "mwait_is_nop", "Don't put CPU to sleep state by MWAIT",
      "Don't put CPU to sleep state by MWAIT",
      0);
#endif
#if BX_CONFIGURE_MSRS
  new bx_param_filename_c(cpu_param,
      "msrs",
      "Configurable MSR definition file",
      "Set path to the configurable MSR definition file",
      "", BX_PATHNAME_LEN);
#endif

//  cpu_param->set_options(menu->SHOW_PARENT);

  // cpuid subtree
#if BX_CPU_LEVEL >= 4
  bx_list_c *cpuid_param = new bx_list_c(root_param, "cpuid", "CPUID Options");

  new bx_param_num_c(cpuid_param,
      "level", "CPU Level",
      "CPU level",
     (BX_CPU_LEVEL < 5) ? BX_CPU_LEVEL : 5, BX_CPU_LEVEL,
      BX_CPU_LEVEL);

  new bx_param_num_c(cpuid_param,
      "stepping", "Stepping ID",
      "Processor 4-bits stepping ID",
      0, 15,
      3);

  new bx_param_num_c(cpuid_param,
      "model", "Model ID",
      "Processor model ID, extended model ID",
      0, 255,
      3);

  new bx_param_num_c(cpuid_param,
      "family", "Family ID",
      "Processor family ID, extended family ID",
      BX_CPU_LEVEL, (BX_CPU_LEVEL >= 6) ? 4095 : BX_CPU_LEVEL,
      BX_CPU_LEVEL);

  new bx_param_string_c(cpuid_param,
      "vendor_string",
      "CPUID vendor string",
      "Set the CPUID vendor string",
#if BX_CPU_VENDOR_INTEL
      "GenuineIntel", 
#else
      "AuthenticAMD", 
#endif
      BX_CPUID_VENDOR_LEN+1);
  new bx_param_string_c(cpuid_param,
      "brand_string",
      "CPUID brand string",
      "Set the CPUID brand string",
#if BX_CPU_VENDOR_INTEL
      "              Intel(R) Pentium(R) 4 CPU        ", 
#else
      "AMD Athlon(tm) processor",
#endif
      BX_CPUID_BRAND_LEN+1);

#if BX_CPU_LEVEL >= 5
  new bx_param_bool_c(cpuid_param,
      "mmx", "Support for MMX instruction set",
      "Support for MMX instruction set",
      1);

  // configure defaults to XAPIC enabled
  static const char *apic_names[] = {
    "legacy",
    "xapic",
#if BX_CPU_LEVEL >= 6
    "xapic_ext",
    "x2apic",
#endif
    NULL
  };

  new bx_param_enum_c(cpuid_param,
      "apic", "APIC configuration",
      "Select APIC configuration (Legacy APIC/XAPIC/XAPIC_EXT/X2APIC)",
      apic_names,
      BX_CPUID_SUPPORT_XAPIC,
      BX_CPUID_SUPPORT_LEGACY_APIC);
#endif

#if BX_CPU_LEVEL >= 6
  // configure defaults to CPU_LEVEL = 6 with SSE2 enabled
  static const char *simd_names[] = {
      "none",
      "sse",
      "sse2",
      "sse3",
      "ssse3",
      "sse4_1",
      "sse4_2",
#if BX_SUPPORT_AVX
      "avx",
      "avx2",
#if BX_SUPPORT_EVEX
      "avx512",
#endif
#endif
      NULL };

  new bx_param_enum_c(cpuid_param,
      "simd", "Support for SIMD instruction set",
      "Support for SIMD (SSE/SSE2/SSE3/SSSE3/SSE4_1/SSE4_2/AVX/AVX2/AVX512) instruction set",
      simd_names,
      BX_CPUID_SUPPORT_SSE2,
      BX_CPUID_SUPPORT_NOSSE);

  new bx_param_bool_c(cpuid_param,
      "sse4a", "Support for AMD SSE4A instructions",
      "Support for AMD SSE4A instructions",
      0);
  new bx_param_bool_c(cpuid_param,
      "misaligned_sse", "Support for AMD Misaligned SSE mode",
      "Support for AMD Misaligned SSE mode",
      0);

  new bx_param_bool_c(cpuid_param,
      "sep", "Support for SYSENTER/SYSEXIT instructions",
      "Support for SYSENTER/SYSEXIT instructions",
      1);
  new bx_param_bool_c(cpuid_param,
      "movbe", "Support for MOVBE instruction",
      "Support for MOVBE instruction",
      0);
  new bx_param_bool_c(cpuid_param,
      "adx", "Support for ADX instructions",
      "Support for ADCX/ADOX instructions",
      0);
  new bx_param_bool_c(cpuid_param,
      "aes", "Support for AES instruction set",
      "Support for AES instruction set",
      0);
  new bx_param_bool_c(cpuid_param,
      "sha", "Support for SHA instruction set",
      "Support for SHA instruction set",
      0);
  new bx_param_bool_c(cpuid_param,
      "xsave", "Support for XSAVE extensions",
      "Support for XSAVE extensions",
      0);
  new bx_param_bool_c(cpuid_param,
      "xsaveopt", "Support for XSAVEOPT instruction",
      "Support for XSAVEOPT instruction",
      0);
#if BX_SUPPORT_AVX
  new bx_param_bool_c(cpuid_param,
      "avx_f16c", "Support for AVX F16 convert instructions",
      "Support for AVX F16 convert instructions",
      0);
  new bx_param_bool_c(cpuid_param,
      "avx_fma", "Support for AVX FMA instructions",
      "Support for AVX FMA instructions",
      0);
  new bx_param_num_c(cpuid_param,
      "bmi", "Support for BMI instructions",
      "Support for Bit Manipulation Instructions (BMI)",
      0, 2,
      0);
  new bx_param_bool_c(cpuid_param,
      "xop", "Support for AMD XOP instructions",
      "Support for AMD XOP instructions",
      0);
  new bx_param_bool_c(cpuid_param,
      "fma4", "Support for AMD four operand FMA instructions",
      "Support for AMD FMA4 instructions",
      0);
  new bx_param_bool_c(cpuid_param,
      "tbm", "Support for AMD TBM instructions",
      "Support for AMD Trailing Bit Manipulation (TBM) instructions",
      0);
#endif
#if BX_SUPPORT_X86_64
  new bx_param_bool_c(cpuid_param,
      "x86_64", "x86-64 and long mode",
      "Support for x86-64 and long mode",
      1);
  new bx_param_bool_c(cpuid_param,
      "1g_pages", "1G pages support in long mode",
      "Support for 1G pages in long mode",
      0);
  new bx_param_bool_c(cpuid_param,
      "pcid", "PCID support in long mode",
      "Support for process context ID (PCID) in long mode",
      0);
  new bx_param_bool_c(cpuid_param,
      "fsgsbase", "FS/GS BASE access instructions support",
      "FS/GS BASE access instructions support in long mode",
      0);
#endif
  new bx_param_bool_c(cpuid_param,
      "smep", "Supervisor Mode Execution Protection support",
      "Supervisor Mode Execution Protection support",
      0);
  new bx_param_bool_c(cpuid_param,
      "smap", "Supervisor Mode Access Prevention support",
      "Supervisor Mode Access Prevention support",
      0);
#if BX_SUPPORT_MONITOR_MWAIT
  new bx_param_bool_c(cpuid_param,
      "mwait", "MONITOR/MWAIT instructions support",
      "MONITOR/MWAIT instructions support",
      BX_SUPPORT_MONITOR_MWAIT);
#endif
#if BX_SUPPORT_VMX
  new bx_param_num_c(cpuid_param,
      "vmx", "Support for Intel VMX extensions emulation",
      "Support for Intel VMX extensions emulation",
      0, BX_SUPPORT_VMX,
      1);
#endif
#if BX_SUPPORT_SVM
  new bx_param_bool_c(cpuid_param,
      "svm", "Secure Virtual Machine (SVM) emulation support",
      "Secure Virtual Machine (SVM) emulation support",
      0);
#endif
#endif // CPU_LEVEL >= 6

  //cpuid_param->set_options(menu->SHOW_PARENT | menu->USE_SCROLL_WINDOW);

  // CPUID subtree depends on CPU model
  SIM->get_param_enum(BXPN_CPU_MODEL)->set_dependent_list(cpuid_param->clone(), 0);
  // enable CPUID subtree only for CPU model choice #0
  SIM->get_param_enum(BXPN_CPU_MODEL)->set_dependent_bitmap(0, BX_MAX_BIT64U);

#endif // CPU_LEVEL >= 4



}

void bx_reset_options()
{
  // cpu
  SIM->get_param("cpu")->reset();

#if BX_CPU_LEVEL >= 4
  // cpuid
  SIM->get_param("cpuid")->reset();
#endif

}