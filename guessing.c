
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>


// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

#include "apu.h"
//#link "apu.c"

#include <stdbool.h>

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x13,			// screen color

  0x11,0x30,0x27,0x0,	// background palette 0
  0x1c,0x20,0x2c,0x0,	// background palette 1
  0x00,0x10,0x20,0x0,	// background palette 2
  0x06,0x16,0x26,0x0,   // background palette 3

  0x16,0x35,0x24,0x0,	// sprite palette 0
  0x00,0x37,0x25,0x0,	// sprite palette 1
  0x0d,0x2d,0x3a,0x0,	// sprite palette 2
  0x0d,0x27,0x2a	// sprite palette 3
};



//
// MUSIC ROUTINES
//

// Namespace(bias=1.0, freq=111860.8, length=64, maxbits=13.0, upper=41)
// 439.0 0.943191918851 41
const int note_table_41[64] = {
4318, 4076, 3847, 3631, 3427, 3235, 3053, 2882, 2720, 2567, 2423, 2287, 2159, 2037, 1923, 1815, 1713, 1617, 1526, 1440, 1360, 1283, 1211, 1143, 1079, 1018, 961, 907, 856, 808, 763, 720, 679, 641, 605, 571, 539, 509, 480, 453, 428, 403, 381, 359, 339, 320, 302, 285, 269, 254, 240, 226, 213, 201, 190, 179, 169, 160, 151, 142, 134, 126, 119, 113, };

// Namespace(bias=1.0, freq=111860.8, length=64, maxbits=13, upper=49)
// 440.5 1.79281159771 49
const int note_table_49[64] = {
4304, 4062, 3834, 3619, 3416, 3224, 3043, 2872, 2711, 2559, 2415, 2279, 2151, 2031, 1917, 1809, 1707, 1611, 1521, 1436, 1355, 1279, 1207, 1139, 1075, 1015, 958, 904, 853, 805, 760, 717, 677, 639, 603, 569, 537, 507, 478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253, 239, 225, 213, 201, 189, 179, 168, 159, 150, 142, 134, 126, 119, 112, };

// Namespace(bias=1.0, freq=111860.8, length=64, maxbits=12, upper=63)
// 443.6 14.2328382554 63
const int note_table_63[64] = {
2137, 4034, 3807, 3593, 3392, 3201, 3022, 2852, 2692, 2541, 2398, 2263, 2136, 2016, 1903, 1796, 1695, 1600, 1510, 1425, 1345, 1270, 1199, 1131, 1068, 1008, 951, 898, 847, 800, 755, 712, 672, 634, 599, 565, 533, 503, 475, 448, 423, 399, 377, 356, 336, 317, 299, 282, 266, 251, 237, 224, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 111, };

// Namespace(bias=-1.0, freq=55930.4, length=64, maxbits=12, upper=53)
// 443.7 8.47550713772 53
const int note_table_tri[64] = {
2138, 2018, 1905, 1798, 1697, 1602, 1512, 1427, 1347, 1272, 1200, 1133, 1069, 1009, 953, 899, 849, 801, 756, 714, 674, 636, 601, 567, 535, 505, 477, 450, 425, 401, 379, 358, 338, 319, 301, 284, 268, 253, 239, 226, 213, 201, 190, 179, 169, 160, 151, 142, 135, 127, 120, 113, 107, 101, 95, 90, 85, 80, 76, 72, 68, 64, 60, 57, };

#define NOTE_TABLE note_table_49
#define BASS_NOTE 36

byte music_index = 0;
byte cur_duration = 0;

const byte music1[]; // music data -- see end of file
const byte* music_ptr = music1;

byte next_music_byte() {
  return *music_ptr++;
}

