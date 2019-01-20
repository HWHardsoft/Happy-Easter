/*
 *  Happy easter for Uzebox 
 *  Version 1.0
 *  Copyright (C) 2012  Hartmut Wendt
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "kernel/uzebox.h"
#include "data/happy_easter_Tiles.pic.inc"
#include "data/fonts.pic.inc"
#include "data/TitleTiles.pic.inc"
#include "data/happy_easter_sprites.pic.inc"
#include "data/patches.inc"
#include "data/Happy_Easter_tracks.inc"


#define _FadingX



/* global definitons */
// program modes
enum {
	PM_Intro,		// program mode intro
	PM_Gameplay,	// program mode game play
	PM_Credits,		// credits screen
	PM_Help,		// help screen
	PM_HoF_view,	// show hall of fame
	PM_HoF_edit		// edit name in hall of fame

};

// egg types
enum {
	empty_egg,
	yellow_egg,
	green_egg,
	grenade,
	carrot,
};


/* animations */
#define chicken_max_animation 18
const char *ani_chicken[] = {
	Chicken_2,
	Chicken_3,
	Chicken_4,
	Chicken_5,
	Chicken_5,
	Chicken_5,
	Chicken_4,
	Chicken_4,
	Chicken_4,
	Chicken_5,
	Chicken_5,
	Chicken_5,
	Chicken_4,
	Chicken_4,
	Chicken_7,
	Chicken_6,
	Chicken_6,
	Chicken_1
};

#define bunny_max_animation 8
const char *ani_bunny[] = {
	Bunny_1,
	Bunny_2,
	Bunny_3,
	Bunny_4,
	Bunny_5,
	Bunny_6,
	Bunny_7,
	Bunny_8
};

const char *ani_yellow_egg[] = {
	Yellow_egg1,
	Yellow_egg2,
	Yellow_egg3
};

const char *ani_green_egg[] = {
	Green_egg1,
	Green_egg2,
	Green_egg3
};


const char *ani_grenade[] = {
	Grenade1,
	Grenade2,
	Grenade3
};

const char *ani_carrot[] = {
	Carrot1,
	Carrot2,
	Carrot3
};


#define bunny_speed 3;				// increment of bunny x position
const u16 Max_egg_timer=400;		// time between 2 falling eggs at game start
const u16 Min_egg_timer=200;		// smallest time between 2 falling eggs 
const u8 Max_Confusion_Time = 100;	// time for bunny confusion after putting a green egg





// 8-bit, 255 period LFSR (for generating pseudo-random numbers)
#define PRNG_NEXT() (prng = ((u8)((prng>>1) | ((prng^(prng>>2)^(prng>>3)^(prng>>4))<<7))))
#define MAX(x,y) ((x)>(y) ? (x) : (y))




struct EepromBlockStruct ebs;

struct egg {
	u8 PosY;
	u8 SpriteType;
	bool Enabled;
	};

struct egg default_egg = {0,empty_egg,false};


u8 prng; // Pseudo-random number generator

u8 program_mode;	// program mode (intro, 1 player mode, 2 player mode, ....
u8 ani_count_bunny=8;		// counter for animation of bunny
u8 ani_count_chicken[5];	// counter for animation of chicken
struct egg eggs[5];

u8 X_pos_bunny;
u16 iEgg_Timer;
u16 iNew_Egg_Timer;
u8 bunny_sprite_direction=0;// bunny moving in left (reverse) direction

bool BMusic_on = true;
u16 iHighscore;
u8 broken_eggs;
u8 Controller_status2;
u8 autorepeat_cnt;
u8 Confusion_Timer;
u8 PosY;
u8 PosX;



