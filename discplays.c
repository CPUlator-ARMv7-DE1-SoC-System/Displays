#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/*-----------------------------------------------------------------\\
				   CONTROLS - SWITCHES & BUTTONS
\\-----------------------------------------------------------------*/
	
uint32_t getCurrentSwitches(){
	volatile uint32_t *switches = (volatile uint32_t *) 0xFF200040;
	return *switches;
}

uint32_t getCurrentButtons(){
	volatile uint32_t *buttons = (volatile uint32_t *) 0xFF200050;
	return *buttons;
}

void delay(volatile int count) {
    while (count > 0) {
        count--;
    }
}

/*-----------------------------------------------------------------\\
						 PIXEL BUFFER (SRAM)
				1 Pixel = RED_GREEN_BLUE_? = 16 bit
\\-----------------------------------------------------------------*/

#define PIXEL_BUFFER ((uintptr_t) 0xC8000000)
#define PIXEL_BUFFER_WIDTH ((int16_t) 320)
#define PIXEL_BUFFER_HEIGHT ((int16_t) 240)
#define PIXEL_BUFFER_GRID_SIZE ((uint8_t) 16)
#define PI ((double) 3.141592654)
#define EPSILON 160

#define RADIUS ((int16_t) 40)
#define PERIOD ((char) 32)
#define SEMI_PERIOD ((char) PERIOD/2)
	
volatile uint16_t *getPixel(int32_t x, int32_t y){
    uintptr_t address = PIXEL_BUFFER + ((uintptr_t)y << 10) + ((uintptr_t)x << 1);
    return (volatile uint16_t *)address;
}

void putPixel(int32_t x, int32_t y, uint16_t color_value){
    if (x < 0 || x >= PIXEL_BUFFER_WIDTH || y < 0 || y >= PIXEL_BUFFER_HEIGHT){
        return;
    }

    *getPixel(x, y) = color_value;
}

void draw (void (*function)(int16_t, int16_t, volatile uint16_t*)){
	for (int y = 0; y < PIXEL_BUFFER_HEIGHT; y++){
        for (int x = 0; x < PIXEL_BUFFER_WIDTH; x++){
			volatile uint16_t *pixel = getPixel(x,y);
            function(x, y, pixel);
        }
    }
}

void animate (void (*function)(int16_t, int16_t, volatile uint16_t*, uint32_t), uint32_t tick_count, uint32_t delta_time){
	for (int y = 0; y < PIXEL_BUFFER_HEIGHT; y++){
        for (int x = 0; x < PIXEL_BUFFER_WIDTH; x++){
			volatile uint16_t *pixel = getPixel(x,y);
            function(x, y, pixel, tick_count);
        }
    }
	delay(delta_time);
}
	
void clear (int16_t x, int16_t y, volatile uint16_t *pixel){
	if(*pixel != 0x0000) *pixel = 0x0000;
}

void grid (int16_t x, int16_t y, volatile uint16_t *pixel){
	if (y % PIXEL_BUFFER_GRID_SIZE == 0 || x % PIXEL_BUFFER_GRID_SIZE == 0){
		*pixel = 0xAAAF;
	} else {
		*pixel = 0x3330;	
	}
}

static int32_t animatedRadius(int32_t maxRadius, uint16_t phase){
    phase %= PERIOD;

    if (phase <= SEMI_PERIOD){
        return (maxRadius - ((maxRadius - 1) * phase / SEMI_PERIOD));
    }
    return 1 + ((maxRadius - 1) * (phase - SEMI_PERIOD) / SEMI_PERIOD);
}


void drawBresLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color){
    int32_t dx, dy, p, x, y, sx, sy;

    dx = abs(x1 - x0);
    dy = abs(y1 - y0);

    x = x0;
    y = y0;

    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;

    p = dx - dy;

    while (x != x1 || y != y1){
        putPixel(x, y, color);

        int32_t p2 = 2 * p;

        if (p2 > -dy){
            p -= dy;
            x += sx;
        }

        if (p2 < dx){
            p += dx;
            y += sy;
        }
    }

    putPixel(x1, y1, color);
}


