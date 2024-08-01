/////////////////////////////////////////////////////////////////////////
// $Id: cpu.cc 12509 2014-10-15 18:00:04Z sshwarts $
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
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "cpustats.h"
#include "dmsg.h"
#include "gsaccess.h"
#include "modlist.h"
#include "hookproc.h"
#include "haeapi.h"
#include "pebproc.h"
#include "haeexception.h"

#include "fetchdecode.h"

// Global -------------------------------
Bit64u ticks = 0;       // 1 GHz
Bit64u system_file_time;

Bit64u nThreads = 0;
volatile Bit64u nRunningThread = 0;
volatile Bit64u flagsStartExec = 0;
volatile Bit64u execDoneCnt;
//---------------------------------------

// table of all Bochs opcodes
extern struct bxIAOpcodeTable BxOpcodesTable[];

char* disasm(char *disbufptr, const bxInstruction_c *i, bx_address cs_base, bx_address rip);

int   TraceIns = 0;

int     _DebugThreadIndex = 0;
Bit64u _DebugInsCntStart = 0;
Bit64u _DebugInsCntEnd = 0;
int _CheckInsCntStart = 0;
int _CheckInsCntEnd = 0;

Bit64u _DebugAddrStart = 0;
Bit64u _DebugAddrEnd = 0;
int _CheckAddrStart = 0;
int _CheckAddrEnd = 0;
Bit64u _BreakPoint = 0;

void BX_CPU_C::IncTicks(unsigned n)
{
    static Bit64u ms_cnt = 0;
    ticks += n;
    ms_cnt += n;
    if(ms_cnt > 100) {
        ms_cnt = 0;
        system_file_time++;
    }
}

void BX_CPU_C::SetHookParam(bool dump)
{
#if 1
    if(dump) {

        OnFetch = NULL;
        OnMemRead = OnMemReadApi;
        OnMemWrite = OnMemWriteApi;
        OnPEBRead = NULL;
        OnPEBWrite = NULL;
        OnTEBRead = NULL;
        OnTEBWrite = NULL;
        return;
    }

#endif
    bNotTargetFetch = 1;
    TargetLower = _stack;
    TargetUpper = PackerEnd;
    bTargetFetch = 1;


    if(thread_index == 0) {
        OnFetch = OnFetch0;
        OnMemRead = OnMemRead0;
        OnMemWrite = OnMemWrite0;
        OnPEBRead = OnPEBRead0;
        OnPEBWrite = OnPEBWrite0;
        OnTEBRead = OnTEBRead0;
        OnTEBWrite = OnTEBWrite0;


    } else {
        OnFetch = OnFetch0;
        OnMemRead = OnMemRead1;
        OnMemWrite = OnMemWrite1;
        OnPEBRead = OnPEBRead1;
        OnPEBWrite = OnPEBWrite1;
        OnTEBRead = OnTEBRead1;
        OnTEBWrite = OnTEBWrite1;
    }




}

void BX_CPU_C::SetDebug(int val)
{
	_Debug = val;
}

void BX_CPU_C::SetThreadParam(HaeThreadParam *param)
{

     thread_index = param->thread_index;
    _TargetImageBase = param->_TargetImageBase;
    _TargetImageEnd = param->_TargetImageEnd;
    _PEB = param->_PEB;
    _PEBend = param->_PEBend;
    _TEB = param->_TEB;
   _TEBend = param->_TEBend;
   _stacktop = param->_stacktop;
   _stack = param->_stack;


}


// BX_CPU implementation
int BX_CPU_C::IsBreak()
{
	return shouldBreak;

}
Bit64u BX_CPU_C::SuspendThread()
{
    MSG((thread_index, OUTPUT_BOTH, "CPU Suspended"));
    shouldBreak = 1;
    return suspendCount++;
}
Bit64u BX_CPU_C::ResumeThread()
{
    if(suspendCount == 0) {
        MSG((thread_index, OUTPUT_BOTH, "ResumeThread: CPU not suspended"));
        return 0;
    } else {
       suspendCount--;
       if(suspendCount == 0) {
        MSG((thread_index, OUTPUT_BOTH, "ResumtThread: ##### not implemented: CPU restarted"));
       }
       return suspendCount;
    }
}
void BX_CPU_C::StopExec()
{
   MSG((thread_index, OUTPUT_BOTH, "break: StopExec: %llx ticks = %llx  icount = %llx", RIP, ticks, BX_CPU_THIS_PTR icount));
	shouldBreak = 1;
#if 0
    Bit64u *p = (Bit64u*)RSP;
    int n = _stacktop - RSP;
    n = n / 8;
    for (int i=0; i<n; i++) {
        MSG((thread_index, OUTPUT_BOTH, "%llx : %016llx", p, *p));
        p++;
    }
#endif
    c_state = T_STATE_STOP;

    char msg[256];
    sprintf(msg, "StopExec: tid = %d RIP: %llx", thread_index, RIP);
    ShowMsgBox(msg);
}

