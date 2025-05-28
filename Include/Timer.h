//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
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

#pragma once

#include <chrono>

using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;	// TimeStamp != std::time_t
using Duration  = std::chrono::duration<float>;

class Timer
{
public:
	Timer();

	// returns the time duration between Start() and Now, minus the paused duration.
	float TotalTime() const;

	// returns the last delta time measured between Start() and Stop()
	float DeltaTime() const;
	float GetPausedTime() const;
	float GetStopDuration() const;	// gets (Now - stopTime)

	void Reset();
	void Start();
	void Stop();
	inline float StopGetDeltaTimeAndReset() { Stop(); float dt = DeltaTime(); Reset(); return dt; }

	// A Timer as to be updated by calling tick. 
	// Tick() will return the time duration since the last time Tick() is called.
	// First call will return the duration between Start() and Tick().
	float Tick();

private:
	TimeStamp baseTime;
	TimeStamp prevTime , currTime;
	TimeStamp startTime, stopTime;
	Duration  pausedTime;
	Duration  dt;
	bool      bIsStopped;
};

