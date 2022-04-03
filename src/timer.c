/*
 * Timer fucntions, with callback, mainly for Windows and *nix.
 *
 * This file is part of uSNMP ("micro-SNMP").
 * uSNMP is released under a BSD-style license. The full text follows.
 *
 * Copyright (c) 2022 Francis Tay. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is hereby granted without fee provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Francis Tay nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY FRANCIS TAY AND CONTRIBUTERS 'AS
 * IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOTLIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL FRANCIS TAY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARAY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "timer.h"

void (*timer_func_handler)(void);

#ifdef _WIN32

HANDLE timer;
VOID CALLBACK timer_sig_handler(PVOID, BOOLEAN);

int timer_start(int mSec, void (*timer_handler)(void))
{
	timer_func_handler = timer_handler;

	if(CreateTimerQueueTimer(&timer, NULL, (WAITORTIMERCALLBACK)timer_sig_handler, NULL, mSec, mSec, WT_EXECUTEINTIMERTHREAD) == 0)
		return 0;
	else
		return 1;
}

VOID CALLBACK timer_sig_handler(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	timer_func_handler();
}

void timer_stop(void)
{
	DeleteTimerQueueTimer(NULL, timer, NULL);
	CloseHandle(timer);
}

#else

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

struct itimerval timer;
struct sigaction new_handler, old_handler;
void timer_sig_handler(int);

int timer_start(int mSec, void (*timer_handler)(void))
{
	timer_func_handler = timer_handler;

	timer.it_interval.tv_sec = timer.it_value.tv_sec = mSec / 1000;
	timer.it_interval.tv_usec = timer.it_value.tv_usec = (mSec % 1000) * 1000;
	if(setitimer(ITIMER_REAL, &timer, NULL))
		return 0;

	new_handler.sa_handler = &timer_sig_handler;
	new_handler.sa_flags = SA_NOMASK;
	if(sigaction(SIGALRM, &new_handler, &old_handler))
		return 0;
	else
		return 1;
}

void timer_sig_handler(int arg)
{
	timer_func_handler();
}

void stop_timer(void)
{
	timer.it_interval.tv_sec = timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = timer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	sigaction(SIGALRM, &old_handler, NULL);
}

#endif













