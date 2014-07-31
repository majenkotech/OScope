
uint32_t sampleFrequency(uint8_t dec) {
	uint32_t base = 1;
	for (int i = 0; i < dec; i++) {
		base = base * 10;
	}
	return base;
}

void calculateFrequencyDomain() {
//	struct complex fft_data[SAMPLEPOINTS];
	// If sampling is currently running then just return
	if (sampleRunning()) {
		return;
	}

	for (uint8_t i = 0; i < MAXCHAN; i++) {
		if (channels[i].enabled) {
			// First zero out the imaginary array
			for (uint16_t x = 0; x < SAMPLEPOINTS; x++) {
				channels[i].imaginary[x] = 0; 
			}

			fix_fft((int16_t *)channels[i].data, channels[i].imaginary, BITS);

			// Store the FFT data in the real value array
			for (uint16_t x = 0; x < SAMPLEPOINTS/2; x++) {
				channels[i].real[x] = abs(channels[i].data[x]);
			}
		}
	}
	sampleFrequencyDomain(sampleFrequency(decades) * 2);	
	// Now we have the data converted the incoming buffer is available again.
	// Lets start filling it again with the next sample block

	// We can begin working on our calculated real values now

	for (uint8_t i = 0; i < MAXCHAN; i++) {
		if (channels[i].enabled) {
			// First zero out the imaginary array
			for (uint16_t x = 0; x < SAMPLEPOINTS; x++) {
				float imag = channels[i].imaginary[x];
				float real = channels[i].real[x];

				// The scaled vector value
				if (channels[i].displayAverage == 1) {
					channels[i].values[x] += sampleToValue(i, sqrt((real * real) + (imag * imag)));
				} else {
					channels[i].values[x] = sampleToValue(i, sqrt((real * real) + (imag * imag)));			
				}
			}
			if (channels[i].displayAverage == 1) {
				channels[i].sampleCount++;
			} else {
				channels[i].sampleCount = 1;
			}
		}
	}

}

void displayFrequencyDomain() {

	int16_t segment = fb.getWidth() / 11;
	int16_t xmargin = (fb.getWidth() - (segment * 10)) / 2;
	int16_t ymargin = (fb.getHeight() - (segment * 8)) / 2;

	fb.fillScreen(Color::Black);
	drawGridLogLin();
	uint32_t offset = 0;
	fb.setCursor(0, 0);
	fb.setTextColor(TextFG);


	uint8_t line = 8;
	for (uint8_t i = 0; i < MAXCHAN; i++) {
		if (channels[i].enabled) {
			drawFrequencyDomain(i, Chan1+i);
		}
	}
}

void drawGridLogLin() {
	int16_t ysegment = fb.getHeight() / 9;
	int16_t xsegment = (fb.getWidth() - (ysegment)) / decades;
	int16_t xmargin = ysegment/2;
	int16_t ymargin = ysegment / 2;

	for (uint16_t x = 0; x < decades; x++) {
		for (uint8_t l = 1; l < 10; l++) {
			float f = log10(l) * (float)xsegment;
			fb.drawLine(xmargin + x * xsegment + f, ymargin, xmargin + x * xsegment + f, ymargin + ysegment * 8, GridBack);
		}
		fb.drawLine(xmargin + x * xsegment, ymargin, xmargin + x * xsegment, ymargin + ysegment * 8, GridFore);
	}
	fb.drawLine(xmargin + decades * xsegment, ymargin, xmargin + decades * xsegment, ymargin + ysegment * 8, GridFore);

	for (uint16_t y = 0; y < 9; y++) {
		fb.drawLine(xmargin, ymargin + y * ysegment, xmargin + decades * xsegment, ymargin + y * ysegment, GridBack);
	}
	fb.drawLine(xmargin, ymargin + 0 * ysegment, xmargin + decades * xsegment, ymargin + 0 * ysegment, GridFore);
	fb.drawLine(xmargin, ymargin + 8 * ysegment, xmargin + decades * xsegment, ymargin + 8 * ysegment, GridFore);
}

void sampleFrequencyDomain(float f) {
	sampleStartTime = millis();
	sampleNumber = 0;
	sampleSize = SAMPLEPOINTS;
	if (configureADC(f)) {
		AD1CON1bits.ON = 1;
	}
}

void drawFrequencyDomain(uint8_t c, uint16_t color) {
	int16_t ysegment = fb.getHeight() / 9;
	int16_t xsegment = (fb.getWidth() - (ysegment)) / decades;
	int16_t xmargin = ysegment/2;
	int16_t ymargin = ysegment / 2;

	int16_t swidth = xsegment * 5;
	int16_t sbottom = ymargin + (ysegment * 8);

	float bucketsize = (float)sampleFrequency(decades) / (float)(SAMPLEPOINTS/2.0);
	float sc = channels[c].sampleCount;
	fb.setTextColor(color, color);
	fb.setCursor(1, 3);
	fb.setFont(Fonts::Default);
	if (channels[c].displayAverage == 1) {
		fb.print("Averaged Samples: ");
		fb.print((int)sc);
		fb.print(" ");
	}
	fb.print("Peak: ");
	float pv = 0;
	float pf = 0;
	for (uint16_t x = 1; x < SAMPLEPOINTS/2; x++) {
		float posa = 0;

		float freqa = (x-1) * bucketsize;
		float loga = log10(freqa);
		posa = (float)xsegment * loga;

		if (posa < 0) posa = 0;
		float freqb = (x) * bucketsize;
		float logb = log10(freqb);
		float posb = (float)xsegment * logb;

		if (posa >= 0 && posb >= 0) {
			float mul = (ysegment / voltages[channels[c].selectedVoltage].value);
			float va = (channels[c].values[x-1] / sc);
			float vb = (channels[c].values[x] / sc);

			if (channels[c].subtract == 1) {
				va = va - stored[x-1];
				vb = vb - stored[x];
			}
			va = va * mul;
			vb = vb * mul;
			
			if (va < 1) va = 1;
			if (vb < 1) vb = 1;

			va = 20.0 * log(va);
			vb = 20.0 * log(vb);

			if (va < 0) va = 0 - va;
			if (vb < 0) vb = 0 - vb;

			if (va > pv) {
				pv = va;
				pf = freqa;
			}
			if (vb > pv) {
				pv = vb;
				pf = freqb;
			}
			
		//	va = va * mul;
		//	vb = vb * mul;

			fb.drawLine(xmargin + posa, sbottom - va, xmargin + posb, sbottom - vb, color);
		}
	}
	printFloatUnits(pf);
	fb.print("Hz");
	if (rightMode == 1) {
		float posa = 0;

		float freqa = (cursor) * bucketsize;
		float loga = log10(freqa);
		posa = (float)xsegment * loga;

		if (posa < 0) posa = 0;
		float mul = (ysegment / voltages[channels[0].selectedVoltage].value);
		float va = (channels[0].values[cursor] / sc);

		if (channels[0].subtract == 1) {
			va = va - stored[cursor];
		}
		va = va * mul;
			
		if (va < 1) va = 1;

		va = 20.0 * log(va);

		if (va < 0) va = 0 - va;

		fb.drawLine(xmargin + posa, sbottom - va, xmargin + posa - 4, sbottom - va - 4, Information);
		fb.drawLine(xmargin + posa, sbottom - va, xmargin + posa + 4, sbottom - va - 4, Information);
		fb.setCursor(xmargin+2, ymargin+2);
		fb.setTextColor(Information, Information);
		fb.print("Cursor: ");
		printFloatUnits(freqa);
		fb.print("Hz");
	}
}