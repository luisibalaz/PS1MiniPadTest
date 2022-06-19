/* 
This is PS1MiniPadTest V0.4, a small sized Pad Tester
for the original Play Station and PSOne. Small enough
to fit inside a Memory Card and use it with FreePSXBoot.

Changes with respect to 0.3.1:
* Fixed slightly weird colors on the buttons 
* Better layout. Analog movement is now displayed on the
  [-127,128] range instead of [0,255]
* Code rewrite for cleanness and ease of reading
* Size has been reduced! Now 28KB instead of 32KB
* Added text at the bottom:
  PS1 Mini Pad Test V0.4
  By Luisibalaz & PSn00bSDK
 */
 
#include <sys/types.h>
#include <psxgpu.h>
#include <psxpad.h>
#include <psxapi.h>


#define OT_LEN 		8

// Geometry of the display

#define SCREEN_XRES	320
#define SCREEN_YRES	240

#define CENTER_X	SCREEN_XRES/2
#define CENTER_Y	SCREEN_YRES/2

#define FACE_CENTER_X	90
#define FACE_CENTER_Y	CENTER_Y/2-8

#define BTN_SHIFT 16

#define LR_CENTER_X	28
#define LR_CENTER_Y	28

#define PORT_OFFSET	(70+SCREEN_YRES/8)

// Colors
#define COLORBG_R	0x10
#define COLORBG_G	0x10
#define COLORBG_B	0x10

#define COLOR_PS_CROSS		0x9BADE4
#define COLOR_PS_SQUARE		0xD591BD
#define COLOR_PS_TRIANGLE	0x39DEC8
#define COLOR_PS_CIRCLE		0xF06E6C
#define COLOR_PS_NONE		0x333333
#define COLOR_PS_BG			0x2A2A2A
#define COLOR_PS_BUMPTRIG	0x5A5A5A
#define COLOR_PUSHED		0xFFFF00

// Button pushes
#define PAD_NONE	0xFFFF

// Display and drawing environments
DISPENV disp;
DRAWENV draw;

char	pribuff[2][65536];		// Primitive packet buffers
u_long	ot[2][OT_LEN];			// Ordering tables
char	*nextpri;				// Pointer to next packet buffer offset
int		db = 0;					// Double buffer index

// TIM image parameters for loading textures and drawing sprites
extern u_int buttons_tim[];
TIM_IMAGE buttons_image;

// Pad buffer
char pad_buff[2][34];

// Hold the ID's for the fonts displays
char font_id[5];

// Declare here to avoid errors in definition order
DR_TPAGE *tpage;

// Hold sprites infomation: Tpage, CLUT, coordinates, UV coordinates, color and associated button
typedef struct _SPRITE_INFO {
    u_short tpage;			// Tpage value
    u_short clut;			// CLUT value
	u_int x,y;				// Coordinates
    u_char u,v;				// UV offset
    char col_r,col_g,col_b;	// Color of the sprite
	int PAD;
} SPRITE_INFO;

// Struct to hold every necessary (up to current support) sprite
typedef struct _SPRT_ARRAY {
	SPRT_16 *up,			*right,		*down,		*left,
			*triangle,		*circle,	*cross,		*square,
			*bg_triangle,	*bg_circle,	*bg_cross,	*bg_square,
			*L1,			*L2,		*R1,		*R2,
			*select,		*start, 	*point_L,	*point_R;
	SPRT 	*bg_L,			*bg_R;
} SPRT_ARRAY;

// Struct to hold every necessary (up to current support) sprite information
typedef struct _SPRITE_INFO_ARRAY {
	SPRITE_INFO	up,			right,		down,		left,
				triangle,	circle,		cross,		square,
				bg_triangle,bg_circle,	bg_cross,	bg_square,
				L1,			L2,			R1,			R2,
				select,		start, 		point_L,	point_R,
				bg_L,		bg_R;
} SPRITE_INFO_ARRAY;

// Gets information from the Tpage and the arguments; then applies it to the SPRITE
void GetSprite(TIM_IMAGE *tim, SPRITE_INFO *sprite,int PAD, uint32_t color, int u, int v, int x, int y) {

    // Get tpage value
    sprite->tpage = getTPage(tim->mode, 0,
        tim->prect->x, tim->prect->y);
    
    // Get CLUT value
    if( tim->mode &0x8) {
        sprite->clut = getClut(tim->crect->x, tim->crect->y);
    }
	// Set coordinates
	sprite->x = x;
	sprite->y = y;

	// Button
	sprite->PAD = PAD;

    // Set UV offset
    sprite->u = u*16;
    sprite->v = v*16;

    // Set neutral color
    sprite->col_r = (color>>16)&0xFF;
    sprite->col_g = (color>>8)&0xFF;
    sprite->col_b = color&0xFF;
}