void centeredEllipse(int16_t x, int16_t y, volatile uint16_t *pixel, uint32_t tick_count){
    int32_t dx = abs(x - PIXEL_BUFFER_WIDTH / 2);
    int32_t dy = abs(y - PIXEL_BUFFER_HEIGHT / 2);

    int32_t a = animatedRadius(RADIUS, tick_count);
    int32_t b = RADIUS;

    int32_t aSquared = a * a;
    int32_t bSquared = b * b;
	
	/*
			x^2/a^2 + y^2/b^2 = 1
		<=> x^2*b^2 + y^2*a^2 = a^2*b^2
	*/
    int32_t value = dx * dx * bSquared + dy * dy * aSquared;
    int32_t target = aSquared * bSquared;

    if (abs(value - target) <= EPSILON * aSquared){
        *pixel = 0xFFFF;
    }
}

int16_t cosTable[PERIOD] = {
     1024,  1004,   946,   852,
      724,   568,   392,   200,
        0,  -200,  -392,  -568,
     -724,  -852,  -946, -1004,
    -1024, -1004,  -946,  -852,
     -724,  -568,  -392,  -200,
        0,   200,   392,   568,
      724,   852,   946,  1004
};

int16_t sinTable[PERIOD] = {
        0,   200,   392,   568,
      724,   852,   946,  1004,
     1024,  1004,   946,   852,
      724,   568,   392,   200,
        0,  -200,  -392,  -568,
     -724,  -852,  -946, -1004,
    -1024, -1004,  -946,  -852,
     -724,  -568,  -392,  -200
};


static inline void drawEllipsePoints(int32_t xc, int32_t yc, int32_t x, int32_t y, uint16_t color, int32_t cosA, int32_t sinA, bool isFirstPoint) {
    
	int32_t rx1 = (x * cosA - y * sinA) >> 10;
    int32_t ry1 = (x * sinA + y * cosA) >> 10;

    int32_t rx2 = (-x * cosA - y * sinA) >> 10;
    int32_t ry2 = (-x * sinA + y * cosA) >> 10;

    int32_t rx3 = (x * cosA + y * sinA) >> 10;
    int32_t ry3 = (x * sinA - y * cosA) >> 10;

    int32_t rx4 = (-x * cosA + y * sinA) >> 10;
    int32_t ry4 = (-x * sinA - y * cosA) >> 10;
	
    putPixel(xc + rx1, yc + ry1, color);
    putPixel(xc + rx2, yc + ry2, color);
    putPixel(xc + rx3, yc + ry3, color);
    putPixel(xc + rx4, yc + ry4, color);
	
	if (x == 0) {
		drawBresLine(xc, yc, xc + rx1, yc + ry1, color);
		drawBresLine(xc, yc, xc + rx4, yc + ry4, color);
	}

	if (y == 0) {
		drawBresLine(xc, yc, xc + rx2, yc + ry2, color);
		drawBresLine(xc, yc, xc + rx3, yc + ry3, color);
	}
}
	

void ellipseBres(int32_t xc, int32_t yc, int32_t rx, int32_t ry, uint16_t phase, uint16_t color, uint8_t angle){
	rx = animatedRadius(rx, phase);
	
    int32_t rx2 = (uint32_t) rx * rx;
    int32_t ry2 = (uint32_t) ry * ry;
	
	int32_t cosA = cosTable[angle];
	int32_t sinA = sinTable[angle];

    int32_t x = 0;
    int32_t y = ry;

    int32_t dx = 0;
    int32_t dy = 2 * rx2 * y;

    int32_t p = ry2 - rx2 * ry + rx2;
	

    /* REGION 1 */
    while (dx < dy){
        drawEllipsePoints(xc, yc, x, y, color, cosA, sinA, true);
		
        x++;
        dx += 2 * ry2;

        if (p < 0){
            p += dx + ry2;
        }else{
            y--;
            dy -= 2 * rx2;
            p += dx - dy + ry2;
        }
    }

    /* REGION 2 */
    p = ry2 * (x + 1) * (x + 1) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;

    while (y >= 0){
        drawEllipsePoints(xc, yc, x, y, color,cosA, sinA, true);

        y--;
        dy -= 2 * rx2;

        if (p > 0){
            p += rx2 - dy;
        }else{
            x++;
            dx += 2 * ry2;
            p += dx - dy + rx2;
        }
    }
}

