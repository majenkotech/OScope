
float sampleAverage(uint8_t c) {
	float a = 0;
	
	for (uint32_t x = 0; x < sampleSize - fb.getWidth(); x++) {
		a += channels[c].values[x];
	}
	a = a / sampleSize;
	return a;
}

uint32_t findFirstFallingValue(uint8_t c, uint32_t offset) {
	for (uint32_t x = offset + 1; x < sampleSize - fb.getWidth(); x++) {
		if (channels[c].values[x-1] > channels[c].values[x]) {
			return x;
		}
	}
	return 0;
}

uint32_t findFirstRisingValue(uint8_t c, uint32_t offset) {
	for (uint32_t x = offset + 1; x < sampleSize - fb.getWidth(); x++) {
		if (channels[c].values[x-1] < channels[c].values[x]) {
			return x;
		}
	}
	return 0;
}

uint32_t findTriggerOffsetZeroCrossing(uint8_t c) {
	for (uint32_t x = 1; x < sampleSize - fb.getWidth(); x++) {
		if (channels[c].values[x-1] < 0 && channels[c].values[x] >= 0) {
			return x-1;
		}
	}
	return 0;
}

uint32_t findTriggerOffsetAverageCrossing(uint8_t c) {
	float a = sampleAverage(c);
	for (uint32_t x = 1; x < sampleSize - fb.getWidth(); x++) {
		if (channels[c].values[x-1] < a && channels[c].values[x] >= a) {
			return x-1;
		}
	}
	return 0;
}

uint32_t findTriggerOffsetRising(uint8_t c) {
	uint32_t startPoint = findFirstFallingValue(c, 0);
	uint32_t triggerPoint = findFirstRisingValue(c, startPoint);
	return triggerPoint;
}