/*** function prototypes *****************************************************************/
void init(void);
void set_PM_mode(u8 mode);
void msg_window(u8 x1, u8 y1, u8 x2, u8 y2);
u8 set_def_EEPROM(void);
void load_def_EEPROM(void);
void save_def_EEPROM(void);
void animate_bunny(void);
void animate_chicken(u8 chicken, u8 *ani_step);
void animate_egg(u8 egg);
u8 GetTile(u8 x, u8 y);
void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ);
void fill_buf(u8 *BUFA, u8 content, u8 ucANZ);
void Draw_egg_reserve(u8 eggs);
void clear_egg(u8 egg);
bool collect_eggs(u8 egg);
void new_egg(void);
void game_over(void);
int get_highscore(u8 entry);
u8 check_highscore(void);
void copy_highsore(u8 entry_from, u8 entry_to);
void clear_highsore(u8 entry);
u8 set_def_highscore(void);
u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data);
void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode);
void show_highscore_char(u8 entry, u8 position, u8 cursor_on);


void init(void)
// init program
{
  // init tile table
  SetTileTable(BGTiles);
  // init font table
  SetFontTilesIndex(BGTILES_SIZE);
  // init Sprites
  SetSpritesTileTable(spriteTiles);	

  // init music	
  InitMusicPlayer(patches);
  // load into screen
  set_PM_mode(PM_Intro);
  
  // init random generator

  Controller_status2 = DetectControllers() & 0x0C;	

  //Use block 24
  ebs.id = 24;
  if (!isEepromFormatted())
     return;

  if (EEPROM_ERROR_BLOCK_NOT_FOUND == EepromReadBlock(24,&ebs))
  {
	set_def_highscore();
  }	
   		

}