void play_music() {
  static byte chs = 0;
  if (music_ptr) {
    // run out duration timer yet?
    while (cur_duration == 0) {
      // fetch next byte in score
      byte note = next_music_byte();
      // is this a note?
      if ((note & 0x80) == 0) {
        // pulse plays higher notes, triangle for lower if it's free
        if (note >= BASS_NOTE || (chs & 4)) {
          int period = NOTE_TABLE[note & 63];
          // see which pulse generator is free
          if (!(chs & 1)) {
            APU_PULSE_DECAY(0, period, DUTY_25, 2, 10);
            chs |= 1;
          } else if (!(chs & 2)) {
            APU_PULSE_DECAY(1, period, DUTY_25, 2, 10);
            chs |= 2;
          }
        } else {
          int period = note_table_tri[note & 63];
          APU_TRIANGLE_LENGTH(period, 15);
          chs |= 4;
        }
      } else {
        // end of score marker
        if (note == 0xff)
          music_ptr = NULL;
        // set duration until next note
        cur_duration = note & 63;
        // reset channel used mask
        chs = 0;
      }
    }
    cur_duration--;
  }
}

void start_music(const byte* music) {
  music_ptr = music;
  cur_duration = 0;
}


//
// MUSIC DATA -- "The Easy Winners" by Scott Jopline
//
const byte music1[] = {
0x27,0x14,0x0f,0x8f,0x0f,0x8e,0x2c,0x11,0x8f,0x0f,0x8e,0x2c,0x14,0x0f,0x8e,0x0f,0x88,0x16,0x87,0x11,0x8e,0x19,0x0f,0x8f,0x2c,0x14,0x0f,0x8e,0x2e,0x0f,0x8f,0x30,0x14,0x11,0x8e,0x0f,0x8e,0x12,0x0f,0x8f,0x0f,0x87,0x13,0x87,0x11,0x8f,0x13,0x8e,0x31,0x14,0x0f,0x8e,0x0f,0x8e,0x2e,0x81,0x11,0x8e,0x0f,0x8e,0x2c,0x14,0x0f,0x8f,0x0f,0x87,0x16,0x87,0x11,0x8f,0x19,0x0f,0x8e,0x30,0x14,0x0f,0x8e,0x0f,0x8f,0x14,0x11,0x8e,0x0f,0x8f,0x29,0x1b,0x0f,0x8e,0x0f,0x87,0x27,0x19,0x87,0x11,0x8f,0x13,0xff
};


// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  pal_all(PALETTE);
}

void title_screen(char pad, bool choice){
  int iter = 0;
  vram_adr(NTADR_A(9, 12));
  vram_write("PSYCHO MANTIS", 13); 
  
  vram_adr(NTADR_A(10, 20));
  vram_write("PRESS START", 11); 
  ppu_on_all();
  
  for(iter = 0; iter < 50; iter++)
  {
    ppu_wait_frame();
  }
  
  while (choice == false)
  {
    pad = pad_poll(0);
    if(pad&PAD_START)
    {
      choice = true;
      ppu_off();
    }
  }
}


