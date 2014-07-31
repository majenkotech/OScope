
void calculateTimeDomain() {
	// If sampling is currently running then just return
	if (sampleRunning()) {
		return;
	}

	// Save the current sample set and convert it to values values all in one loop
	for (int chan = 0; chan < MAXCHAN; chan++) {
		if (channels[chan].enabled) {
			for (int x = 0; x < sampleSize; x++) {
				channels[chan].values[x] = sampleToValue(chan, channels[chan].data[x]);
			}
		}
	}

	// Start a new sampling while we draw this one
	sampleTimeDomain();
}

void displayTimeDomain() {
	if (millis() - sampleStartTime > 1000) {
		// Save the current sample set and convert it to values values all in one loop
		for (int chan = 0; chan < MAXCHAN; chan++) {
			if (channels[chan].enabled) {
				for (int x = 0; x < sampleSize; x++) {
					channels[chan].values[x] = sampleToValue(chan, channels[chan].data[x]);
				}
			}
		}
	}
	// We want a nice pretty square grid
	drawGridLinLin();

	// Now let's work out our trigger point
	char trigType = 'N';
	uint32_t offset = 0;
	
	if ((offset = findTriggerOffsetAverageCrossing(triggerChannel)) != 0) {
		trigType = 'A';
	} else if((offset = findTriggerOffsetZeroCrossing(triggerChannel)) != 0) {
		trigType = 'Z';
	} else if ((offset = findTriggerOffsetRising(triggerChannel)) != 0) {
		trigType = 'R';
	} else {
		trigType = 'N';
	}
	fb.setFont(Fonts::Default);

	// Display the timing data
	fb.setCursor(5, 3);
	fb.setTextColor(TextFG, 0);
	fb.print(timeSelections[selectedTimebase].name);
	fb.print("/div Trigger: ");
	fb.print(trigType);

	// Now for each channel.
	uint8_t line = 11; // This is the Y position for printing
	for (uint8_t chan = 0; chan < MAXCHAN; chan++) {
		if (channels[chan].enabled) {

			// Calculate interesting values
			float maxv = -999999;
			float minv = 999999;
			float avev = 0;
			for (uint32_t x = offset; x < offset+(segment*10); x++) {
				float sval = channels[chan].values[x];
				if (sval > maxv) {
					maxv = sval;
				} else if (sval < minv) {
					minv = sval;
				}
				avev += sval;
			}
			avev = avev / (segment*10);

			// ... and display them			
			fb.setTextColor(Chan1 + chan, 0);
			fb.setCursor(5, line);
			fb.print(voltages[channels[chan].selectedVoltage].name);
			fb.print("/div ");
			printFloatUnits(maxv - minv);
			fb.print("Vp-p ");
			printFloatUnits(avev);
			fb.print("Vave ");
			line += 8;

			// Remove the DC offset if we're AC coupled
			if (channels[chan].acCoupled) {
				for (uint16_t x = 0; x < sampleSize; x++) {
					channels[chan].values[x] -= avev;
				}
			}

			// Optionally display the average voltage line
			if (channels[chan].displayAverage) {
				float sp = avev * (segment / voltages[channels[chan].selectedVoltage].value);
				for (uint16_t x = 0; x <= segment * 10; x+=2) {
					fb.setPixel(xmargin + x, ymargin + 4 * segment - sp, Chan1 + chan);
				}
			}

			// And finally draw the line itself.
			drawTimeDomain(chan, offset, channels[chan].acCoupled ? channels[chan].referenceVoltage : 0, Chan1 + chan);
		}
	}
	if (rightMode == 1) {
		uint32_t x = cursor;
		int16_t vpos = channels[0].acCoupled ? channels[0].referenceVoltage : 0;
		float sp = channels[0].values[offset+x] * (segment / voltages[channels[0].selectedVoltage].value);
		fb.drawLine(xmargin + x, vpos + ymargin + 4 * segment - sp, xmargin + x - 4, vpos + ymargin + 4 * segment - sp - 4, Information);
		fb.drawLine(xmargin + x, vpos + ymargin + 4 * segment - sp, xmargin + x + 4, vpos + ymargin + 4 * segment - sp - 4, Information);
		fb.setCursor(5, line);
		
		fb.setTextColor(Information, Information);
		fb.print("Cursor: ");
		printFloatUnits(channels[0].values[offset+x]);
		fb.print("V");
	}
}

void drawGridLinLin() {
	fb.fillScreen(Color::Black);

	for (uint16_t x = 0; x < 11; x++) {
		fb.drawLine(xmargin + x * segment, ymargin, xmargin + x * segment, ymargin + segment * 8, GridBack);
	}

	for (uint16_t y = 0; y < 9; y++) {
		fb.drawLine(xmargin, ymargin + y * segment, xmargin + 10 * segment, ymargin + y * segment, GridBack);
	}

	fb.drawLine(xmargin + 5 * segment, ymargin, xmargin + 5 * segment, ymargin + 8 * segment, GridFore);
	fb.drawLine(xmargin, ymargin + 4 * segment, xmargin + 10 * segment, ymargin + 4 * segment, GridFore);
}

void sampleTimeDomain() {
	sampleStartTime = millis();
	float spsamp = timeSelections[selectedTimebase].value / (float)segment;
	float f = 1.0/spsamp;
	sampleSize = TIMESAMPLES;
	sampleNumber = 0;
	if (configureADC(f)) {
		AD1CON1bits.ON = 1;
	}
}

void drawTimeDomain(uint8_t c, uint32_t hoffset, int16_t vpos, uint16_t color) {
	float lsp = -99999999;
	for (uint16_t x = 0; x <= segment * 10; x++) {
		float sp = 0;
		sp = channels[c].values[hoffset+x] * (segment / voltages[channels[c].selectedVoltage].value);
		if (lsp != -99999999) {
			fb.drawLine(xmargin + x - 1, vpos + ymargin + 4 * segment - lsp, xmargin + x, vpos + ymargin + 4 * segment - sp, color);
		}
		lsp = sp;
	}
}
