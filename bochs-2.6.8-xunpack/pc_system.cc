/////////////////////////////////////////////////////////////////////////
// $Id: pc_system.cc 12615 2015-01-25 21:24:13Z sshwarts $
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
//
/////////////////////////////////////////////////////////////////////////

#include "bochs.h"
#include "cpu.h"

#define LOG_THIS bx_pc_system.

#if defined(PROVIDE_M_IPS)
double     m_ips; // Millions of Instructions Per Second
#endif

// Option for turning off BX_TIMER_DEBUG?
// Check out m_ips and ips

#define SpewPeriodicTimerInfo 0
#define MinAllowableTimerPeriod 1

const Bit64u bx_pc_system_c::NullTimerInterval = 0xffffffff;

  // constructor
bx_pc_system_c::bx_pc_system_c()
{

  BX_ASSERT(numTimers == 0);

  // Timer[0] is the null timer.  It is initialized as a special
  // case here.  It should never be turned off or modified, and its
  // duration should always remain the same.
  ticksTotal = 112345; // Reset ticks since emulator started.
  timer[0].inUse      = 1;
  timer[0].period     = NullTimerInterval;
  timer[0].active     = 1;
  timer[0].continuous = 1;
  timer[0].funct      = nullTimer;
  timer[0].this_ptr   = this;
  numTimers = 1; // So far, only the nullTimer.
}

void bx_pc_system_c::initialize(Bit32u ips)
{
  ticksTotal = 0;
  timer[0].timeToFire = NullTimerInterval;
  currCountdown       = NullTimerInterval;
  currCountdownPeriod = NullTimerInterval;
  lastTimeUsec = 0;
  usecSinceLast = 0;
  triggeredTimer = 0;


  // parameter 'ips' is the processor speed in Instructions-Per-Second
  m_ips = double(ips) / 1000000.0L;

  BX_DEBUG(("ips = %u", (unsigned) ips));
}

int bx_pc_system_c::Reset(unsigned type)
{
	
  // Always reset cpu
  for (int i=0; i<BX_SMP_PROCESSORS; i++) {
    BX_CPU(i)->reset(type, 0, 0, 0);
  }

  return(0);
}


void bx_pc_system_c::register_state(void)
{
  bx_list_c *list = new bx_list_c(SIM->get_bochs_root(), "pc_system", "PC System State");

  BXRS_DEC_PARAM_SIMPLE(list, currCountdown);
  BXRS_DEC_PARAM_SIMPLE(list, currCountdownPeriod);
  BXRS_DEC_PARAM_SIMPLE(list, ticksTotal);
  BXRS_DEC_PARAM_SIMPLE(list, lastTimeUsec);
  BXRS_DEC_PARAM_SIMPLE(list, usecSinceLast);

  bx_list_c *timers = new bx_list_c(list, "timer");
  for (unsigned i = 0; i < numTimers; i++) {
    char name[4];
    sprintf(name, "%u", i);
    bx_list_c *bxtimer = new bx_list_c(timers, name);
    BXRS_PARAM_BOOL(bxtimer, inUse, timer[i].inUse);
    BXRS_DEC_PARAM_FIELD(bxtimer, period, timer[i].period);
    BXRS_DEC_PARAM_FIELD(bxtimer, timeToFire, timer[i].timeToFire);
    BXRS_PARAM_BOOL(bxtimer, active, timer[i].active);
    BXRS_PARAM_BOOL(bxtimer, continuous, timer[i].continuous);
    BXRS_DEC_PARAM_FIELD(bxtimer, param, timer[i].param);
  }
}

// ================================================
// Bochs internal timer delivery framework features
// ================================================

int bx_pc_system_c::register_timer(void *this_ptr, void (*funct)(void *),
  Bit32u useconds, bx_bool continuous, bx_bool active, const char *id)
{
  // Convert useconds to number of ticks.
  Bit64u ticks = (Bit64u) (double(useconds) * m_ips);

  return register_timer_ticks(this_ptr, funct, ticks, continuous, active, id);
}