int main(){
int ibuttons=0,ibuttons_old;
u8 uc1, uc2=0;

  // init program
  init();        
  // proceed game	
  while(1)
  {
    WaitVsync(3);	  
    // get controller state
    ibuttons_old = ibuttons;
	ibuttons = ReadJoypad(0);
    switch(program_mode)
	{
	  // proceed intro mode	
	  case PM_Intro:
     	if ((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) {
		  // wait for release of button A 
		  while (BTN_A & ReadJoypad(0)) {
		    prng++; 		
			WaitVsync(1);
		  }
      	  prng = MAX(prng,1);
		  if (PosY == 0) set_PM_mode(PM_Gameplay);	
		  else if (PosY == 1) set_PM_mode(PM_Help);			  
		  else if (PosY == 4) set_PM_mode(PM_Credits);	
  		  
		  else if (PosY == 3) {
		    iHighscore = 0; 
		    set_PM_mode(PM_HoF_view);

		  } else {
		    if (BMusic_on) {
			  BMusic_on = false;
			  DrawMap2(13,16,Music_Off_map);
			  StopSong();
		    } else {
			  BMusic_on = true;	
			  Fill(18,16,3,1,0);		  
			  DrawMap2(13,16,Music_On_map);
			  ResumeSong();
			}
		  }
 		}
		// UP button
		if ((ibuttons & BTN_UP) && !(ibuttons_old & BTN_UP)) {

		  // wait for release of button BTN_UP 
		  while (BTN_UP & ReadJoypad(0));
		  
		  Fill(11,10 + (PosY * 3),1,2,0);
		  if (PosY) PosY--;	
		  DrawMap2(11,10 + (PosY * 3),Cursor_egg);		  

		// DOWN button
		} else if ((ibuttons & BTN_DOWN) && !(ibuttons_old & BTN_DOWN)) {

		  // wait for release of button BTN_DOWN 
		  while (BTN_DOWN & ReadJoypad(0));

		  Fill(11,10 + (PosY * 3),1,2,0);
		  if (PosY<4) PosY++;	
		  DrawMap2(11,10 + (PosY * 3),Cursor_egg);		  
		  
		}
        break;

	  // proceed game_play	
	  case PM_Gameplay:
	  	// bunny animation and moving	
	  	// move the bunny left		
		if (((ibuttons & BTN_LEFT) && !Confusion_Timer) || ((ibuttons & BTN_RIGHT) && Confusion_Timer))  {
			if (X_pos_bunny > 10) {
			  	X_pos_bunny -= bunny_speed;	
				bunny_sprite_direction = SPRITE_FLIP_X;
				// restart animation
				if (ani_count_bunny == bunny_max_animation) ani_count_bunny = 0;
			}
				
	  	// move the bunny left
		} else if (((ibuttons & BTN_RIGHT)  && !Confusion_Timer) || ((ibuttons & BTN_LEFT) && Confusion_Timer)) {
			if (X_pos_bunny < 190) {
				X_pos_bunny += bunny_speed;
				bunny_sprite_direction = 0;
				// restart animation
				if (ani_count_bunny == bunny_max_animation) ani_count_bunny = 0;
			}
		}
		
		// animate the easter bunny
		animate_bunny();

		// bunny confusion after catching a rotten green egg
		if(Confusion_Timer) {
			Confusion_Timer--;
			// beep if confusion times end
			if (!Confusion_Timer) TriggerFx(4, 0x7f, true);
        }
				
		// generate random new eggs	
		if (iNew_Egg_Timer)	iNew_Egg_Timer--;
		else {
		  new_egg();
		  iNew_Egg_Timer = iEgg_Timer / 10;
		  if (iEgg_Timer > Min_egg_timer) iEgg_Timer -= 2;	
		}	


		// animate chicken
		for(uc1=0; uc1 < 5; uc1++) animate_chicken(uc1,&ani_count_chicken[uc1]);

		// animate eggs
		for(uc1=0; uc1 < 5; uc1++) animate_egg(uc1);

		// collision detection
		for(uc1=0; uc1 < 5; uc1++) collect_eggs(uc1);

		if (program_mode != PM_Gameplay) break;
		// Draw highscore
		PrintInt(5,2,iHighscore,true); 
		
		// Draw egg reserve in upper right corner
		if (broken_eggs) Draw_egg_reserve(broken_eggs - 1);
		else game_over();
        break;

	  // proceed credits & help screen...
	  case PM_Credits:	
	  case PM_Help:			
	  case PM_HoF_view:					
     	if (((BTN_A & ibuttons) && !(ibuttons_old & BTN_A)) ||
		   ((BTN_B & ibuttons) && !(ibuttons_old & BTN_B)) ||	
		   ((BTN_X & ibuttons) && !(ibuttons_old & BTN_Y)) ||	
		   ((BTN_Y & ibuttons) && !(ibuttons_old & BTN_Y)) ||	
		   ((BTN_START & ibuttons) && !(ibuttons_old & BTN_START)) ||	
		   ((BTN_SELECT & ibuttons) && !(ibuttons_old & BTN_SELECT))) set_PM_mode(PM_Intro);		
        break;


	  case PM_HoF_edit:				
		// cursor blinking
		uc1++;
		if (uc1 >= 10) uc1 = 0;
		// proceed cursor position with left & right button
		if ((ibuttons & BTN_RIGHT) && !(ibuttons_old & BTN_RIGHT)) {
		  if (PosX < 7) {       	   
		  	show_highscore_char(PosY - 1, PosX, 0); 		 
		    PosX++; 			
          }
		}
		if ((ibuttons & BTN_LEFT) && !(ibuttons_old & BTN_LEFT)) {		 
 		  if (PosX) {
		    show_highscore_char(PosY - 1, PosX, 0); 
		    PosX--; 
          }
		}
		// chose character up & down button
		if ((ibuttons & BTN_UP) && !(ibuttons_old & BTN_UP)) {
		  edit_highscore_entry(PosY,PosX,BTN_UP); 
		}
		else if ((ibuttons & BTN_DOWN) && !(ibuttons_old & BTN_DOWN)) {		 
 		  edit_highscore_entry(PosY,PosX,BTN_DOWN); 
		}     
		// show cursor
		show_highscore_char(PosY - 1, PosX, uc1 > 4);

		// store new entry
		if (ibuttons & BTN_A)   
		{
		  // store new highscore 
		  EepromWriteBlock(&ebs);
		  set_PM_mode(PM_Intro);
		}
		break;

	}


  }
  

} 


void set_PM_mode(u8 mode) {
// set parameters, tiles, background etc for choosed program mode
u8 uc1, uc2;

	#ifdef _FadingX
	 FadeOut(1, true);
	#endif
			
	switch (mode)
	{

	  case	PM_Intro:
		SetSpriteVisibility(false);
		StopSong();

		// init tile table
  		SetTileTable(TitleTiles);
   		 
	    // cursor is invisible now
	    StopSong();
		ClearVram();

		// fill background with pattern
		for(uc1 = 0; uc1 < 15; uc1++)
		for(uc2 = 0; uc2 < 14; uc2++)
		DrawMap2(uc1 * 2,uc2 * 2,Pattern_map);

		// draw filled rectangles for title and menu
		Fill(4,2,22,6,0);
		Fill(8,9,14,16,0);
		
		// draw game title in the top off the screen
		DrawMap2(5,3,Title);

		// draw the menu
		DrawMap2(13,10,Start_map);
		DrawMap2(13,13,Help_map); 		
		if (BMusic_on) DrawMap2(13,16,Music_On_map);
		else DrawMap2(13,16,Music_Off_map); 
		DrawMap2(13,19,Highscore_map); 		
		DrawMap2(13,22,Credits_map); 		
		
		// draw the cursor egg
		DrawMap2(11,10,Cursor_egg);

		// init variables
		PosY = 0;
		iHighscore = 0;

		if (BMusic_on) {
  			//start sound track
  			SetMasterVolume(130);
  			WaitVsync(10);
  			StartSong(Eastern_intro_song);		
        }
		break;


	  case	PM_Help:
		// init tile table
  		SetTileTable(BGTiles);

		// fill background with pattern
		for(uc1 = 0; uc1 < 15; uc1++)
		for(uc2 = 0; uc2 < 14; uc2++)
		DrawMap2(uc1 * 2,uc2 * 2,Pattern);

		// draw filled rectangle for text areas
		Fill(2,2,26,24,0);  
		
		// help text
		Print(3,3,PSTR("GET AS MANY EGGS IN YOUR"));		
		Print(3,4,PSTR("EGG-BASKET AS YOU CAN!"));		

		DrawMap2(2,7,Yellow_egg1);
		Fill(2,9,2,1,0);
		Print(5,7,PSTR("FREE CHICKEN FROM EGGS"));		
		Print(5,8,PSTR("EARN 10 POINTS PER EGG"));		

		DrawMap2(2,11,Green_egg1);
		Fill(2,13,2,1,0);
		Print(5,11,PSTR("DON'T CATCH THE ROTTEN"));		
		Print(5,12,PSTR("EGG OR ITS ODOR WILL"));		
		Print(5,13,PSTR("STUN YOU! (-20 POINTS)"));		

		DrawMap2(2,16,Grenade1);
		Fill(2,18,2,1,0);
		Print(5,16,PSTR("NEVER ALLOW A GRENADE"));		
		Print(5,17,PSTR("TO FALL ON THE GROUND"));		
		Print(5,18,PSTR("... YOU CAN GUESS WHY"));				

		DrawMap2(2,21,Carrot1);
		Fill(2,23,2,1,0);
		Print(5,21,PSTR("WILL DECREASE THE"));		
		Print(5,22,PSTR("AMOUNT OFF BROKEN EGGS"));		
		Print(5,23,PSTR("EARN 100 POINTS"));		
		break;


	case PM_Credits:

		// init tile table
  		SetTileTable(BGTiles);

		// fill background with pattern
		for(uc1 = 0; uc1 < 15; uc1++)
		for(uc2 = 0; uc2 < 14; uc2++)
		DrawMap2(uc1 * 2,uc2 * 2,Pattern);
		
		// draw filled rectangle for text areas
		Fill(4,4,22,20,0); 

		Print(9,6,PSTR("VERSION 1.0"));
        
		Print(5,9,PSTR("` HARTMUT WENDT 2012"));
        Print(5,11,PSTR("WWW.HWHARDSOFT.DE.VU"));  
        Print(8,13,PSTR("SOUNDTRACK BY"));
        Print(8,14,PSTR("CARSTEN KUNZ"));


        Print(8,17,PSTR("LICENSED UNDER"));
        Print(10,18,PSTR("GNU GPL V3"));
  
        Print(8,21,PSTR("PRESS ANY KEY"));

		break;

      case	PM_HoF_view:
	    // show the hall of fame

		SetSpriteVisibility(false);

		// init tile table
  		SetTileTable(BGTiles);


	    PosY = check_highscore();
	    if (PosY == 2) copy_highsore(1,2);
	    if (PosY == 1) {
		  copy_highsore(1,2);
		  copy_highsore(0,1);
        }
		clear_highsore(PosY - 1);

		// reset cursor to left position
		PosX = 0;

		// fill background with pattern
		for(uc1 = 0; uc1 < 15; uc1++)
		for(uc2 = 0; uc2 < 14; uc2++)
		DrawMap2(uc1 * 2,uc2 * 2,Pattern);
		
		// draw filled rectangle for text areas
		Fill(7,7,16,14,0); 

		// Print text and winners
		Print(9,8,PSTR("HALL OF FAME"));
        view_highscore_entry(8,12,1,!(PosY));
        view_highscore_entry(8,14,2,!(PosY));
        view_highscore_entry(8,16,3,!(PosY));  
		if (PosY)  {
		  mode = PM_HoF_edit;
		  Fill(7,2,16,4,0);	
		  Print(8,3,PSTR("CONGRATULATION"));	
		  Print(8,4,PSTR("NEW HIGHSCORE!"));	
		} 
		Print(8,19,PSTR("PRESS BUTTON A"));	
		break;	  

	  
	  case	PM_Gameplay:
		StopSong();
		   		
  		// init tile table
  		SetTileTable(BGTiles);

		// blank screen - no music
		StopSong();
		ClearVram();
	
		WaitVsync(2);

		// Draw background
		Fill(0,0,30,27,1);

		// Draw bottom
		for(uc1 = 0; uc1 < 29; uc1= uc1+2) DrawMap2(uc1,27,base);

		// Draw bar
		Fill(0,5,30,1,0x46);

		// Draw chicken
		for(uc1 = 0; uc1 < 5; uc1++) DrawMap2((uc1 * 3) + 7,0,Chicken_1);

		
		// init variables
		fill_buf(ani_count_chicken,chicken_max_animation,5);
		eggs[0] = default_egg;
		eggs[1] = default_egg;
		eggs[2] = default_egg;
		eggs[3] = default_egg;
		eggs[4] = default_egg;
		
		iHighscore = 0;
		broken_eggs = 4;
		X_pos_bunny = 110;
		iEgg_Timer = Max_egg_timer;
		iNew_Egg_Timer = Max_egg_timer / 10;

		SetSpriteVisibility(true);
		if (BMusic_on) {
  			//start sound track
  			SetMasterVolume(130);
  			WaitVsync(10);
  			StartSong(Eastern_game_song);		
        }
		break;
		



	}

	#ifdef _FadingX
    FadeIn(1, true);
	#endif

	program_mode = mode;

}




u8 GetTile(u8 x, u8 y)
// get background tile from vram
{

 return (vram[(y * 30) + x] - RAM_TILES_COUNT);

}



void Draw_egg_reserve(u8 eggs)
// draw reserve of eggs 
{
  for(u8 uc1=0; uc1 < 3; uc1++) {
  	// draw entire egg
    if (uc1 < eggs) DrawMap2(27 - (uc1 * 2),1,Entire_egg);
	// draw broken egg
	else DrawMap2(27 - (uc1 * 2),1,Broken_egg);
  }

}



bool collect_eggs(u8 egg) {
// collect an egg or other object
	
	if (eggs[egg].Enabled == false) return(false);

	// check vertical position
	if (eggs[egg].PosY < 27) return(false);

	// check horizontal position
	int XDiff = X_pos_bunny + 12 - (((egg * 3) + 8) * 8);	
	if ((XDiff < -8) || (XDiff > 8)) return(false);

	switch(eggs[egg].SpriteType) {
			
		case yellow_egg:
		    iHighscore += 10;
			TriggerFx(5, 0x7f, true);
			break;

		case green_egg:
			TriggerFx(7, 0x7f, true);
			if (iHighscore > 20) iHighscore-= 20;
			else iHighscore = 0;
			// confusion of easter bunny
			Confusion_Timer = Max_Confusion_Time; 

			break;

		case grenade:	
			TriggerFx(10, 0x7f, true);		
			break;

		case carrot:
			TriggerFx(9, 0x7f, true);
			iHighscore += 100;
			if (broken_eggs < 4) broken_eggs++;
			break;
	}		
	clear_egg(egg);
	return(true);
}


void new_egg(void) {
// start a new egg run
  u8 u1,u2;
  
  	// get random slot and object
	do 
	{	  
	  u2 = PRNG_NEXT() % 100;	  
	  u1 = u2 / 20;

	} while ((eggs[u1].Enabled) || (chicken_max_animation != ani_count_chicken[u1]));
	
	u2 = u2 % 20;
	if (u2 == 1) eggs[u1].SpriteType = carrot;
	else if ((u2 == 19) || (u2 == 2) || (u2 == 7)) eggs[u1].SpriteType = green_egg; 
	else if (u2 == 5) eggs[u1].SpriteType = grenade;
	else eggs[u1].SpriteType = yellow_egg;

	eggs[u1].PosY = 0;
	ani_count_chicken[u1] = 0;
}



void game_over(void) {
// show game over message
	Fill(11,12,8,4,0);
	Print(13,13,PSTR("GAME"));
	Print(13,14,PSTR("OVER"));

	// stop all animations
	fill_buf(ani_count_chicken,chicken_max_animation,5);
	eggs[0] = default_egg;
	eggs[1] = default_egg;
	eggs[2] = default_egg;
	eggs[3] = default_egg;
	eggs[4] = default_egg;

	// play gameover sound
	TriggerFx(8, 0x7f, true);
	WaitVsync(120);
  	if (0 == check_highscore()) set_PM_mode(PM_Intro);	  	
  	else set_PM_mode(PM_HoF_view);	

}



/**** A N I M A T I O N S ***************************************************************/

void animate_bunny(void) {
// animate the bunny
  // increment the bunny animation picture
  if (ani_count_bunny < bunny_max_animation) ani_count_bunny++;
  MapSprite2(0,ani_bunny[ani_count_bunny - 1],bunny_sprite_direction);		
  MoveSprite(0,X_pos_bunny,179,3,5);
}


void animate_chicken(u8 chicken, u8 *ani_step) {
// animate a chicken
  // increment the chicken animation picture
  if (*ani_step < chicken_max_animation) {
    DrawMap2((chicken * 3) + 7,0,ani_chicken[(*ani_step)++]);	   
  }	
  // start egg animation 		
  if (*ani_step == (chicken_max_animation - 2))eggs[chicken].Enabled =true;
}


void animate_egg(u8 egg) {
// animate egg
    u8 uc1 =0;
	if (eggs[egg].Enabled == false) return;

  	// increment the egg position
	eggs[egg].PosY++;


	// falling object
  	if (eggs[egg].PosY <= 29) {

		u8 egg_tile_pos	= eggs[egg].PosY / 3;
		u8 egg_map = eggs[egg].PosY % 3;
				
		// choose and draw the object
		switch(eggs[egg].SpriteType) {
			
			case yellow_egg:
				DrawMap2((egg * 3) + 7,(egg_tile_pos * 2) + 6,ani_yellow_egg[egg_map]);
				break;

			case green_egg:
				DrawMap2((egg * 3) + 7,(egg_tile_pos * 2) + 6,ani_green_egg[egg_map]);
				break;

			case grenade:
				DrawMap2((egg * 3) + 7,(egg_tile_pos * 2) + 6,ani_grenade[egg_map]);
				break;

			case carrot:
				DrawMap2((egg * 3) + 7,(egg_tile_pos * 2) + 6,ani_carrot[egg_map]);
				break;
		}

		if (egg_map == 0) {
		  uc1 = (egg_tile_pos * 2) + 5;
		  SetTile((egg * 3) + 7,uc1,1);
		  SetTile((egg * 3) + 8,uc1,1);
		}


	// touch down
	} else if (eggs[egg].PosY == 30) {
			
		switch(eggs[egg].SpriteType) {
			
			case yellow_egg:
				TriggerFx(6, 0x7f, true);
				if (broken_eggs) broken_eggs--;
			case green_egg:
				DrawMap2((egg * 3) + 7,25,Fried_egg);				
				break;

			case grenade:
				clear_egg(egg);
				game_over();
				break;

			case carrot:
				clear_egg(egg);
				break;
		}		

 	} else if (eggs[egg].PosY >= 35) clear_egg(egg);
	
	 	
}



void clear_egg(u8 egg) {
// clear egg
	// fill with background
	Fill((egg * 3) + 7,22,3,5,1);

	// restore bottom
	u8 egg_posx = egg * 3;
	if (egg_posx % 2) {
		DrawMap2(egg_posx + 7,27,base);
		DrawMap2(egg_posx + 9,27,base);

    } else {
		DrawMap2(egg_posx + 6,27,base);
		DrawMap2(egg_posx + 8,27,base);
	}
	// reset variables
	eggs[egg] = default_egg;

}




/**** S T U F F ********************************************************************/


void msg_window(u8 x1, u8 y1, u8 x2, u8 y2) {
// draw a window with frame and black backgound on the screen

    // window backgound
	Fill(x1 + 1, y1 + 1, x2 - x1 - 1, y2 - y1 - 1,401);
	// upper frame
	Fill(x1 + 1, y1, x2 - x1 - 1, 1,411);
	// lower frame
	Fill(x1 + 1, y2, x2 - x1 - 1, 1,410);
	// left frame
	Fill(x1 , y1 + 1, 1, y2 - y1 - 1,409);
	// right frame
	Fill(x2, y1 + 1, 1 , y2 - y1 - 1,409);
	// upper, left corner
	SetTile(x1,y1,405);
	// upper, right corner
	SetTile(x2,y1,406);
	// lower, left corner
	SetTile(x1,y2,407);
	// lower, right corner
	SetTile(x2,y2,408);
}	








/**
copy a buffer into another buffer 
@param source buffer
@param target buffer
@param count of copied bytes
@return none
*/
void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ)
{
 for(;ucANZ>0 ; ucANZ--)
 {
  *(BUFB++) = *(BUFA++);
 }   
}