void BX_CPU_C::ClearBreak()
{
	shouldBreak = 0;
}


Bit64u BX_CPU_C::pop(Bit8u size)
{
	Bit64u val = readMem(RSP, size);
	RSP += (Bit64u)size;
	return val;
}

void BX_CPU_C::push(Bit64u val, Bit8u size)
{
	RSP -= size;
	writeMem(RSP, val, size);
}
//***************************************************************** mem i/f read
Bit8u BX_CPU_C::readByte(Bit64u addr)
{
	Bit8u* p = (Bit8u*)addr;

	Bit8u data = *p;

    if(Fetching) {
         if(OnFetch) OnFetch(thread_index, addr, &data, 1, this);
    } else {
        if(OnMemRead) if(OnMemRead(thread_index, addr, &data, 1, this)) return data;
    }
    if (addr >= _TEB && addr < _TEBend) {
        if(OnTEBRead) if(OnTEBRead(thread_index, addr, &data, 1, this)) return data;
    }
    if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBRead) if(OnPEBRead(thread_index, addr, &data, 1, this)) return data;
    }
	return data;
}

Bit16u BX_CPU_C::readWord(Bit64u addr)
{

	Bit16u *p = (Bit16u*)addr;
	Bit16u data = *p;

    if(Fetching) {
         if(OnFetch) OnFetch(thread_index, addr, &data, 2, this);
    } else {
        if(OnMemRead) if(OnMemRead(thread_index, addr, &data, 2, this)) return data;
    }
    if (addr >= _TEB && addr < _TEBend) {
        if(OnTEBRead) if(OnTEBRead(thread_index, addr, &data, 2, this)) return data;
    }
    if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBRead) if(OnPEBRead(thread_index, addr, &data, 2, this)) return data;
    }
	return data;

}

Bit32u BX_CPU_C::readDword(Bit64u addr)
{

	Bit32u *p = (Bit32u*)addr;
	Bit32u data = *p;
    if(Fetching) {
         if(OnFetch) OnFetch(thread_index, addr, &data, 4, this);
    } else {
        if(OnMemRead) if(OnMemRead(thread_index, addr, &data, 4, this)) return data;
    }
    if (addr >= _TEB && addr < _TEBend) if(OnTEBRead) {
        if(OnTEBRead(thread_index, addr, &data, 4, this)) return data;
    }
    if (addr >= _PEB && addr < _PEBend){
        if(OnPEBRead) if(OnPEBRead(thread_index, addr, &data, 4, this)) return data;
    }
	return data;

}
Bit64u BX_CPU_C::readQword(Bit64u addr)
{

	Bit64u *p = (Bit64u*)addr;
	Bit64u data = *p;
    if(Fetching) {
         if(OnFetch) OnFetch(thread_index, addr, &data, 8, this);
    } else {
        if(OnMemRead) if(OnMemRead(thread_index, addr, &data, 8, this)) return data;
    }
    if (addr >= _TEB && addr < _TEBend) if(OnTEBRead) {
        if(OnTEBRead(thread_index, addr, &data, 8, this)) return data;
    }
    if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBRead) if(OnPEBRead(thread_index, addr, &data, 8, this)) return data;
    }
	return data;
}

//all reads from memory should be through this function
Bit64u BX_CPU_C::readMem(Bit64u addr, Bit8u size) 
{
	Bit64u result = 0;
	//   addr += segmentBase;

	switch (size) {
	case SIZE_BYTE:
		result = (Bit64u)readByte(addr);
		break;
	case SIZE_WORD:
		result = (Bit64u)readWord(addr);
		break;
	case SIZE_DWORD:
		result = (Bit64u)readDword(addr);
		break;
	case SIZE_QWORD:
		result = (Bit64u)readQword(addr);
		break;
	}

	return result;
}

Bit32u BX_CPU_C::readBuffer(Bit64u addr, void *buf, Bit32u nbytes) {

	for (Bit32u i = 0; i < nbytes; i++) {
		((unsigned char*)buf)[i] = readByte(addr + i);
	}
	return nbytes;
}

//*************************************************************** Mem i/f write
void BX_CPU_C::writeByte(Bit64u addr, Bit8u val) 
{

	Bit8u *p = (Bit8u *)addr;
    if(OnMemWrite) if(OnMemWrite(thread_index, addr, &val, 1, this)) return;

	if (addr >= _TEB && addr < _TEBend) {
        if(OnTEBWrite)
        if (OnTEBWrite(thread_index, addr, &val, 1, this)) return;
	}
	if (addr >= _PEB && addr < _PEBend) {
        if (OnPEBWrite)
        if (OnPEBWrite(thread_index, addr, &val, 1, this)) return;
	}
	*p = val;
}


