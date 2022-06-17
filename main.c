/* 
This is PS1MiniPadTest. A a small sized Pad Tester
for the original Play Station and PSOne. Smallenough
to fit inside a Memory Card and use it with FreePSXBoot.
 */
 
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgte.h>
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

#define PORT_OFFSET	(80+SCREEN_YRES/8)

// Colors
#define COLORBG_R	0x10
#define COLORBG_G	0x10
#define COLORBG_B	0x10

#define COLOR_PS_CROSS		0x9BADE4
#define COLOR_PS_SQUARE		0xD591BD
#define COLOR_PS_TRIANGLE	0x39DEC8
#define COLOR_PS_CIRCLE		0xF06E6C
#define COLOR_PS_NONE		0x585858
#define COLOR_PS_NEUTRAL	0xF0F0F0
#define COLOR_PUSHED		0xFFFF00

// Button pushes

#define PAD_NONE	0xFFFF

/* Display and drawing environments */
DISPENV disp;
DRAWENV draw;

char	pribuff[2][65536];		/* Primitive packet buffers */
u_long	ot[2][OT_LEN];			/* Ordering tables */
char	*nextpri;				/* Pointer to next packet buffer offset */
int		db = 0;					/* Double buffer index */

// TIM image parameters for loading the ball texture and drawing sprites
extern u_int buttons_tim[];
TIM_IMAGE buttons_image;

// Pad buffer
char pad_buff[2][34];

// Hold sprites info
typedef struct _SPRITE {
    u_short tpage;	// Tpage value
    u_short clut;	// CLUT value
	u_int x,y;		// Relative coordinates
    u_char u,v;		// UV offset
    CVECTOR col;
	int PAD;
} SPRITE;

// Hold the ID's for the fonts displays
int font_id[2];

void GetSprite(TIM_IMAGE *tim, SPRITE *sprite,int PAD, uint32_t color, int u, int v, int x, int y) {

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

	sprite->PAD = PAD;

    // Set UV offset
    sprite->u = u*16;
    sprite->v = v*16;

    // Set neutral color
    sprite->col.r = (color>>16)&0xFF;
    sprite->col.g = (color>>8)&0xFF;
    sprite->col.b = color&0xFF;
}