// Draws a fixed 16x16 sprite
char *SortSprite(u_int PORT, uint16_t btn, SPRT_16 *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE_INFO *sprite) {
    sprt = (SPRT_16*)pri;				// Initialize the sprite
    setSprt16(sprt);

    setXY0(sprt,sprite->x,sprite->y+PORT_OFFSET*PORT);	// Set position
    setUV0(sprt, sprite->u, sprite->v);					// Relative coordinate of sprite
    setRGB0(sprt,										// Set color
        (btn & sprite->PAD) ? sprite->col_r : 0xff,
        (btn & sprite->PAD) ? sprite->col_g : 0xff,
        (btn & sprite->PAD) ? sprite->col_b : 0x00);
    sprt->clut = sprite->clut;          // Set the CLUT value

    addPrim(ot, sprt);                  // Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

// Draws a fixed 32x32 sprite
char *SortSprite32(u_int PORT, uint16_t btn, SPRT *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE_INFO *sprite) {
    sprt = (SPRT*)pri;               	// Initialize the sprite
    setSprt(sprt);

    setXY0(sprt,sprite->x,sprite->y+PORT_OFFSET*PORT);	// Set position
    setUV0(sprt, sprite->u, sprite->v);					// Relative coordinate of sprite
	setWH(sprt, 32, 32);								// Set size
    setRGB0(sprt,                       				// Set color
        (btn&sprite->PAD) ? sprite->col_r : 0xff,
        (btn&sprite->PAD) ? sprite->col_g : 0xff,
        (btn&sprite->PAD) ? sprite->col_b : 0x00);
    sprt->clut = sprite->clut;			// Set the CLUT value

    addPrim(ot, sprt);                  // Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

// Draws a moving 16x16 sprite
char *SortSpriteMoving(uint8_t mov_x, uint8_t mov_y,u_int PORT, SPRT_16 *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE_INFO *sprite) {
    sprt = (SPRT_16*)pri;               // Initialize the sprite
    setSprt16(sprt);

	// Coordinate + movement (normalized from [0,255] to [-16,16])
    setXY0(sprt,sprite->x + (mov_x - 128>>3),sprite->y + (mov_y - 128>>3) + PORT_OFFSET*PORT);                   // Set position
    setUV0(sprt, sprite->u, sprite->v);
    setRGB0(sprt,				// Set color
        sprite->col_r,
        sprite->col_g,
        sprite->col_b);
    sprt->clut = sprite->clut;	// Set the CLUT value

    addPrim(ot, sprt);			// Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

// Draw all digital buttons with presses. This is to have code a little cleaner.
void DrawDigitalButtonPresses(int port, uint16_t btn, SPRT_ARRAY SPRTS, SPRITE_INFO_ARRAY SPRITES_INFO) {
	nextpri = SortSprite(port, btn, SPRTS.up,    tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.up);
	nextpri = SortSprite(port, btn, SPRTS.right, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.right);
	nextpri = SortSprite(port, btn, SPRTS.down,  tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.down);
	nextpri = SortSprite(port, btn, SPRTS.left,  tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.left);

	nextpri = SortSprite(port, btn, SPRTS.triangle, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.triangle);
	nextpri = SortSprite(port, btn, SPRTS.circle,   tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.circle);
	nextpri = SortSprite(port, btn, SPRTS.cross,    tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.cross);
	nextpri = SortSprite(port, btn, SPRTS.square,   tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.square);

	nextpri = SortSprite(port, btn, SPRTS.bg_triangle, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_triangle);
	nextpri = SortSprite(port, btn, SPRTS.bg_circle,   tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_circle);
	nextpri = SortSprite(port, btn, SPRTS.bg_cross,    tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_cross);
	nextpri = SortSprite(port, btn, SPRTS.bg_square,   tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_square);
	
	nextpri = SortSprite(port, btn, SPRTS.L1, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.L1);
	nextpri = SortSprite(port, btn, SPRTS.L2, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.L2);
	nextpri = SortSprite(port, btn, SPRTS.R1, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.R1);
	nextpri = SortSprite(port, btn, SPRTS.R2, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.R2);

	nextpri = SortSprite(port, btn, SPRTS.select, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.select);
	nextpri = SortSprite(port, btn, SPRTS.start,  tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.start);
}

// Draw all joystick movement data. This is to have code a little cleaner.
void DrawAnalogJoystickMovement(int port, uint16_t btn, uint8_t ls_x, uint8_t ls_y, uint8_t rs_x, uint8_t rs_y, SPRT_ARRAY SPRTS, SPRITE_INFO_ARRAY SPRITES_INFO) {
	nextpri = SortSpriteMoving(ls_x, ls_y, port, SPRTS.point_L, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.point_L);
	nextpri = SortSpriteMoving(rs_x, rs_y, port, SPRTS.point_R, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.point_R);
	nextpri = SortSprite32(port, btn, SPRTS.bg_L, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_L);
	nextpri = SortSprite32(port, btn, SPRTS.bg_R, tpage, ot[db] + (OT_LEN - 1), nextpri, &SPRITES_INFO.bg_R);

	// Print joystick values
	FntPrint(font_id[port+2],
		"X:%+4d            X:%+4d\n"
		"Y:%+4d            Y:%+4d",
		ls_x-128, rs_x-128, ls_y-128, rs_y-128);
}

// Initialize necessary things
void init() {
	
	// Reset GPU (also installs event handler for VSync)
	ResetGraph( 0 );

	// Set display and draw environment parameters
	SetDefDispEnv( &disp, 0, 0, SCREEN_XRES, SCREEN_YRES );
	SetDefDrawEnv( &draw, 0, 0, SCREEN_XRES, SCREEN_YRES );
	// disp.isinter = 0; // Not enable interlace (required for hires)
	
	// Set clear color, area clear and dither processing
	setRGB0( &draw, COLORBG_R, COLORBG_G, COLORBG_B );
	draw.isbg = 1;
	draw.dtd = 1;
	
	// Apply the display and drawing environments
	PutDispEnv( &disp );
	PutDrawEnv( &draw );
	
	// Enable video output
	SetDispMask( 1 );
	
	// Upload the textures
	GetTimInfo( (u_long*)buttons_tim, &buttons_image );			// Get TIM parameters
	LoadImage( buttons_image.prect, buttons_image.paddr );		// Upload texture to VRAM
	if( buttons_image.mode & 0x8 ) {
		LoadImage( buttons_image.crect, buttons_image.caddr );	// Upload CLUT if present
	}

    // Initialize pads. Using BIOS pad driver.
	// Upcoming change: Use custom SPI driver. This would enable rumble,
	// automatic analog mode, and maybe pressure for DS2.
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	StartPAD();
	ChangeClearPAD(0);
	
	// Load font and set multiple streams for every "block of text" to be used
	FntLoad(960, 0);
	font_id[0] = FntOpen(12, 20, 128, 128, 0, 80);
	font_id[1] = FntOpen(12, PORT_OFFSET+20, 128, 128, 0, 80);
	font_id[2] = FntOpen(73, 88, 192, 32, 0, 80);
	font_id[3] = FntOpen(73, PORT_OFFSET+88, 192, 32, 0, 80);
	font_id[4] = FntOpen(12, 210, 200, 32, 0, 80);	
}

int main(int argc, const char* argv[]) {
	
	// Sprites for every button
	SPRT_ARRAY SPRTS;

	// Custom Sprite array, this holds information for every Sprite
	SPRITE_INFO_ARRAY SPRITES_INFO; 

	// Variable to hold values of the buffer for every port
   	PADTYPE	*pad;
	
	// Init graphics and Sprites before doing anything else
	init();

	// Get and set info for every sprite
	// Getting all of these into a function somehow breaks everything?
    GetSprite(&buttons_image, &SPRITES_INFO.up,    PAD_UP,    COLOR_PS_NONE,  0, 0, CENTER_X-FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.right, PAD_RIGHT, COLOR_PS_NONE,  1, 0, CENTER_X-FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &SPRITES_INFO.down,  PAD_DOWN,  COLOR_PS_NONE,  2, 0, CENTER_X-FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.left,  PAD_LEFT,  COLOR_PS_NONE,  3, 0, CENTER_X-FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &SPRITES_INFO.triangle, PAD_TRIANGLE, COLOR_PS_TRIANGLE, 0, 1, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.circle,   PAD_CIRCLE,   COLOR_PS_CIRCLE,   1, 1, CENTER_X+FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &SPRITES_INFO.cross,    PAD_CROSS,    COLOR_PS_CROSS,    2, 1, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.square,   PAD_SQUARE,   COLOR_PS_SQUARE,   3, 1, CENTER_X+FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &SPRITES_INFO.bg_triangle, PAD_NONE, COLOR_PS_NONE, 3, 3, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.bg_circle,   PAD_NONE, COLOR_PS_NONE, 3, 3, CENTER_X+FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &SPRITES_INFO.bg_cross,    PAD_NONE, COLOR_PS_NONE, 3, 3, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &SPRITES_INFO.bg_square,   PAD_NONE, COLOR_PS_NONE, 3, 3, CENTER_X+FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &SPRITES_INFO.L1,	PAD_L1, COLOR_PS_BUMPTRIG, 0, 2, CENTER_X-LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y+(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &SPRITES_INFO.L2, PAD_L2, COLOR_PS_BUMPTRIG, 1, 2, CENTER_X-LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y-(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &SPRITES_INFO.R1, PAD_R1, COLOR_PS_BUMPTRIG, 2, 2, CENTER_X+LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y+(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &SPRITES_INFO.R2, PAD_R2, COLOR_PS_BUMPTRIG, 3, 2, CENTER_X+LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y-(BTN_SHIFT>>1));

    GetSprite(&buttons_image, &SPRITES_INFO.select, PAD_SELECT, COLOR_PS_NONE, 0, 3, CENTER_X-LR_CENTER_X, FACE_CENTER_Y);
    GetSprite(&buttons_image, &SPRITES_INFO.start,  PAD_START,  COLOR_PS_NONE, 1, 3, CENTER_X+LR_CENTER_X, FACE_CENTER_Y);

    GetSprite(&buttons_image, &SPRITES_INFO.bg_L, PAD_L3, COLOR_PS_BG, 0, 4, CENTER_X-36, FACE_CENTER_Y + 20);
    GetSprite(&buttons_image, &SPRITES_INFO.bg_R, PAD_R3, COLOR_PS_BG, 0, 4, CENTER_X+20, FACE_CENTER_Y + 20);

    GetSprite(&buttons_image, &SPRITES_INFO.point_L, PAD_NONE, COLOR_PUSHED, 2, 3, CENTER_X-28, FACE_CENTER_Y + 28);
    GetSprite(&buttons_image, &SPRITES_INFO.point_R, PAD_NONE, COLOR_PUSHED, 2, 3, CENTER_X+28, FACE_CENTER_Y + 28);

	// Main loop
	while(1) {
		
		// Clear ordering table and set start address of primitive
		// buffer for next frame
		ClearOTagR( ot[db], OT_LEN );
		nextpri = pribuff[db];

		// We have two ports on the console, so...
		for(int port=0; port<2; port++) {
			
			// Get inputs to this variable
			pad = (PADTYPE*)&pad_buff[port][0];

			// Status is HIGH when unplugged, LOW when plugged
			if (pad->stat) FntPrint(font_id[port], "Port %d:\nNone",port+1);
			else {
				switch(pad->type) {
					// Goal: Support every type of input
					// If it doesn't fall in this categories, this controller is not supported
					default:
						FntPrint(font_id[port], "Port %d:\nNot Supported",port+1);
						break;
					
					// Digital Pad: 0x4
					case PAD_ID_DIGITAL:
						FntPrint(font_id[port], "Port %d:\nDigital",port+1);
						DrawDigitalButtonPresses(port, pad->btn, SPRTS, SPRITES_INFO);
						break;

					// Analog Flightstick: 0x5
					case PAD_ID_ANALOG_STICK:
						FntPrint(font_id[port], "Port %d:\nFlightstick",port+1);
						DrawDigitalButtonPresses(port, pad->btn, SPRTS, SPRITES_INFO);
						DrawAnalogJoystickMovement(port, pad->btn, pad->ls_x, pad->ls_y, pad->rs_x, pad->rs_y, SPRTS, SPRITES_INFO);
						break;
					
					// Analog Pad: 0x7
					case PAD_ID_ANALOG:
						FntPrint(font_id[port], "Port %d:\nAnalog",port+1);
						DrawDigitalButtonPresses(port, pad->btn, SPRTS, SPRITES_INFO);
						DrawAnalogJoystickMovement(port, pad->btn, pad->ls_x, pad->ls_y, pad->rs_x, pad->rs_y, SPRTS, SPRITES_INFO);
						break;
				}
			}

		}

		FntPrint(font_id[4], "PS1 Mini Pad Test V0.4\nBy Luisibalaz & PSn00bSDK");

		FntFlush(font_id[0]);
		FntFlush(font_id[1]);
		FntFlush(font_id[2]);
		FntFlush(font_id[3]);
		FntFlush(font_id[4]);

		// Wait for GPU and VSync
		DrawSync( 0 );
		VSync( 0 );
		
		// Since draw.isbg is non-zero this clears the screen
		PutDrawEnv( &draw );
		
		// Begin drawing the new frame
		DrawOTag( ot[db]+(OT_LEN-1) );
		
		// Alternate to the next buffer
		db = !db;
		
	}
		
	return 0;

}