void BX_CPU_C::writeWord(Bit64u addr, Bit16u val)
{
	//   writeByte(addr, (uint8_t)val);
	//   writeByte(addr + 1, (uint8_t)(val >> 8));

	Bit16u *p = (Bit16u*)addr;
    if(OnMemWrite) if(OnMemWrite(thread_index, addr, &val, 2, this)) return;
	if (addr >= _TEB && addr < _TEBend) {
        if(OnTEBWrite)
        if (OnTEBWrite(thread_index,addr, &val, 2, this)) return;
	}
	if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBWrite)
        if (OnPEBWrite(thread_index,addr, &val, 2, this)) return;
	}

	*p = val;
}

void BX_CPU_C::writeDword(Bit64u addr, Bit32u val)
{

	//writeWord(addr, (uint16_t)val);
	//writeWord(addr + 2, (uint16_t)(val >> 16));
	Bit32u *p = (Bit32u*)addr;
    if(OnMemWrite) if(OnMemWrite(thread_index, addr, &val, 4, this)) return;
	if (addr >= _TEB && addr < _TEBend) {
        if (OnTEBWrite)
        if (OnTEBWrite(thread_index,addr, &val, 4, this)) return;
	}
	if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBWrite)
        if (OnPEBWrite(thread_index,addr, &val, 4, this)) return;
	}
	*p = val;
}
void BX_CPU_C::writeQword(Bit64u addr, Bit64u val)
{

	Bit64u *p = (Bit64u*)addr;
    if(OnMemWrite) if(OnMemWrite(thread_index, addr, &val, 8, this)) return;
	if (addr >= _TEB && addr < _TEBend) {
        if (OnTEBWrite)
        if (OnTEBWrite(thread_index, addr, &val, 8, this)) return;
	}
	if (addr >= _PEB && addr < _PEBend) {
        if(OnPEBWrite)
        if (OnPEBWrite(thread_index, addr, &val, 8, this)) return;
	}
	*p = val;
}

//all writes to memory should be through this function
void BX_CPU_C::writeMem(Bit64u addr, Bit64u val, Bit8u size) {

	switch (size) {
	case SIZE_BYTE:
		writeByte(addr, (Bit8u)val);
		break;
	case SIZE_WORD:
		writeWord(addr, (Bit16u)val);
		break;
	case SIZE_DWORD:
		writeDword(addr, (Bit32u)val);
		break;
	case SIZE_QWORD:
		writeQword(addr, val);
		break;

	}
}

Bit32u BX_CPU_C::writeBuffer(Bit64u addr, void *buf, Bit32u nbytes)
{

	for (Bit32u i = 0; i < nbytes; i++) {
		writeByte(addr + i, (Bit8u)((unsigned char*)buf)[i]);
	}
	return nbytes;
}



void BX_CPU_C::cpu_loop(void)
{

//	bxInstruction_c iStroage, *i = &iStroage;

      // want to allow changing of the instruction inside instrumentation callback
//      BX_INSTR_BEFORE_EXECUTION(BX_CPU_ID, i);
//      RIP += i->ilen();
//      BX_CPU_CALL_METHOD(i->execute1, (i)); // might iterate repeat instruction
//      BX_CPU_THIS_PTR prev_rip = RIP; // commit new RIP
//      BX_INSTR_AFTER_EXECUTION(BX_CPU_ID, i);
//      BX_CPU_THIS_PTR icount++;

//      BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);

}

static int SkipDis(const bxInstruction_c* i)
{

    return 0;

    Bit16u opcode = i->getIaOpcode();
    if(opcode >= BX_IA_JMP_Ed && opcode <= BX_IA_JZ_Jd) return 1;
    if(opcode >= BX_IA_CALL_Jq && opcode <= BX_IA_IRET_Op64) return 1;
    if(opcode >= BX_IA_CALL_Eq && opcode <= BX_IA_JMPF_Op64_Ep) return 1;


    return 0;
}


Bit64u _callLoc;
extern Bit64u ticks;
static const char *regname[17] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15", "rip"
};
char* dis_sprintf(char *disbufptr, const char *fmt, ...);