/**
fill a buffer 
@param target buffer
@param byte to fill
@param count of copied bytes
@return none
*/
void fill_buf(u8 *BUFA, u8 content, u8 ucANZ)
{
 for(;ucANZ>0 ; ucANZ--)
 {
  *(BUFA++) = content;
 }   
}



int get_highscore(u8 entry) {
// get the actual highscore from eeprom
  // check the value for entry	
  if (entry > 2) return(0);
	
   // read the eeprom block
  if (!isEepromFormatted() || EepromReadBlock(24, &ebs))
        return(0);   
  return((ebs.data[(entry * 10)+8] * 256) + ebs.data[(entry * 10)+9]);
}



u8 check_highscore(void) {
// check the actual highsore
u8 a;
int i1;
   // read the eeprom block
  if (!isEepromFormatted() || EepromReadBlock(24, &ebs))
        return(0);   
  for(a=0; a<3; a++) {
    i1 = (ebs.data[(a * 10)+8] * 256) + ebs.data[(a * 10)+9];
    if (iHighscore > i1) return(a + 1);
  }

  // highscore is lower as saved highscores 
  return(0);
}



void copy_highsore(u8 entry_from, u8 entry_to) {
// copy a highscore entry to another slot
u8 a;
   // read the eeprom block
  for(a=0; a<10; a++) {
    ebs.data[(entry_to * 10) + a] = ebs.data[(entry_from * 10) + a];
  } 
}