int bx_pc_system_c::register_timer_ticks(void* this_ptr, bx_timer_handler_t funct,
    Bit64u ticks, bx_bool continuous, bx_bool active, const char *id)
{
  unsigned i;

  // If the timer frequency is rediculously low, make it more sane.
  // This happens when 'ips' is too low.
  if (ticks < MinAllowableTimerPeriod) {
    //BX_INFO(("register_timer_ticks: adjusting ticks of %llu to min of %u",
    //          ticks, MinAllowableTimerPeriod));
    ticks = MinAllowableTimerPeriod;
  }

  // search for new timer (i = 0 is reserved for NullTimer)
  for (i = 1; i < numTimers; i++) {
    if (timer[i].inUse == 0)
      break;
  }

  if (numTimers >= BX_MAX_TIMERS) {
    BX_PANIC(("register_timer: too many registered timers"));
    return -1;
  }
#if BX_TIMER_DEBUG
  if (this_ptr == NULL)
    BX_PANIC(("register_timer_ticks: this_ptr is NULL!"));
  if (funct == NULL)
    BX_PANIC(("register_timer_ticks: funct is NULL!"));
#endif

  timer[i].inUse      = 1;
  timer[i].period     = ticks;
  timer[i].timeToFire = (ticksTotal + Bit64u(currCountdownPeriod-currCountdown)) + ticks;
  timer[i].active     = active;
  timer[i].continuous = continuous;
  timer[i].funct      = funct;
  timer[i].this_ptr   = this_ptr;
  strncpy(timer[i].id, id, BxMaxTimerIDLen);
  timer[i].id[BxMaxTimerIDLen-1] = 0; // Null terminate if not already.
  timer[i].param      = 0;

  if (active) {
    if (ticks < Bit64u(currCountdown)) {
      // This new timer needs to fire before the current countdown.
      // Skew the current countdown and countdown period to be smaller
      // by the delta.
      currCountdownPeriod -= (currCountdown - Bit32u(ticks));
      currCountdown = Bit32u(ticks);
    }
  }

  BX_DEBUG(("timer id %d registered for '%s'", i, id));
  // If we didn't find a free slot, increment the bound, numTimers.
  if (i==numTimers)
    numTimers++; // One new timer installed.

  // Return timer id.
  return i;
}

void bx_pc_system_c::countdownEvent(void)
{
  unsigned i, first = numTimers, last = 0;
  Bit64u   minTimeToFire;
  bx_bool  triggered[BX_MAX_TIMERS];

  // The countdown decremented to 0.  We need to service all the active
  // timers, and invoke callbacks from those timers which have fired.
#if BX_TIMER_DEBUG
  if (currCountdown != 0)
    BX_PANIC(("countdownEvent: ticks!=0"));
#endif

  // Increment global ticks counter by number of ticks which have
  // elapsed since the last update.
  ticksTotal += Bit64u(currCountdownPeriod);
  minTimeToFire = (Bit64u) -1;

  for (i = 0; i < numTimers; i++) {
    triggered[i] = 0; // Reset triggered flag.
    if (timer[i].active) {
#if BX_TIMER_DEBUG
      if (ticksTotal > timer[i].timeToFire)
        BX_PANIC(("countdownEvent: ticksTotal > timeToFire[%u], D " FMT_LL "u", i,
                  timer[i].timeToFire-ticksTotal));
#endif
      if (ticksTotal == timer[i].timeToFire) {
        // This timer is ready to fire.
        triggered[i] = 1;

        if (timer[i].continuous==0) {
          // If triggered timer is one-shot, deactive.
          timer[i].active = 0;
        } else {
          // Continuous timer, increment time-to-fire by period.
          timer[i].timeToFire += timer[i].period;
          if (timer[i].timeToFire < minTimeToFire)
            minTimeToFire = timer[i].timeToFire;
        }
        if (i < first) first = i;
        last = i;
      } else {
        // This timer is not ready to fire yet.
        if (timer[i].timeToFire < minTimeToFire)
          minTimeToFire = timer[i].timeToFire;
      }
    }
  }

  // Calculate next countdown period.  We need to do this before calling
  // any of the callbacks, as they may call timer features, which need
  // to be advanced to the next countdown cycle.
  currCountdown = currCountdownPeriod =
      Bit32u(minTimeToFire - ticksTotal);

  for (i = first; i <= last; i++) {
    // Call requested timer function.  It may request a different
    // timer period or deactivate etc.
    if (triggered[i] && (timer[i].funct != NULL)) {
      triggeredTimer = i;
      timer[i].funct(timer[i].this_ptr);
      triggeredTimer = 0;
    }
  }
}

void bx_pc_system_c::nullTimer(void* this_ptr)
{
	// This function is always inserted in timer[0].  It is sort of
	// a heartbeat timer.  It ensures that at least one timer is
	// always active to make the timer logic more simple, and has
	// a duration of less than the maximum 32-bit integer, so that
	// a 32-bit size can be used for the hot countdown timer.  The
	// rest of the timer info can be 64-bits.  This is also a good
	// place for some logic to report actual emulated
	// instructions-per-second (IPS) data when measured relative to
	// the host computer's wall clock.

	UNUSED(this_ptr);

}