const char* BX_CPU_C::disasm_regval(char* disbufptr, const bxInstruction_c* i)
{
    int regno0 = i->dst();
    if(regno0 >= 16) regno0 = 0;

    int regno1 = i->getSrcReg(1);
    if(regno1 >= 16) regno1 = 0;

    Bit64u dst = BX_CPU_THIS_PTR gen_reg[i->dst()].rrx;

    Bit16u ia_opcode = i->getIaOpcode();
    unsigned src0 = (unsigned) BxOpcodesTable[ia_opcode].src[0];
    unsigned src_type0 = src0 >> 3;

    //int bcall = 0;
    //if(ia_opcode >= BX_IA_JMP_Ed && ia_opcode <= BX_IA_JZ_Jd) bcall = 1;
    //if(ia_opcode >= BX_IA_CALL_Jq && ia_opcode <= BX_IA_IRET_Op64) bcall = 1;
    //if(ia_opcode >= BX_IA_CALL_Eq && ia_opcode <= BX_IA_JMPF_Op64_Ep) bcall = 1;

    unsigned src1 = (unsigned) BxOpcodesTable[ia_opcode].src[0];
    unsigned src_type1 = src1 >> 3;

    if(src_type0 < 16) {
        disbufptr = dis_sprintf(disbufptr, "%-3s = %016llx", regname[regno0], BX_CPU_THIS_PTR gen_reg[regno0].rrx);
    } else {
        disbufptr = dis_sprintf(disbufptr, "%s", "                      ");
    }

    if(src_type1 < 16) {
        disbufptr = dis_sprintf(disbufptr, " %-3s = %016llx", regname[regno1], BX_CPU_THIS_PTR gen_reg[regno1].rrx);
    } else {
        disbufptr = dis_sprintf(disbufptr, " %s", "                      ");
    }

    disbufptr = dis_sprintf(disbufptr, " eflags = %04x  %llx", BX_CPU_THIS_PTR force_flags(), RSP );

    return disbufptr;
}
//========================================================================
void BX_CPU_C::executeInstruction(void)
{
    char dstr[1024], dstr1[1024];
    BX_CPU_THIS_PTR prev_rip = RIP;
    BX_CPU_THIS_PTR speculative_rsp = 0;
    bxInstruction_c iStroage, *i = &iStroage;

   // if(thread_index == 2 && bx_cpu.icount > 100000000) SetDebug(1);

    if(RIP == EXCEPTION_VEC_RETURN_MAGIC) {
        haeExceptionVecReturn(this);

    } else {
       QString func = isAPIEntry(RIP);
       if(!func.isEmpty()) {
           if(_Debug) MSG((thread_index, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx TOS: %llx", _callLoc, func, RIP));
           Bit64u nextRIP = haeapi->ExecAPI(func, RIP, this);
           if(nextRIP) {
              goto ExecuteCode;
           }
       } else {
ExecuteCode:
          Fetching = 1;
          int ret= fetchDecode64(RIP, 0, i, 16);
          Fetching = 0;
          Bit64u RIP0 = RIP;

          //Obsidium1.8.3.2
          if(bx_cpu.icount > 16527140 && bx_cpu.icount < 16527143)
              MSG((thread_index, OUTPUT_BOTH, "RIP: %llx, icount: %lld", RIP, bx_cpu.icount));




          if(ret) {
              MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
              StopExec();
          } else {

            if(_Debug && !SkipDis(i))  {
               disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
           //  MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s", RIP0, dstr));
            }
           _callLoc = RIP;
           RIP += i->ilen();
           BX_CPU_CALL_METHOD(i->execute1, (i));

           if(_Debug && !SkipDis(i) && !_RepeatIns)  {
                disasm_regval(dstr1, i);
                MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));

           }
           BX_CPU_THIS_PTR icount++;

           // BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);
           if(thread_index == 0) IncTicks(10);


           if(_BreakPoint)
               if(_BreakPoint == RIP ) {
                   MSG((thread_index, OUTPUT_BOTH, "rax = %llx rbp = %llx rbx = %llx rcx = %llx rdx = %llx rsi = %llx rdi = %llx tos = %llx",
                        RAX, RBP, RBX, RCX, RDX, RSI, RDI, readDword(RSP)));
                   bx_cpu.StopExec();
               }

           if(thread_index == _DebugThreadIndex) {



              // icount debug start
              if(_DebugInsCntStart && !_CheckInsCntStart) {
                  if(BX_CPU_THIS_PTR icount >= _DebugInsCntStart) {
                      SetDebug(1);
                      _CheckInsCntStart = 1;
                   }
              }
              // icount debug endt
              if(_DebugInsCntEnd && !_CheckInsCntEnd) {
                  if(BX_CPU_THIS_PTR icount > _DebugInsCntEnd) {
                      _CheckInsCntEnd = 1;
                      SetDebug(0);
                  }
              }
              // RIP debug start
              if(_DebugAddrStart && !_CheckAddrStart) {
                  if(RIP == _DebugAddrStart) {
                      SetDebug(1);
                      _CheckAddrStart = 1;
                   }
              }
              // RIP debug end
              if(_DebugAddrEnd && !_CheckAddrEnd) {
                  if(RIP == _DebugAddrEnd) {
                      SetDebug(0);
                      _CheckAddrEnd = 1;
                   }
              }
           }
         }
      }
   }
}

