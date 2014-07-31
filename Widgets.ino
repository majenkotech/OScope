
boolean button(int16_t x, int16_t y, int16_t w, int16_t h, boolean sel, char *text) {
	int16_t tpx = tp.y();
	int16_t tpy = fb.getHeight() - tp.x();

	boolean hover = (
		tpx >= x && tpx < (x + w) &&
		tpy >= y && tpy < (y + h)
	);
	
	uint16_t fg = MenuFG;
	uint16_t bg = MenuBG;

	if (sel) {
		fg = MenuBG;
		bg = MenuFG;
	}

	if (hover) {
		bg = MenuHI;
	}
	
	fb.fillRoundRect(x, y, w, h, 8, bg);
	fb.drawRoundRect(x, y, w, h, 8, fg);

	fb.setFont(Fonts::Topaz);
	fb.setTextColor(fg, fg);
	uint16_t tw = fb.stringWidth(text);
	fb.setCursor(x + (w/2) - (tw/2), y + (h/2) - 4);
	fb.print(text);
	return hover;
}

void hbar(int16_t x, int16_t y, int16_t w, int16_t h, float mval, float cval, uint16_t color) {
	fb.drawRectangle(x, y, w, h, color);
	float pct = cval / mval * (float)(w - 4);
	fb.fillRect(x+2, y+2, pct, h-4, color);
}