Bit64u bx_pc_system_c::time_usec_sequential()
{
   Bit64u this_time_usec = time_usec();
   if(this_time_usec != lastTimeUsec) {
      Bit64u diff_usec = this_time_usec-lastTimeUsec;
      lastTimeUsec = this_time_usec;
      if(diff_usec >= usecSinceLast) {
        usecSinceLast = 0;
      } else {
        usecSinceLast -= diff_usec;
      }
   }
   usecSinceLast++;
   return (this_time_usec+usecSinceLast);
}

Bit64u bx_pc_system_c::time_usec()
{
  return (Bit64u) (((double)(Bit64s)time_ticks()) / m_ips);
}

void bx_pc_system_c::start_timers(void) { }

void bx_pc_system_c::activate_timer_ticks(unsigned i, Bit64u ticks, bx_bool continuous)
{


  // If the timer frequency is rediculously low, make it more sane.
  // This happens when 'ips' is too low.
  if (ticks < MinAllowableTimerPeriod) {
    //BX_INFO(("activate_timer_ticks: adjusting ticks of %llu to min of %u",
    //          ticks, MinAllowableTimerPeriod));
    ticks = MinAllowableTimerPeriod;
  }

  timer[i].period = ticks;
  timer[i].timeToFire = (ticksTotal + Bit64u(currCountdownPeriod-currCountdown)) + ticks;
  timer[i].active     = 1;
  timer[i].continuous = continuous;

  if (ticks < Bit64u(currCountdown)) {
    // This new timer needs to fire before the current countdown.
    // Skew the current countdown and countdown period to be smaller
    // by the delta.
    currCountdownPeriod -= (currCountdown - Bit32u(ticks));
    currCountdown = Bit32u(ticks);
  }
}

void bx_pc_system_c::activate_timer(unsigned i, Bit32u useconds, bx_bool continuous)
{
  Bit64u ticks;

#if BX_TIMER_DEBUG
  if (i >= numTimers)
    BX_PANIC(("activate_timer: timer %u OOB", i));
  if (i == 0)
    BX_PANIC(("activate_timer: timer 0 is the nullTimer!"));
#endif

  // if useconds = 0, use default stored in period field
  // else set new period from useconds
  if (useconds==0) {
    ticks = timer[i].period;
  } else {
    // convert useconds to number of ticks
    ticks = (Bit64u) (double(useconds) * m_ips);

    // If the timer frequency is rediculously low, make it more sane.
    // This happens when 'ips' is too low.
    if (ticks < MinAllowableTimerPeriod) {
      //BX_INFO(("activate_timer: adjusting ticks of %llu to min of %u",
      //          ticks, MinAllowableTimerPeriod));
      ticks = MinAllowableTimerPeriod;
    }

    timer[i].period = ticks;
  }

  activate_timer_ticks(i, ticks, continuous);
}

void bx_pc_system_c::deactivate_timer(unsigned i)
{
#if BX_TIMER_DEBUG
  if (i >= numTimers)
    BX_PANIC(("deactivate_timer: timer %u OOB", i));
  if (i == 0)
    BX_PANIC(("deactivate_timer: timer 0 is the nullTimer!"));
#endif

  timer[i].active = 0;
}

bx_bool bx_pc_system_c::unregisterTimer(unsigned timerIndex)
{
#if BX_TIMER_DEBUG
  if (timerIndex >= numTimers)
    BX_PANIC(("unregisterTimer: timer %u OOB", timerIndex));
  if (timerIndex == 0)
    BX_PANIC(("unregisterTimer: timer 0 is the nullTimer!"));
  if (timer[timerIndex].inUse == 0)
    BX_PANIC(("unregisterTimer: timer %u is not in-use!", timerIndex));
#endif

  if (timer[timerIndex].active) {
    BX_PANIC(("unregisterTimer: timer '%s' is still active!", timer[timerIndex].id));
    return 0; // Fail.
  }

  // Reset timer fields for good measure.
  timer[timerIndex].inUse      = 0; // No longer registered.
  timer[timerIndex].period     = BX_MAX_BIT64S; // Max value (invalid)
  timer[timerIndex].timeToFire = BX_MAX_BIT64S; // Max value (invalid)
  timer[timerIndex].continuous = 0;
  timer[timerIndex].funct      = NULL;
  timer[timerIndex].this_ptr   = NULL;
  memset(timer[timerIndex].id, 0, BxMaxTimerIDLen);

  if (timerIndex == (numTimers - 1)) numTimers--;

  return 1; // OK
}

void bx_pc_system_c::setTimerParam(unsigned timerIndex, Bit32u param)
{
#if BX_TIMER_DEBUG
  if (timerIndex >= numTimers)
    BX_PANIC(("setTimerParam: timer %u OOB", timerIndex));
#endif
  timer[timerIndex].param = param;
}