void clear_highsore(u8 entry) {
// clear the name in actual entry and set the score to highscore
u8 a;
  // clear name 
  for(a=0; a<8; a++) {
    ebs.data[(entry * 10) + a] = 0x20;
  }   
  // set score
  ebs.data[(entry * 10) + 8] = iHighscore / 256;
  ebs.data[(entry * 10) + 9] = iHighscore % 256;
}



u8 set_def_highscore(void) {
// write the default highscore list in the EEPROM
  // entry 1
  ebs.data[0] = 'H';
  ebs.data[1] = 'A';
  ebs.data[2] = 'R';
  ebs.data[3] = 'T';
  ebs.data[4] = 'M';
  ebs.data[5] = 'U';
  ebs.data[6] = 'T';
  ebs.data[7] = ' ';
  ebs.data[8] = 0x07;
  ebs.data[9] = 0xD0;	
  // entry 2
  ebs.data[10] = 'M';
  ebs.data[11] = 'A';
  ebs.data[12] = 'R';
  ebs.data[13] = 'I';
  ebs.data[14] = 'E';
  ebs.data[15] = ' ';
  ebs.data[16] = ' ';
  ebs.data[17] = ' ';
  ebs.data[18] = 0x05;
  ebs.data[19] = 0xDC;	
  // entry 3
  ebs.data[20] = 'L';
  ebs.data[21] = 'U';
  ebs.data[22] = 'K';
  ebs.data[23] = 'A';
  ebs.data[24] = 'S';
  ebs.data[25] = ' ';
  ebs.data[26] = ' ';
  ebs.data[27] = ' ';
  ebs.data[28] = 0x03;
  ebs.data[29] = 0xE8;	
  return(EepromWriteBlock(&ebs));
}