//================
void BX_CPU_C::ExecuteSimpleBlock(Bit64u start_addr, Bit64u end_addr, PARAM_BLK* param_blk)
{
    char dstr[1024], dstr1[1024];

    bxInstruction_c iStroage, *i = &iStroage;

    BX_CPU_THIS_PTR icount = 0;
    bool found = false;
    RIP = start_addr;
    RSP = _stacktop - 0x40;

    RSP -= 0x10;
    RCX = param_blk->params[0];
    RDX = param_blk->params[1];
    R8 = param_blk->params[2];
    R9 = param_blk->params[3];

    if(param_blk->nparam > 4) {

        for(int i =0; i<param_blk->nparam-4; i++) {
            push(param_blk->params[param_blk->nparam - i -1], 8);
        }
    }
 //   if(RIP == 0x45872d) SetDebug(1);

    MSG((0, OUTPUT_DEBUG, "RCX: %LLX RDX: %LLX r8: %LLX R9: %LLX", RCX, RDX, R8, R9));

    _Debug = 1;

    while(icount < 100000000) {   // timeout

      BX_CPU_THIS_PTR prev_rip = RIP;
      BX_CPU_THIS_PTR speculative_rsp = 0;

      if(IsBreak()) break;
      if(RIP == end_addr) {
          MSG((0, 3, "Execution Done"));
          MSG((0, OUTPUT_DEBUG, "RCX: %LLX RDX: %LLX r8: %LLX R9: %LLX", RCX, RDX, R8, R9));
          return;
      }

      QString func = isAPIEntry(RIP);
      if(!func.isEmpty()) {
          if(_Debug) MSG((0, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx", _callLoc, func, RIP));
          Bit64u nextRIP = haeapi->ExecAPI(func, RIP, this);
          if(nextRIP) {
             goto ExecuteCode;
          }
      } else {
ExecuteCode:
         Fetching = 1;
         int ret= fetchDecode64(RIP, 0, i, 16);
         Fetching = 0;

         Bit64u RIP0 = RIP;
         if(ret) {
             MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
             StopExec();
         } else {

           if(_Debug)  {
              disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
          //  MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s", RIP0, dstr));
           }
          _callLoc = RIP;
          RIP += i->ilen();
          BX_CPU_CALL_METHOD(i->execute1, (i));

          if(_Debug)  {
               disasm_regval(dstr1, i);
               MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));

          }
          BX_CPU_THIS_PTR icount++;

          // BX_SYNC_TIME_IF_SINGLE_PROCESSOR(0);
          if(thread_index == 0) IncTicks(10);

        }
     }


   }
   MSG((0, 3, "??? Execution TIMEOUT" ));
   SetDebug(0);

}



extern Bit64u data_api_addr;
extern Bit64u data_api_ins_loc;
extern bool    read_api_addr;


// Safengine_ResolveAPI ============================================================================
void BX_CPU_C::Safengine_ResolveAPI(Bit64u start_addr, RESOLVE_API_INFO* info, RESOLVE_API_SEG_INFO *tinfo)
{
    char dstr[1024], dstr1[1024];

    bxInstruction_c iStroage, *i = &iStroage;

    BX_CPU_THIS_PTR icount = 0;
    bool found = false;
    RIP = start_addr;
    data_api_addr = 0;          // API address
    bool in_api = false;        // entered API obfuscator routine
    RSP = _stacktop - 0x40;
    bool read_api_addr = false;

    push(JMP_API_RETURN, 8);    // jmp _imp_....  return address

    info->dll_cnt = 0;
    info->result = 0;
    info->data_code = 0;


 //   if(RIP == 0x45872d) SetDebug(1);

  //  static int cnt = 0;
  //  cnt++;
  //  if(cnt == 1) SetDebug(1);
  //  if(cnt == 2) SetDebug(0);

    while(icount < 100000000) {   // timeout

      BX_CPU_THIS_PTR prev_rip = RIP;
      BX_CPU_THIS_PTR speculative_rsp = 0;

       QString func = isAPIEntry(RIP);
       if(!func.isEmpty()) {

           if(func.startsWith("GetModuleHandleA") && !read_api_addr) {

                 Bit64u nextRIP = haeapi->ExecAPI(func, RIP, this);
                 if(info->dll_cnt < 3) {
                     info->dll_handle[info->dll_cnt] = RAX;
                 }
                 info->dll_cnt++;

                 MSG((0, OUTPUT_BOTH, "GetModuleHandleA: %llx", RAX));

           } else {
              if(_Debug) MSG((thread_index, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx", _callLoc, func, RIP));
              info->ret_addr = pop(8);
              info->api_addr = RIP;
              info->result = 1;
              found = true;
              if( (info->ret_addr >= tinfo->text_start && info->ret_addr < tinfo->text_end) || info->ret_addr == JMP_API_RETURN) {
                  break;
              } else {
                  RIP = info->ret_addr;   // strange ???     call obfs-api / call - org api / return proc / return org ???
                  break;
              }
           }

       } else if (in_api && RIP >= tinfo->text_start && RIP < tinfo->text_end){ // check return

           if(data_api_addr && !found) {

              info->result = 2;
              info->ret_addr = RIP;
              info->api_addr = data_api_addr;
              Bit64u *p = (Bit64u*)data_api_ins_loc;
              info->data_code = *p;
              found = true;
           } else {  // may be incorrect !!
               info->result = 7;
               info->ret_addr = RIP;
               found = true;
           }
           break;


       } else {

          Fetching = 1;
          int ret= fetchDecode64(RIP, 0, i, 16);
          Fetching = 0;

          if(!in_api && isSegAddress(RIP, 1)) {
              in_api = true;
          }

          Bit64u RIP0 = RIP;
          if(ret) {
              MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
              return;

          } else {

              if(_Debug)  {
               disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
              }

              _callLoc = RIP;
              RIP += i->ilen();
              BX_CPU_CALL_METHOD(i->execute1, (i));

              if(_Debug)  {
                disasm_regval(dstr1, i);
                MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));
              }
              BX_CPU_THIS_PTR icount++;
              if(thread_index == 0) IncTicks(10);
         }
      }
   }
   if(!found) {
       info->result = 0;
   }
   SetDebug(0);

}

