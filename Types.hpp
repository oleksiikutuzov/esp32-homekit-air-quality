#pragma once

struct particleSensorState_t {
	unsigned short avgPM25		   = 0;
	unsigned short measurements[5] = {0, 0, 0, 0, 0};
	unsigned char  measurementIdx  = 0;
	bool		   valid		   = false;
};
