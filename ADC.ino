

boolean configureADC(float freq) {
	uint16_t numchans = 0;
	uint16_t enabledChannels = 0;
	for (uint8_t i = 0; i < MAXCHAN; i++) {
		if (channels[i].enabled) {
			enabledChannels |= 1<<i;
			numchans++;
		}
	}
	if (numchans == 0) {
		return false;
	}
	float f = freq * (float)numchans;
	uint32_t baseclock = F_CPU;
	uint8_t ps = 0;
	
	if (baseclock / f > 65535) {
		baseclock = F_CPU / 2;
		ps = 1;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 4;
		ps = 2;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 8;
		ps = 3;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 16;
		ps = 4;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 32;
		ps = 5;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 64;
		ps = 6;
	}

	if (baseclock / f > 65535) {
		baseclock = F_CPU / 256;
		ps = 7;
	}


	AD1CON1 = 0;
	AD1CON2 = 0;
	AD1CON3 = 0;
	AD1CHS  = 0;

	AD1CON1bits.FORM = 0b011;
	AD1CON1bits.SSRC = 0b010;
	AD1CON1bits.ASAM = 1;
	AD1CON2bits.CSCNA = 1;
	AD1CON2bits.SMPI = numchans-1;
	AD1CON3bits.SAMC = 0b00111;
	AD1CON3bits.ADCS = 0b00000111;
	
	AD1PCFG = ~enabledChannels;
	AD1CSSL = enabledChannels;

	IPC6bits.AD1IP = 6;
	IFS1bits.AD1IF = 0;
	IEC1bits.AD1IE = 1;

	T3CON = 0;
	T3CONbits.TCKPS = ps;
	PR3 = baseclock / f;
	T3CONSET = 1<<15;
//	AD1CON1SET = 1<<15;
	return true;
}


extern "C" {
	#include <sys/attribs.h>
	void __ISR(_ADC_VECTOR, IPL6) _ADCInterrupt() {
		float val;
		uint8_t cnum = 0;
		for (uint8_t i = 0; i < MAXCHAN; i++) {
			if (channels[i].enabled) {
				channels[i].data[sampleNumber] = (&ADC1BUF0)[cnum];
				cnum += 4;
			}
		}
		sampleNumber++;
		if (sampleNumber >= sampleSize) {
			AD1CON1bits.ON = 0;
		}
		IFS1bits.AD1IF = 0;
	}
}

boolean sampleRunning() {
	return (AD1CON1bits.ON == 1);
}

inline float sampleToValue(uint8_t chan, int16_t value) {
	return (float)value / 32767.0 * (channels[chan].referenceVoltage);	
}

void setGain(uint8_t chan, uint8_t gain) {
	if (chan == 0) {
		if (gain == 1) {
			digitalWrite(PIN_C2, LOW);
			digitalWrite(PIN_C3, HIGH);
		}
		if (gain == 10) {
			digitalWrite(PIN_C2, HIGH);
			digitalWrite(PIN_C3, LOW);
		}			
	}
}