void BX_CPU_C::Safengine_ResolveAPI_Full(Bit64u start_addr, RESOLVE_API_INFO* info, RESOLVE_API_SEG_INFO *tinfo)
{
    char dstr[1024], dstr1[1024];

    bxInstruction_c iStroage, *i = &iStroage;

    BX_CPU_THIS_PTR icount = 0;
    bool found = false;
    RIP = start_addr;
    data_api_addr = 0;          // API address
    bool in_api = false;        // entered API obfuscator routine
    RSP = _stacktop - 0x40;
    bool read_api_addr = false;

    push(JMP_API_RETURN, 8);    // jmp _imp_....  return address

    info->dll_cnt = 0;
    info->result = 0;
    info->data_code = 0;


 //   if(RIP == 0x45872d) SetDebug(1);

#if 0
    static int cnt = 0;
    cnt++;
    if(cnt == 1) SetDebug(1);
    if(cnt == 2) SetDebug(0);
#endif
    while(icount < 400000000) {   // timeout

      BX_CPU_THIS_PTR prev_rip = RIP;
      BX_CPU_THIS_PTR speculative_rsp = 0;

       QString func = isAPIEntry(RIP);
       if(!func.isEmpty()) {


           MSG((thread_index, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx", _callLoc, func, RIP));
           Bit64u nextRIP = haeapi->ExecAPI(func, RIP, this);
           if(nextRIP) {
              goto ExecuteCode;
           }
       } else if (in_api && RIP >= tinfo->text_start && RIP < tinfo->text_end) {
           MSG((0, 3, "RETURN "));
           found = false;
           break;

       } else {
ExecuteCode:

          Fetching = 1;
          int ret= fetchDecode64(RIP, 0, i, 16);
          Fetching = 0;

          if(!in_api && isSegAddress(RIP, 1)) {
              in_api = true;
          }

          Bit64u RIP0 = RIP;
          if(ret) {
              MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
              return;

          } else {

              if(_Debug) { // && !SkipDis(i))  {
               disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
              }

              _callLoc = RIP;
              RIP += i->ilen();
              BX_CPU_CALL_METHOD(i->execute1, (i));

              if(_Debug) { // && !SkipDis(i))  {
                disasm_regval(dstr1, i);
                MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));
              }
              BX_CPU_THIS_PTR icount++;
              if(thread_index == 0) IncTicks(10);
         }
      }
   }
   if(!found) {
       info->result = 0;
   }
   MSG((0,3,"####################### timeout"));
   SetDebug(0);

}