uint16_t animateMultEllipsesBres(uint32_t tick_count, uint32_t delta_time, uint8_t angle, uint8_t relative_angle){
	uint32_t phase0 = (tick_count % PERIOD); 
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0, 0x00FF, angle);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, (2*RADIUS), (2*RADIUS), phase0, 0xF00F, relative_angle);
	delay(delta_time);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0,0x0000, angle);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, (2*RADIUS), (2*RADIUS), phase0, 0x0000, relative_angle);
	tick_count++;
	return tick_count;
}

uint16_t animateMultTraceEllipsesBres(uint32_t tick_count, uint32_t delta_time, uint8_t angle, uint8_t relative_angle){
	uint32_t phase0 = (tick_count % PERIOD); 
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0, 0x00FF, angle++);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, (2*RADIUS), (2*RADIUS), phase0, 0xF00F, relative_angle++);
	delay(delta_time);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0,0x0000, angle++);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, (2*RADIUS), (2*RADIUS), phase0, 0x0000, relative_angle++);
	tick_count++;
	return tick_count;
}

uint16_t animateEllipseBres(uint32_t tick_count, uint32_t delta_time, uint8_t angle){
	uint32_t phase0 = (tick_count % PERIOD); 
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0, 0xFFFF, angle);
	delay(delta_time);
	ellipseBres(PIXEL_BUFFER_WIDTH/2, PIXEL_BUFFER_HEIGHT/2, RADIUS, RADIUS, phase0,0x0000, angle);
	tick_count++;
	return tick_count;
}

#define AVAILABLE_SWITCHES ((uint16_t) 2)

int main(void)
{
	uint32_t delta_time = 100000;
	uint32_t current_switches = 0;
	uint32_t max_switch_value = (uint32_t) (pow(2, AVAILABLE_SWITCHES) - 1);
	
	uint16_t tick_count = 0;
	uint8_t relative_angle = 8;
	
	draw(clear);
	
	do {
		current_switches = getCurrentSwitches();
		
		if(current_switches > max_switch_value) return 1;
		if((current_switches & 1) && !(current_switches & 2)){
			switch(getCurrentButtons()){
				case 1:
					tick_count = (tick_count % PERIOD) + 1;
					animate(centeredEllipse, tick_count, delta_time);
					tick_count++;
					draw(clear);
					break;
				case 2:
					tick_count = animateEllipseBres(tick_count, delta_time, 16);
					break;
				case 4:
					tick_count = animateEllipseBres(tick_count, delta_time, 8);
					break;
				case 8:
					relative_angle++;
					relative_angle = relative_angle % PERIOD;
					tick_count = animateMultEllipsesBres(tick_count, delta_time, 16, relative_angle);
					break;
				case 9:
					relative_angle++;
					relative_angle = relative_angle % PERIOD;
					tick_count = animateMultTraceEllipsesBres(tick_count, delta_time, 16, relative_angle);
					break;
				default:
			}
		}
		
		if(!(current_switches & 1) && (current_switches & 2)){
			switch(getCurrentButtons()){
				case 1:
					tick_count = (tick_count % PERIOD) + 1;
					animate(centeredEllipse, tick_count, delta_time);
					tick_count ++;
					draw(grid);
					break;
				case 2:
					tick_count = animateEllipseBres(tick_count, delta_time, 16);
					break;
				case 4:
					tick_count = animateEllipseBres(tick_count, delta_time, 8);
					break;
				case 8:
					relative_angle++;
					relative_angle = relative_angle % PERIOD;
					tick_count = animateMultEllipsesBres(tick_count, delta_time, 16, relative_angle);
					break;
				case 9:
					relative_angle++;
					relative_angle = relative_angle % PERIOD;
					tick_count = animateMultTraceEllipsesBres(tick_count, delta_time, 16, relative_angle);
					break;
				default:
					draw(grid);
			}
		}
		
		if((current_switches == 0)){
			draw(clear);
		}
	} while(1);
	
	draw(clear);
    return 0;
}	