void main(void)
{
  bool flag = false;
  bool choice = false;
  bool end_game = false;
  int iter;
  int max = 100;
  int low = 0;
  int guess = 0;
  char guec[3];
  int i = 0;
  char pad;	
  setup_graphics();
  
  apu_init();
  music_ptr = 0;

    
  
  
  
  //TITLE SCREEN//
  title_screen(pad, choice);
  ppu_off();
  
  //CLEARING MEMORY//////////////////////////
  vram_adr(NTADR_A(9, 12));
  vram_write("                           ", 13); // clear this rows text
  
  vram_adr(NTADR_A(10, 20));
  vram_write("                           ", 11);
  
  // draw message  
  // MESSAGE 1 ////////////////////////////////
  vram_adr(NTADR_A(13, 12));
  vram_write("HELLO.", 6);
  
  vram_adr(NTADR_A(4, 14));
  vram_write("LET ME GUESS YOUR NUMBER.", 25);
  
  
  
  // enable rendering
  ppu_on_all();
  
  
  for(iter = 0; iter < 130; iter++)
  {
    ppu_wait_frame();
  }
  // TURN OFF THE STUPID SCREEN
  ppu_off();
  
  
  ////////////////////////////////////////////
    	
    
  	vram_adr(NTADR_A(8, 12));
  	vram_write("THINK OF A NUMBER", 17);
  
  	vram_adr(NTADR_A(2, 14));
  	vram_write("BETWEEN ZERO AND ONE HUNDRED", 28);
  	// enable rendering
  	ppu_on_all();
  
  	for(iter = 0; iter < 200; iter++)
  	{
          
  	  ppu_wait_frame();
          
  	}
   
  	ppu_off();
  
  while(!end_game){
    	max = 100;
    	low = 0;
  
  	guess = (max + low) / 2; // should initialize to 50 here
  
  
  
  /////////////////////////////////////////////
  // LOOPING PART OF THE GUESSING GAME.
 	while(!flag)
 	{
    
    
    	choice = false;
    	vram_adr(NTADR_A(2, 14));
    	vram_write("                           ", 28); // clear this rows text
    
    	itoa(guess, guec, 10); // convert int to chr*
 
    
    	vram_adr(NTADR_A(5, 12));
    	vram_write("I GUESS YOUR NUMBER IS: ", 24);
    
    	vram_adr(NTADR_A(15, 14));
    	vram_write(guec, 2);
    	// String interpolation here
    
    	vram_adr(NTADR_A(2, 18));
    	vram_write("A:Too High", 11);
    
    	vram_adr(NTADR_A(2, 20));
    	vram_write("B:Too Low", 10);
    
    	vram_adr(NTADR_A(2, 22));
    	vram_write("Start:Correct", 14);
    
    
    
    // enable rendering
    	ppu_on_all();
    
    	for(iter = 0; iter < 50; iter++)
    	{
          if (!music_ptr) start_music(music1);
      	  waitvsync();
      	  play_music();
    	  ppu_wait_frame();
    	}
    
    	while(choice == false)
    	{
          if (!music_ptr) start_music(music1);
      	  waitvsync();
      	  play_music();
      
      //for(i = 0; i<2; i++)
      //{
        	pad = pad_poll(0);
        	if(pad&PAD_A){
        	  max = guess - 1;
        	  guess = (max + low) / 2;
        	  choice = true;
        	  ppu_off();
        	}
        	else if(pad&PAD_B){
        	  low = guess + 1;
        	  guess = (max + low) / 2;
        	  choice = true;
        	  ppu_off();
        	}

        	else if(pad&PAD_START){
          	  flag = true;
          	  choice = true;
          	  ppu_off();
      		}
      	//}
    	}
        
        choice = false;
        ppu_off();
  	}
    
    //I'm clearing this way 
    vram_adr(NTADR_A(2, 12));
    vram_write("                           ", 28); // clear this rows text
    
    vram_adr(NTADR_A(2, 14));
    vram_write("                           ", 28); // clear this rows text
    
    vram_adr(NTADR_A(2, 18));
    vram_write("                           ", 28); // clear this rows text
    
    vram_adr(NTADR_A(2, 20));
    vram_write("                           ", 28); // clear this rows text
    
    vram_adr(NTADR_A(2, 22));
    vram_write("                           ", 28); // clear this rows text
    
    ppu_off();
    vram_adr(NTADR_A(12, 12));
    vram_write("Again?", 6);
    
    vram_adr(NTADR_A(2, 18));
    vram_write("A:Yes", 5);
    
    vram_adr(NTADR_A(2, 20));
    vram_write("B:No", 4);
    ppu_on_all();
    
    while(choice == false){
      if (!music_ptr) start_music(music1);
      	  waitvsync();
      	  play_music();
    
    	pad = pad_poll(0);
    	if(pad&PAD_A){
          flag = false;
          choice = true;
          ppu_off();
        }
    	else if(pad&PAD_B){
        	choice = true;
    		end_game = true;
        	ppu_off();
       	}
      
    }
    choice = false;
    vram_adr(NTADR_A(2, 18));
    vram_write("                           ", 28); // clear this rows text
    
    vram_adr(NTADR_A(2, 20));
    vram_write("                           ", 28); // clear this rows text
  
  	
  }
  // infinite loop
  while(1) {
    ppu_off();
    vram_adr(NTADR_A(13, 12));
    vram_write("help i'm stuck", 22);
    
    vram_adr(NTADR_A(13, 20));
    vram_write("is that you, god?", 30);
    ppu_on_all();
  }
}