// ResolveAPI ============================================================================
void BX_CPU_C::VMProtect_ResolveAPI(Bit64u start_addr, RESOLVE_API_INFO* info, RESOLVE_API_SEG_INFO *tinfo)
{
    char dstr[1024], dstr1[1024];

    bxInstruction_c iStroage, *i = &iStroage;

    BX_CPU_THIS_PTR icount = 0;
    bool found = false;
    RIP = start_addr;
    data_api_addr = 0;          // API address
    bool in_api = false;        // entered API obfuscator routine
    RSP = _stacktop - 0x40;
    bool read_api_addr = false;

    push(JMP_API_RETURN, 8);    // jmp _imp_....  return address
    push(JMP_API_RETURN1, 8);    // jmp _imp_....  return address

    info->dll_cnt = 0;
    info->result = 0;
    info->data_code = 0;


   // if(RIP == 0x14000a419) SetDebug(1);

    while(icount < 100000000) {   // timeout

      BX_CPU_THIS_PTR prev_rip = RIP;
      BX_CPU_THIS_PTR speculative_rsp = 0;


       QString func = isAPIEntry(RIP);
       if(!func.isEmpty()) {

              if(_Debug) MSG((thread_index, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx", _callLoc, func, RIP));
              info->ret_addr = pop(8);
              info->api_addr = RIP;
              info->result = 1;
              found = true;
              if( (info->ret_addr >= tinfo->text_start && info->ret_addr < tinfo->text_end) || info->ret_addr == JMP_API_RETURN) {
                  break;
              } else {
                  RIP = info->ret_addr;   // strange ???     call obfs-api / call - org api / return proc / return org ???
                  break;
              }

       } else {

          Fetching = 1;
          int ret= fetchDecode64(RIP, 0, i, 16);
          Fetching = 0;

          if(!in_api && isSegAddress(RIP, 1)) {
              in_api = true;
          }

          Bit64u RIP0 = RIP;
          if(ret) {
              MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
              return;

          } else {

              if(_Debug)  {
               disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
              }

              _callLoc = RIP;
              RIP += i->ilen();
              BX_CPU_CALL_METHOD(i->execute1, (i));

              if(_Debug)  {
                disasm_regval(dstr1, i);
                MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));
              }
              BX_CPU_THIS_PTR icount++;
              if(thread_index == 0) IncTicks(10);
         }
      }
   }
   if(!found) {
       info->result = 0;
   }
   SetDebug(0);

}
#if 0
// Themida -------------------------------------------------------------------------------------------
void BX_CPU_C::Themida_ResolveAPI(Bit64u start_addr, RESOLVE_API_INFO* info, RESOLVE_API_SEG_INFO *tinfo)
{
    char dstr[1024], dstr1[1024];

    bxInstruction_c iStroage, *i = &iStroage;

    BX_CPU_THIS_PTR icount = 0;
    bool found = false;
    RIP = start_addr;
    data_api_addr = 0;          // API address
    bool in_api = false;        // entered API obfuscator routine
    RSP = _stacktop - 0x40;
    bool read_api_addr = false;

    push(JMP_API_RETURN, 8);    // jmp _imp_....  return address
    push(JMP_API_RETURN1, 8);    // jmp _imp_....  return address

    info->dll_cnt = 0;
    info->result = 0;
    info->data_code = 0;


   // if(RIP == 0x14000a419) SetDebug(1);

    while(icount < 100000000) {   // timeout

      BX_CPU_THIS_PTR prev_rip = RIP;
      BX_CPU_THIS_PTR speculative_rsp = 0;


       QString func = isAPIEntry(RIP);
       if(!func.isEmpty()) {

              if(_Debug) MSG((thread_index, OUTPUT_DEBUG, "API Call at %llx func: %llx  RIP: %llx", _callLoc, func, RIP));
              info->ret_addr = pop(8);
              info->api_addr = RIP;
              info->result = 1;
              found = true;
              if( (info->ret_addr >= tinfo->text_start && info->ret_addr < tinfo->text_end) || info->ret_addr == JMP_API_RETURN) {
                  break;
              } else {
                  RIP = info->ret_addr;   // strange ???     call obfs-api / call - org api / return proc / return org ???
                  break;
              }

       } else {

          Fetching = 1;
          int ret= fetchDecode64(RIP, 0, i, 16);
          Fetching = 0;

          if(!in_api && isSegAddress(RIP, 1)) {
              in_api = true;
          }

          Bit64u RIP0 = RIP;
          if(ret) {
              MSG((thread_index, OUTPUT_BOTH, "??? fetDecode error: %d RIP = %llx", ret, RIP0));
              return;

          } else {

              if(_Debug)  {
               disasm(dstr, (const bxInstruction_c*)i, (Bit64u)0, RIP);
              }

              _callLoc = RIP;
              RIP += i->ilen();
              BX_CPU_CALL_METHOD(i->execute1, (i));

              if(_Debug)  {
                disasm_regval(dstr1, i);
                MSG((thread_index, OUTPUT_DEBUG, "In: %llx: %-40s %s", RIP0, dstr, dstr1));
              }
              BX_CPU_THIS_PTR icount++;
              if(thread_index == 0) IncTicks(10);
         }
      }
   }
   if(!found) {
       info->result = 0;
   }
   SetDebug(0);

}
#endif

#define BX_REPEAT_TIME_UPDATE_INTERVAL (BX_MAX_TRACE_LENGTH-1)

