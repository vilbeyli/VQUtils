//	VQE
//	Copyright(C) 2020  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com
//	Contact: volkanilbeyli@gmail.com

#include "Timer.h"

Timer::Timer()
	:
	bIsStopped(true)
{
	Reset();
}

TimeStamp GetNow() { return std::chrono::system_clock::now();}

float Timer::TotalTime() const
{
	Duration totalTime = Duration::zero();

    // Base   Stop       Start   Stop      Curr
    //--*-------*----------*------*---------|
    //          <---------->
    //             Paused
    if (bIsStopped)    totalTime = (stopTime - baseTime) - pausedTime;

    // Base         Stop      Start         Curr
    //--*------------*----------*------------|
    //               <---------->
    //                  Paused
	else totalTime = (currTime - baseTime) - pausedTime;

	return totalTime.count();
}


float Timer::DeltaTime() const
{
	return dt.count();
}

void Timer::Reset()
{
	baseTime = prevTime = GetNow();
	bIsStopped = true;
	dt = Duration::zero();
}

void Timer::Start()
{
	if (bIsStopped)
	{
		pausedTime = startTime - stopTime;
		prevTime = GetNow();
		bIsStopped = false;
	}
	Tick();
}

void Timer::Stop()
{
	Tick();
	if (!bIsStopped)
	{
		stopTime = GetNow();
		bIsStopped = true;
	}
}

float Timer::Tick()
{
	if (bIsStopped)
	{
		dt = Duration::zero();
		return dt.count();
	}

	currTime = GetNow();
	dt = currTime - prevTime;

	prevTime = currTime;
	dt = dt.count() < 0.0f ? dt.zero() : dt;	// make sure dt >= 0 (is this necessary?)

	return dt.count();
}

float Timer::GetPausedTime() const
{
	return pausedTime.count();
}
float Timer::GetStopDuration() const
{
	Duration stopDuration = GetNow() - stopTime;
	return stopDuration.count();
}