char *SortSprite(u_int PORT,uint16_t btn, SPRT_16 *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE *sprite) {
    sprt = (SPRT_16*)pri;               // initialize the sprite
    setSprt16(sprt);

    setXY0(sprt,sprite->x,sprite->y+PORT_OFFSET*PORT);                   // Set position
    setUV0(sprt, sprite->u, sprite->v);  // Set size
    setRGB0(sprt,                       // Set color
        (btn&sprite->PAD) ? sprite->col.r : 0xff,
        (btn&sprite->PAD) ? sprite->col.g : 0xff,
        (btn&sprite->PAD) ? sprite->col.b : 0x00);
    sprt->clut = sprite->clut;          // Set the CLUT value

    addPrim(ot, sprt);                  // Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

char *SortSprite32(u_int PORT,uint16_t btn, SPRT *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE *sprite) {
    sprt = (SPRT*)pri;               // initialize the sprite
    setSprt(sprt);

    setXY0(sprt,sprite->x,sprite->y+PORT_OFFSET*PORT);                   // Set position
    setUV0(sprt, sprite->u, sprite->v);
	setWH(sprt, 32, 32);  // Set size
    setRGB0(sprt,                       // Set color
        (btn&sprite->PAD) ? sprite->col.r : 0xff,
        (btn&sprite->PAD) ? sprite->col.g : 0xff,
        (btn&sprite->PAD) ? sprite->col.b : 0x00);
    sprt->clut = sprite->clut;          // Set the CLUT value

    addPrim(ot, sprt);                  // Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

char *SortSpriteMoving(uint8_t mov_x, uint8_t mov_y,u_int PORT, SPRT_16 *sprt,DR_TPAGE *tpage, u_long *ot, char *pri, SPRITE *sprite) {
    sprt = (SPRT_16*)pri;               // initialize the sprite
    setSprt16(sprt);

    setXY0(sprt,sprite->x + (mov_x - 128>>3),sprite->y + (mov_y - 128>>3) + PORT_OFFSET*PORT);                   // Set position
    setUV0(sprt, sprite->u, sprite->v);
    setRGB0(sprt,                       // Set color
        sprite->col.r,
        sprite->col.g,
        sprite->col.b);
    sprt->clut = sprite->clut;          // Set the CLUT value

    addPrim(ot, sprt);                  // Sort the primitive and advance
    sprt++;
	pri = (char*)sprt;
    
    tpage = (DR_TPAGE*)pri;             // Sort the texture page value
    setDrawTPage(tpage, 0, 0, sprite->tpage);
    addPrim(ot, tpage);

    return pri+sizeof(DR_TPAGE);        // Return new primitive pointer
}                                       // (set to nextpri)

void init() {
	
	/* Reset GPU (also installs event handler for VSync) */
	printf("Init GPU... ");
	ResetGraph( 0 );
	printf("Done.\n");
	
	
	printf("Set video mode... ");
	
	/* Set display and draw environment parameters */
	SetDefDispEnv( &disp, 0, 0, SCREEN_XRES, SCREEN_YRES );
	SetDefDrawEnv( &draw, 0, 0, SCREEN_XRES, SCREEN_YRES );
	// disp.isinter = 0; /* Not enable interlace (required for hires) */
	
	/* Set clear color, area clear and dither processing */
	setRGB0( &draw, COLORBG_R, COLORBG_G, COLORBG_B );
	draw.isbg = 1;
	draw.dtd = 1;
	
	/* Apply the display and drawing environments */
	PutDispEnv( &disp );
	PutDrawEnv( &draw );
	
	/* Enable video output */
	SetDispMask( 1 );
	
	printf("Done.\n");
	
	
	/* Upload the textures */
	printf("Upload texture... ");
	
	GetTimInfo( (u_long*)buttons_tim, &buttons_image ); /* Get TIM parameters */
	LoadImage( buttons_image.prect, buttons_image.paddr );		/* Upload texture to VRAM */
	if( buttons_image.mode & 0x8 ) {
		LoadImage( buttons_image.crect, buttons_image.caddr );	/* Upload CLUT if present */
	}

	printf("Done.\n");

    // Initialize pads
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	StartPAD();
	ChangeClearPAD(0);
	
	// Load font and set two font displays
	FntLoad(960, 0);
	font_id[0] = FntOpen(12, 20, 128, 128, 0, 80);
	font_id[1] = FntOpen(12, PORT_OFFSET+20, 128, 128, 0, 80);
	
	printf("Done.\n");
	
}

int main(int argc, const char* argv[]) {
	
	DR_TPAGE *tpage;
	
	SPRT_16 *sprt_up;
	SPRT_16 *sprt_right;
	SPRT_16 *sprt_down;
	SPRT_16 *sprt_left;

	SPRT_16 *sprt_triangle;
	SPRT_16 *sprt_circle;
	SPRT_16 *sprt_cross;
	SPRT_16 *sprt_square;

	SPRT_16 *sprt_bg_triangle;
	SPRT_16 *sprt_bg_circle;
	SPRT_16 *sprt_bg_cross;
	SPRT_16 *sprt_bg_square;
	
	SPRT_16 *sprt_L1;
	SPRT_16 *sprt_L2;
	SPRT_16 *sprt_R1;
	SPRT_16 *sprt_R2;

	SPRT_16 *sprt_select;
	SPRT_16 *sprt_start;

	SPRT_16 *sprt_point_R;
	SPRT_16 *sprt_point_L;
	SPRT *sprt_bg_R;
	SPRT *sprt_bg_L;
    
	// Custom Sprite

	SPRITE up;
	SPRITE right;
	SPRITE down;
	SPRITE left;
	
	SPRITE triangle;
	SPRITE circle;
	SPRITE cross;
	SPRITE square;
	
	SPRITE bg_triangle;
	SPRITE bg_circle;
	SPRITE bg_cross;
	SPRITE bg_square;
	
	SPRITE L1;
	SPRITE L2;
	SPRITE R1;
	SPRITE R2;
	
	SPRITE select;
	SPRITE start;
	
	SPRITE point_R;
	SPRITE point_L;
	SPRITE bg_R;
	SPRITE bg_L;

   	PADTYPE	*pad;
	
	/* Init graphics and stuff before doing anything else */
	init();
	
    GetSprite(&buttons_image, &up,    PAD_UP,    COLOR_PS_NONE,  0, 0, CENTER_X-FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &right, PAD_RIGHT, COLOR_PS_NONE,  1, 0, CENTER_X-FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &down,  PAD_DOWN,  COLOR_PS_NONE,  2, 0, CENTER_X-FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &left,  PAD_LEFT,  COLOR_PS_NONE,  3, 0, CENTER_X-FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &triangle, PAD_TRIANGLE, COLOR_PS_TRIANGLE, 0, 1, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &circle,   PAD_CIRCLE,   COLOR_PS_CIRCLE,   1, 1, CENTER_X+FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &cross,    PAD_CROSS,    COLOR_PS_CROSS,    2, 1, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &square,   PAD_SQUARE,   COLOR_PS_SQUARE,   3, 1, CENTER_X+FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &bg_triangle, PAD_NONE, COLOR_PS_NEUTRAL, 3, 3, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y-BTN_SHIFT);
    GetSprite(&buttons_image, &bg_circle,   PAD_NONE, COLOR_PS_NEUTRAL, 3, 3, CENTER_X+FACE_CENTER_X+BTN_SHIFT, FACE_CENTER_Y);
    GetSprite(&buttons_image, &bg_cross,    PAD_NONE, COLOR_PS_NEUTRAL, 3, 3, CENTER_X+FACE_CENTER_X,           FACE_CENTER_Y+BTN_SHIFT);
    GetSprite(&buttons_image, &bg_square,   PAD_NONE, COLOR_PS_NEUTRAL, 3, 3, CENTER_X+FACE_CENTER_X-BTN_SHIFT, FACE_CENTER_Y);

    GetSprite(&buttons_image, &L1, PAD_L1, COLOR_PS_NONE, 0, 2, CENTER_X-LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y+(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &L2, PAD_L2, COLOR_PS_NONE, 1, 2, CENTER_X-LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y-(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &R1, PAD_R1, COLOR_PS_NONE, 2, 2, CENTER_X+LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y+(BTN_SHIFT>>1));
    GetSprite(&buttons_image, &R2, PAD_R2, COLOR_PS_NONE, 3, 2, CENTER_X+LR_CENTER_X, FACE_CENTER_Y - LR_CENTER_Y-(BTN_SHIFT>>1));

    GetSprite(&buttons_image, &select, PAD_SELECT, COLOR_PS_NONE, 0, 3, CENTER_X-LR_CENTER_X, FACE_CENTER_Y);
    GetSprite(&buttons_image, &start,  PAD_START,  COLOR_PS_NONE, 1, 3, CENTER_X+LR_CENTER_X, FACE_CENTER_Y);

    GetSprite(&buttons_image, &bg_R, PAD_R3, COLOR_PS_NONE, 0, 4, CENTER_X+20, FACE_CENTER_Y + 24);
    GetSprite(&buttons_image, &bg_L, PAD_L3, COLOR_PS_NONE, 0, 4, CENTER_X-36, FACE_CENTER_Y + 24);

    GetSprite(&buttons_image, &point_R, PAD_NONE, 0xFFFF00, 2, 3, CENTER_X+28, FACE_CENTER_Y + 32);
    GetSprite(&buttons_image, &point_L, PAD_NONE, 0xFFFF00, 2, 3, CENTER_X-28, FACE_CENTER_Y + 32);
	
	/* Main loop */
	printf("Entering loop...\n");
	
	while(1) {
		
		/* Clear ordering table and set start address of primitive */
		/* buffer for next frame */
		ClearOTagR( ot[db], OT_LEN );
		nextpri = pribuff[db];

		for(int port=0;port<2;port++) {
			
			/* Handle inputs */
			pad = (PADTYPE*)&pad_buff[port][0];

			FntPrint(font_id[port], "Controller:\n%s\n",
				(pad->type==PAD_ID_DIGITAL & !pad->stat) ? "DIGITAL" :
					((pad->type==PAD_ID_ANALOG | pad->type==PAD_ID_ANALOG_STICK) & !pad->stat) ? "ANALOG" : "NONE");


			if (! pad->stat & (pad->type==PAD_ID_DIGITAL | pad->type==PAD_ID_ANALOG | pad->type==PAD_ID_ANALOG_STICK)) {
				nextpri = SortSprite(port, pad->btn, sprt_up,    tpage, ot[db] + (OT_LEN - 1), nextpri, &up);
				nextpri = SortSprite(port, pad->btn, sprt_right, tpage, ot[db] + (OT_LEN - 1), nextpri, &right);
				nextpri = SortSprite(port, pad->btn, sprt_down,  tpage, ot[db] + (OT_LEN - 1), nextpri, &down);
				nextpri = SortSprite(port, pad->btn, sprt_left,  tpage, ot[db] + (OT_LEN - 1), nextpri, &left);

				nextpri = SortSprite(port, pad->btn, sprt_triangle, tpage, ot[db] + (OT_LEN - 1), nextpri, &triangle);
				nextpri = SortSprite(port, pad->btn, sprt_circle,   tpage, ot[db] + (OT_LEN - 1), nextpri, &circle);
				nextpri = SortSprite(port, pad->btn, sprt_cross,    tpage, ot[db] + (OT_LEN - 1), nextpri, &cross);
				nextpri = SortSprite(port, pad->btn, sprt_square,   tpage, ot[db] + (OT_LEN - 1), nextpri, &square);

				nextpri = SortSprite(port, pad->btn, sprt_bg_triangle, tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_triangle);
				nextpri = SortSprite(port, pad->btn, sprt_bg_circle,   tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_circle);
				nextpri = SortSprite(port, pad->btn, sprt_bg_cross,    tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_cross);
				nextpri = SortSprite(port, pad->btn, sprt_bg_square,   tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_square);
				
				nextpri = SortSprite(port, pad->btn, sprt_L1, tpage, ot[db] + (OT_LEN - 1), nextpri, &L1);
				nextpri = SortSprite(port, pad->btn, sprt_L2, tpage, ot[db] + (OT_LEN - 1), nextpri, &L2);
				nextpri = SortSprite(port, pad->btn, sprt_R1, tpage, ot[db] + (OT_LEN - 1), nextpri, &R1);
				nextpri = SortSprite(port, pad->btn, sprt_R2, tpage, ot[db] + (OT_LEN - 1), nextpri, &R2);

				nextpri = SortSprite(port, pad->btn, sprt_select, tpage, ot[db] + (OT_LEN - 1), nextpri, &select);
				nextpri = SortSprite(port, pad->btn, sprt_start,  tpage, ot[db] + (OT_LEN - 1), nextpri, &start);

				if (pad->type == PAD_ID_ANALOG | pad->type == PAD_ID_ANALOG_STICK)
				{
					nextpri = SortSpriteMoving(pad->rs_x, pad->rs_y, port, sprt_point_R, tpage, ot[db] + (OT_LEN - 1), nextpri, &point_R);
					nextpri = SortSpriteMoving(pad->ls_x, pad->ls_y, port, sprt_point_L, tpage, ot[db] + (OT_LEN - 1), nextpri, &point_L);
					nextpri = SortSprite32(port, pad->btn, sprt_bg_R, tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_R);
					nextpri = SortSprite32(port, pad->btn, sprt_bg_L, tpage, ot[db] + (OT_LEN - 1), nextpri, &bg_L);
					
					// Print joystick values. Change range and improve display!
					FntPrint(font_id[port], "\n\n\n\nLSX %3d\nLSY %3d\nRSX %3d\nRSY %3d", pad->ls_x, pad->ls_y, pad->rs_x, pad->rs_y);
				}
			}

		}

		FntFlush(font_id[0]);
		FntFlush(font_id[1]);

		/* Wait for GPU and VSync */
		DrawSync( 0 );
		VSync( 0 );
		
		/* Since draw.isbg is non-zero this clears the screen */
		PutDrawEnv( &draw );
		
		/* Begin drawing the new frame */
		DrawOTag( ot[db]+(OT_LEN-1) );
		
		/* Alternate to the next buffer */
		db = !db;
		
	}
		
	return 0;

}