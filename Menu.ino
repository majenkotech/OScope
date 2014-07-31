
enum {
	MenuHidden,
	MenuRoot,
	MenuDomain,
	MenuChannels,
	MenuTimebase,
	MenuUtil,
	MenuEditChannel,
	MenuBrightness
};

uint8_t menu_page = MenuHidden;
uint8_t selectedChannel = 0;

void menu() {
	tp.sample();

	switch (menu_page) {
		case MenuHidden:
			displayOpenButton();
			break;
		case MenuRoot:
			displayRootMenu();
			break;
		case MenuDomain:
			displayDomainMenu();
			break;
		case MenuTimebase:
			displayTimebaseMenu();
			break;
		case MenuUtil:
			displayUtilMenu();
			break;
		case MenuChannels:
			displayChannelsMenu();
			break;
		case MenuEditChannel:
			displayEditChannel(selectedChannel);
			break;
		case MenuBrightness:
			displayBrightnessMenu();
			break;
	}
}

void displayEditChannel(uint8_t c) {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	
	char temp[50];

	static boolean plusState = false;
	static boolean minusState = false;
	static boolean acState = false;
	static boolean averageState = false;
	static boolean enabledState = false;
	static boolean saveState = false;
	static boolean subState = false;

	sprintf(temp, "  %s/div  ", voltages[channels[c].selectedVoltage].name);
	uint16_t tw = fb.stringWidth(temp);

	fb.setFont(Fonts::Topaz);
	fb.setTextColor(MenuFG, 0);
	fb.setCursor(cx - (tw/2), sy * 2 + (sy / 2) - 4);
	fb.print(temp);

	boolean minus = button(sx * 2, sy * 2, sx, sy, false, "-");
	boolean plus = button(cx + sx * 2, sy * 2, sx, sy, false, "+");

	if (minusState != minus) {
		minusState = minus;
		if (!minus) {
			if (channels[c].selectedVoltage != 0) {
				channels[c].selectedVoltage--;
			}
			setGain(c, voltages[channels[c].selectedVoltage].extra);
		}
	}

	if (plusState != plus) {
		plusState = plus;
		if (!plus) {
			if (voltages[channels[c].selectedVoltage+1].name != 0) {
				channels[c].selectedVoltage++;
			}
			setGain(c, voltages[channels[c].selectedVoltage].extra);
		}
	}

	boolean enabled = button(sx * 2 , sy * 4, sx + (sx/2), sy, channels[c].enabled, (char *)(channels[c].enabled ? "ON" : "OFF"));
	boolean ac = button(sx * 4 + (sx/4) , sy * 4, sx + (sx/2), sy, channels[c].acCoupled, (char *)(channels[c].acCoupled ? "AC" : "DC"));
	boolean average = button(sx * 6 + (sx/2), sy * 4, sx + (sx/2), sy, channels[c].displayAverage, "AVE");
	boolean save = button(sx * 2 , cy + sy * 2, sx + (sx/2), sy, false, "SAVE");
	boolean sub = button(sx * 4 + (sx/4) , cy + sy * 2, sx + (sx/2), sy, channels[c].subtract, "SUB");


	if (enabled != enabledState) {
		enabledState = enabled;
		if (!enabled) {
			channels[c].enabled = 1 - channels[c].enabled;
		}
	}

	if (ac != acState) {
		acState = ac;
		if (!ac) {
			channels[c].acCoupled = 1 - channels[c].acCoupled;
		}
	}

	if (average != averageState) {
		averageState = average;
		if (!average) {
			channels[c].displayAverage = 1 - channels[c].displayAverage;
		}
	}

	if (save != saveState) {
		saveState = save;
		if (!save) {
			fb.setCursor(0, 0);
			for (uint32_t x = 0; x < SAMPLEPOINTS; x++) {
				stored[x] = channels[c].values[x] / (float)channels[c].sampleCount;
				fb.print(channels[c].values[x]); fb.print(" ");
			}
			fb.print("Saved");
		}
	}

	if (sub != subState) {
		subState = sub;
		if (!sub) {
			channels[c].subtract = 1 - channels[c].subtract;
		}
	}

	
	static boolean closeState = false;
	boolean close = button(cx + sx, cy + sy*2, sx * 3, sy, false, "BACK");
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuChannels;
		}
	}
}

void displayChannelsMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	

	char temp[5];
	
	static boolean chanStates[MAXCHAN];
	boolean chans[MAXCHAN];

	for (uint8_t chan = 0; chan < MAXCHAN; chan++) {
		sprintf(temp, "%d", chan+1);
		chans[chan] = button((sx / 2) + sx + (sx * ((chan % 4) * 2)), sy + (sy * (chan / 4) * 2), sx, sy, channels[chan].enabled, temp);
		if (chans[chan] != chanStates[chan]) {
			chanStates[chan] = chans[chan];
			if (!chans[chan]) {
				menu_page = MenuEditChannel;
				selectedChannel = chan;
			}
		}
	}

	static boolean closeState = false;
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuRoot;
		}
	}
}

void displayTimebaseMenu() {
	switch (domain) {
		case Time:
			displayTimeSelectionMenu();
			break;
		case Frequency:
			displayFrequencySelectionMenu();
			break;
	}
}

void displayUtilMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	

	static boolean upgradeState = false;
	boolean upgrade = button(sx, sy, sx * 3, sy * 2, false, "UPGRADE");
	if (upgrade != upgradeState) {
		upgradeState = upgrade;
		if (!upgrade) {
			fb.fillScreen(0);
			fb.setFont(Fonts::Topaz);
			fb.setTextColor(Information, 0);
			fb.setCursor(70, 100);
			fb.print("Upload Firmware Now...");
			fb.update(&tft);
			delay(1000);
			bootloader();
		}
	}

	static boolean rbtState = false;
	boolean rbt = button(cx + sx, sy, sx * 3, sy * 2, false, "REBOOT");
	if (rbt != rbtState) {
		rbtState = rbt;
		if (!rbt) {
			reset();
		}
	}

	static boolean brightState = false;
	boolean bright = button(sx, cy + sy, sx * 3, sy * 2, false, "BRIGHTNESS");
	if (bright != brightState) {
		brightState = bright;
		if (!bright) {
			menu_page = MenuBrightness;
		}
	}

	static boolean closeState = false;
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuRoot;
		}
	}

}

void displayTimeSelectionMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	
	char temp[50];

	static boolean plusState = false;
	static boolean minusState = false;
	static boolean closeState = false;

	fb.setFont(Fonts::Topaz);

	sprintf(temp, "  %s/div  ", timeSelections[selectedTimebase].name);
	uint16_t tw = fb.stringWidth(temp);

	fb.setTextColor(MenuFG, 0);
	fb.setCursor(cx - (tw/2), sy * 2 + (sy / 2) - 4);
	fb.print(temp);

	boolean minus = button(sx * 2, sy * 2, sx, sy, false, "-");
	boolean plus = button(cx + sx * 2, sy * 2, sx, sy, false, "+");
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");

	if (minusState != minus) {
		minusState = minus;
		if (!minus) {
			AD1CON1bits.ON = 0;
			if (selectedTimebase == 0) {
				while (timeSelections[selectedTimebase+1].name != 0) {
					selectedTimebase++;
				}
			} else {
				selectedTimebase--;
			}
		}
	}

	if (plusState != plus) {
		plusState = plus;
		if (!plus) {
			AD1CON1bits.ON = 0;
			selectedTimebase++;
			if (timeSelections[selectedTimebase].name == 0) {
				selectedTimebase = 0;
			}
		}
	}
	
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuRoot;
		}
	}
}

void displayFrequencySelectionMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	
	char temp[50];

	static boolean plusState = false;
	static boolean minusState = false;
	static boolean closeState = false;

	fb.setFont(Fonts::Topaz);

	sprintf(temp, "  %dHz  ", sampleFrequency(decades));
	uint16_t tw = fb.stringWidth(temp);

	fb.setTextColor(MenuFG, 0);
	fb.setCursor(cx - (tw/2), sy * 2 + (sy / 2) - 4);
	fb.print(temp);

	boolean minus = button(sx * 2, sy * 2, sx, sy, false, "-");
	boolean plus = button(cx + sx * 2, sy * 2, sx, sy, false, "+");
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");

	if (minusState != minus) {
		minusState = minus;
		if (!minus) {
			if (decades > 1) {
				AD1CON1bits.ON = 0;
				decades--;
			}
			for (uint8_t c = 0; c < MAXCHAN; c++) {
				for (uint32_t x = 0; x < SAMPLEPOINTS; x++) {
					channels[c].values[x] = 0;
				}
				channels[c].sampleCount = 0;
			}
		}
	}

	if (plusState != plus) {
		plusState = plus;
		if (!plus) {
			if (decades < 5) {
				AD1CON1bits.ON = 0;
				decades++;
			}
			for (uint8_t c = 0; c < MAXCHAN; c++) {
				for (uint32_t x = 0; x < SAMPLEPOINTS; x++) {
					channels[c].values[x] = 0;
				}
				channels[c].sampleCount = 0;
			}
		}
	}
	
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuRoot;
		}
	}

}

void displayDomainMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	

	static boolean timeState = false;
	static boolean freqState = false;
	static boolean closeState = false;
		
	boolean time = button(sx, sy, sx * 3, sy * 2, domain == Time, "TIME");
	boolean freq = button(cx + sx, sy, sx * 3, sy * 2, domain == Frequency, "FREQUENCY");
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");

	if (time != timeState) {
		timeState = time;
		if (!time) {
			domain = Time;
		}
	}

	if (freq != freqState) {
		freqState = freq;
		if (!freq) {
			domain = Frequency;
		}
	}

	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuRoot;
		}
	}
}

void displayRootMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;

	static boolean domainState = false;
	static boolean channelsState = false;
	static boolean timebaseState = false;
	static boolean closeState = false;
	static boolean utilState = false;
		
	boolean domainb = button(sx, sy, sx * 3, sy * 2, false, "DOMAIN");
	boolean channels = button(cx + sx, sy, sx * 3, sy * 2, false, "CHANNELS");
	boolean timebase = button(sx, cy + sy, sx * 3, sy * 2, false, "TIMEBASE");
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "CLOSE");
	boolean util = button(cx - sx, cy - sy/2, sx * 2, sy, false, "UTIL");

	if (domainb != domainState) {
		domainState = domainb;
		if (!domainb) {
			menu_page = MenuDomain;
		}
	}

	if (channels != channelsState) {
		channelsState = channels;
		if (!channels) {
			menu_page = MenuChannels;
		}
	}

	if (timebase != timebaseState) {
		timebaseState = timebase;
		if (!timebase) {
			menu_page = MenuTimebase;
		}
	}

	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuHidden;
		}
	}

	if (util != utilState) {
		utilState = util;
		if (!util) {
			menu_page = MenuUtil;
		}
	}

}

void displayOpenButton() {
	static boolean lastState = false;
	boolean newState = button(fb.getWidth() - 60, 3, 60, 40, false, "MENU");
	if (newState != lastState) {
		lastState = newState;
		if (!newState) {
			menu_page = MenuRoot;
		}
	}
}

void displayBrightnessMenu() {
	uint16_t dw = fb.getWidth();
	uint16_t dh = fb.getHeight();
	uint16_t cx = dw / 2;
	uint16_t cy = dh / 2;
	uint16_t sx = cx / 5;
	uint16_t sy = cy / 4;	
	static boolean closeState = false;
	boolean close = button(cx + sx, cy + sy, sx * 3, sy * 2, false, "BACK");
	if (close != closeState) {
		closeState = close;
		if (!close) {
			menu_page = MenuUtil;
		}
	}

		char temp[50];

	static boolean plusState = false;
	static boolean minusState = false;
	
	fb.setFont(Fonts::Topaz);

	sprintf(temp, "  %d  ", brightness);
	uint16_t tw = fb.stringWidth(temp);

	fb.setTextColor(MenuFG, 0);
	fb.setCursor(cx - (tw/2), sy * 2 + (sy / 2) - 4);
	fb.print(temp);

	boolean minus = button(sx * 2, sy * 2, sx, sy, false, "-");
	boolean plus = button(cx + sx * 2, sy * 2, sx, sy, false, "+");
	

	if (minusState != minus) {
		minusState = minus;
		if (!minus) {
			if (brightness > 0) {
				brightness--;
			}
			analogWrite(PIN_OC3, brightness * 25 + 5);
		}
	}

	if (plusState != plus) {
		plusState = plus;
		if (!plus) {
			if (brightness < 10) {
				brightness++;
			}
			analogWrite(PIN_OC3, brightness * 25 + 5);
		}
	}
}