u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data) {
// shows an entry of the higscore
u8 a,c;

  // read the eeprom block
  if (load_data)
  {
    if (!isEepromFormatted() || EepromReadBlock(24, &ebs))
        return(1);   
  }
  entry--;
  for(a = 0; a < 8;a++) {
	c = ebs.data[a + (entry * 10)];  
	PrintChar(x + a, y, c);  
  }
  PrintInt(x + 13, y, (ebs.data[(entry * 10)+8] * 256) + ebs.data[(entry * 10)+9], true);
  return(0);
}



void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode) {
// edit and view and char in the name of choosed entry    
entry--;
u8 c = ebs.data[(entry * 10) + cursor_pos];
  // proceed up & down button
  if (b_mode == BTN_UP) {
     c++;
     if (c > 'Z') c = ' '; 
     else if (c == '!') c = 'A';
  }
  if (b_mode == BTN_DOWN) {		 
     c--;      
     if (c == 0x1F) c = 'Z';
	 else if (c < 'A') c = ' ';
  }
  ebs.data[(entry * 10) + cursor_pos] = c;

}


void show_highscore_char(u8 entry, u8 position, u8 cursor_on) {
// shows a char of edited name
u8 c = ebs.data[(entry * 10) + position];
    if (cursor_on) PrintChar(8 + position, (entry * 2) + 12, '_');   // show '_'
    else if (c == ' ') PrintChar(8 + position, (entry * 2) + 12, ' ');	// space
    else PrintChar(8 + position, (entry * 2) + 12, c); 	
	
}






