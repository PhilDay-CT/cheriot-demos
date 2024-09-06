// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdlib>
#include <string>
#include <debug.hh>
#include <thread.h>

#include "../sonata_lcd/lcd.hh"
#include "../provider/provider.h"

#include "../logos/CT_banner.h"

#define COLS 70
#define HEADER 30
#define LINES 11

using namespace sonata::lcd;
	
struct Line {
	Color fg;
	Color bg;
	char * line;
}; 

Line lines[LINES];
char Header[COLS+1];

int top = 0;
int count = 0;
int next = 0;

void _print(const char *line, const char *error, Color fg, Color bg, bool header)
{

	static auto lcd  = SonataLcd();
	static bool init = false;

	if (!init) {
		init = true;
	
		CHERI::with_interrupts_disabled([&]() {	
			auto logoRect1 = Rect::from_point_and_size({0, 0}, {48, 13});
			lcd.draw_image_rgb565(logoRect1, CT_Banner);
			
			lcd.draw_line({0,16}, {lcd.resolution().width, 16}, Color::Black);
		});

		for (int i=0; i<LINES; i++) {
			lines[i].line = (char *)malloc(COLS+1);
			lines[i].line[COLS] = 0;
		};
		Header[COLS]=0;
		
		// A bit of hack but it initialises things
		// nicely
		strncpy(lines[0].line, "Ready", 5);
		lines[0].fg = Color::Green;
		lines[0].bg = Color::White;
		next=1; 
	}
	
	if (header) {
		auto h = strlen(line);
		for (int i=0; i<COLS; i++) {
			if (i < h) {
				Header[i] = line[i];
			} else {
				Header[i] = ' ';
			}
		}
		lcd.draw_str({65, 2}, Header, bg, fg, Font::Medium);
		return;
	}

	if (next == top) {
		// Scroll
		if (++top >= LINES)
			top = 0;
	}

	// Fill with spaces
	for (int i=0; i<COLS; i++) {
		lines[next].line[i] = ' ';
	}

	// Copy the line into the buffer
	lines[next].fg = fg;
	lines[next].bg = bg;
	auto lineS = strlen(line);
	strncpy(lines[next].line, line, std::min(lineS, (uint32_t)COLS));
	if (error != nullptr) {
		auto x = COLS-lineS-1;
		strncpy(&(lines[next].line[lineS+1]), error, std::min(strlen(error), x));
	}
	next++;
	if (next >= LINES) {
		next = 0;
	}

	uint32_t y = 18;
	int l = top;

	CHERI::with_interrupts_disabled([&]() {	
		for (int i=0; i<LINES; i++) {
			lcd.draw_str({2, y}, lines[l].line, lines[l].bg, lines[l].fg, Font::Small);
			l++;
			if (l >= LINES) {
				l = 0;
			}	
			if (l == next)
				break;
			y += 10;
		}
	});
}

namespace console {

void __cheri_compartment("console")
header(const char *header) {
	_print(header, nullptr, Color::Black, Color::White, true);
}

void __cheri_compartment("console")
print(const char * line) {
	_print(line, nullptr, Color::Black, Color::White,  false);
}


void __cheri_compartment("console")
error(const char *line, const char *error) {
	_print(line, error, Color::White, Color::Red, false);
}

}