void BX_CPP_AttrRegparmN(2) BX_CPU_C::repeat(bxInstruction_c *i, BxRepIterationPtr_tR execute)
{
  // non repeated instruction
  if (! i->repUsedL()) {
    BX_CPU_CALL_REP_ITERATION(execute, (i));
    return;
  }

#if BX_X86_DEBUGGER
  BX_CPU_THIS_PTR in_repeat = 0;
#endif

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    while(1) {
      if (RCX != 0) {
        BX_CPU_CALL_REP_ITERATION(execute, (i));
        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
        _RepeatIns = 1;
        RCX --;
      }
      if (RCX == 0) {
          _RepeatIns = 0;
          return;
      }

#if BX_DEBUGGER == 0
      if (BX_CPU_THIS_PTR async_event)
#endif
        break; // exit always if debugger enabled

      BX_CPU_THIS_PTR icount++;

      BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
    }
  }
  else
#endif
  if (i->as32L()) {
    while(1) {
      if (ECX != 0) {
        BX_CPU_CALL_REP_ITERATION(execute, (i));
        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
        RCX = ECX - 1;
        _RepeatIns = 1;
      }
      if (ECX == 0) {
          _RepeatIns = 0;
          return;
      }

#if BX_DEBUGGER == 0
      if (BX_CPU_THIS_PTR async_event)
#endif
        break; // exit always if debugger enabled

      BX_CPU_THIS_PTR icount++;

      BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
    }
  }
  else  // 16bit addrsize
  {
    while(1) {
      if (CX != 0) {
        BX_CPU_CALL_REP_ITERATION(execute, (i));
        BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
        CX --;
        _RepeatIns = 1;
      }
      if (CX == 0) {
          _RepeatIns = 0;
          return;
      }

#if BX_DEBUGGER == 0
      if (BX_CPU_THIS_PTR async_event)
#endif
        break; // exit always if debugger enabled

      BX_CPU_THIS_PTR icount++;

      BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
    }
  }

#if BX_X86_DEBUGGER
  BX_CPU_THIS_PTR in_repeat = 1;
#endif

  RIP = BX_CPU_THIS_PTR prev_rip; // repeat loop not done, restore RIP

  // assert magic async_event to stop trace execution
  BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
}

void BX_CPP_AttrRegparmN(2) BX_CPU_C::repeat_ZF(bxInstruction_c *i, BxRepIterationPtr_tR execute)
{
  unsigned rep = i->lockRepUsedValue();

  // non repeated instruction
  if (rep < 2) {
    BX_CPU_CALL_REP_ITERATION(execute, (i));
    return;
  }

#if BX_X86_DEBUGGER
  BX_CPU_THIS_PTR in_repeat = 0;
#endif

  if (rep == 3) { /* repeat prefix 0xF3 */
#if BX_SUPPORT_X86_64
    if (i->as64L()) {
      while(1) {
        if (RCX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          RCX --;
          _RepeatIns = 1;
        }
        if (! get_ZF() || RCX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
    else
#endif
    if (i->as32L()) {
      while(1) {
        if (ECX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          RCX = ECX - 1;
          _RepeatIns = 1;
        }
        if (! get_ZF() || ECX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
    else  // 16bit addrsize
    {
      while(1) {
        if (CX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          CX --;
          _RepeatIns = 1;
        }
        if (! get_ZF() || CX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
  }
  else {          /* repeat prefix 0xF2 */
#if BX_SUPPORT_X86_64
    if (i->as64L()) {
      while(1) {
        if (RCX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          RCX --;
          _RepeatIns = 1;
        }
        if (get_ZF() || RCX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
    else
#endif
    if (i->as32L()) {
      while(1) {
        if (ECX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          RCX = ECX - 1;
          _RepeatIns = 1;
        }
        if (get_ZF() || ECX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
    else  // 16bit addrsize
    {
      while(1) {
        if (CX != 0) {
          BX_CPU_CALL_REP_ITERATION(execute, (i));
          BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
          CX --;
          _RepeatIns = 1;
        }
        if (get_ZF() || CX == 0) {
            _RepeatIns = 0;
            return;
        }

#if BX_DEBUGGER == 0
        if (BX_CPU_THIS_PTR async_event)
#endif
          break; // exit always if debugger enabled

        BX_CPU_THIS_PTR icount++;

        BX_SYNC_TIME_IF_SINGLE_PROCESSOR(BX_REPEAT_TIME_UPDATE_INTERVAL);
      }
    }
  }

#if BX_X86_DEBUGGER
  BX_CPU_THIS_PTR in_repeat = 1;
#endif

  RIP = BX_CPU_THIS_PTR prev_rip; // repeat loop not done, restore RIP

  // assert magic async_event to stop trace execution
  BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
}

// boundaries of consideration:
//
//  * physical memory boundary: 1024k (1Megabyte) (increments of...)
//  * A20 boundary:             1024k (1Megabyte)
//  * page boundary:            4k
//  * ROM boundary:             2k (dont care since we are only reading)
//  * segment boundary:         any

void BX_CPU_C::prefetch(void)
{
	BX_PANIC(("prefetch called"));
}

