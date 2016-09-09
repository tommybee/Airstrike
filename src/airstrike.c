#ifdef WIN32
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "SDL_mixer.lib")
#pragma comment(lib, "SDL_image.lib")
#endif

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "airstrike.h"
#include "sprite_types.h"
#include "config.h"
#include "text.h"
#include "sound.h"
#include "console.h"
#include "players.h"
#include "mech.h"
#include "level.h"
#include "winds.h"
#include "math.h"

sprite_group_t *mech_group;
sprite_group_t *bullet_group;
sprite_group_t *effects_group;
sprite_group_t *foreground_group;
sprite_group_t *ui_group;

static int paused = 0;
static int max_points = 6; /* actually one more than the end score */
static Uint32 displayflags = 0;
static int show_debug = 0; /* If true print dbg info on screen */
static Uint32 frame_times[32];
static int frame_time_i = 0;
static int screen_w;
static int screen_h;


static void atexit_cleanup(void)
{
  /* Since we don't want a crash in fullscreen mode
     we hope that this will bring us back to a window */
  SDL_SetVideoMode(screen_w, screen_h, 0, 0);
}

static int general_setup(void)
{
  int i;

  maths_setup();
  config_setup();

  screen_w = cfgnum("screen.width",800);
  screen_h = cfgnum("screen.height",600);

  if (cfgnum("fullscreen",0) == 1)
    displayflags |= SDL_FULLSCREEN;

  sprite_global.opt_alpha = cfgnum("alpha",0);

  i = SDL_Init(SDL_INITFLAGS);
  assert(i == 0);

  atexit(atexit_cleanup);
  
  SDL_WM_SetCaption(VERSIONSTRING,0);
  sprite_global.display = 
    SDL_SetVideoMode(screen_w, screen_h, 0, displayflags);
  assert(sprite_global.display);
  SDL_ShowCursor(SDL_DISABLE);

  if (cfgnum("sound",0) == 1)
    {
      sound_setup();
      /* place the ear on the ground, in the center */
      sound_move_ear(screen_w/2,screen_h);
    }
  sprite_setup();
  text_setup();
  console_setup();
  console_set_pos(9,254);
  console_load_bg(path_to_data("console-bg.png"));
  sprite_types_setup();
  sprite_background_load("data/bg.png","data/bgmask.png");

  level_setup();
  winds_setup();
		 
  return 0;
}

void players_setup(void)
{
  int nr_humans;
  sprite_t *sp;
  nr_humans = cfgnum("nr_players",1);
  assert((nr_humans >= 0) && (nr_humans <= 2));
  player_sprite_type[0] = &blueplane;
  player_sprite_type[1] = &biplane;
  player_startpos[0][0] = 700;
  player_startpos[0][1] = 500;
  player_startpos[1][0] = 5;
  player_startpos[1][1] = 300;
  player_points[0] = max_points;
  player_points[1] = max_points;

  if (nr_humans > 0)
    player_sethuman(0);
  else
    player_setai(0);
  
  if (nr_humans == 2)
    player_sethuman(1);
  else
    player_setai(1);
}

void engine_setup(void)
{
  mech_group = sprite_group_create();
  bullet_group = sprite_group_create();
  effects_group = sprite_group_create();
  foreground_group = sprite_group_create();
  ui_group = sprite_group_create();
}

void objects_setup(void)
{
  int i,x,y;
  sprite_t *sp;
  float v[2];

  srand(23);

  sp = sprite_create(&bonusmachine);
  sprite_group_insert(mech_group,sp);
  sprite_set_pos(sp,level.xmax/2,15);
  
  for (i=0;i<3;i++)
    {
      sp = sprite_create(&bird);
      sprite_group_insert(mech_group,sp);
    }

  for (i=0;i<5;i++)
    {
      sp = sprite_create(&bouncer);
      sprite_group_insert(mech_group,sp);
      sprite_set_pos(sp,rand() % 700 + 50,rand() % 200 + 100);
    }
  
  sp = sprite_create(&cloud);
  sprite_group_insert(foreground_group,sp);
  sprite_set_pos(sp,400,70);
  v[0] = 15; v[1] = 0;
  sprite_set_vel(sp,v);

  sp = sprite_create(&cloud);
  sprite_group_insert(foreground_group,sp);
  sprite_set_pos(sp,0,150);
  v[0] = 10; v[1] = 0;
  sprite_set_vel(sp,v);

  sp = sprite_create(&zeppelin);
  sprite_group_insert(mech_group,sp);
  sprite_set_pos(sp,50,200);

  sp = sprite_create(&airballoon);
  sprite_group_insert(mech_group,sp);
  sprite_set_pos(sp,100,150);
  v[0] = 5; v[1] = 3;
  sprite_set_vel(sp,v);
  
  sp = sprite_create(&cannon);
  sprite_group_insert(mech_group,sp);
  sprite_set_pos(sp,380,518);
}

static int process_events(void)
{
  SDL_Event event;
  int ret = 1;
  while ( SDL_PollEvent(&event) >= 1 ) 
    {
      switch (event.type) 
	{
	case SDL_QUIT: 
	  ret = 0;
	  break;
	case SDL_KEYDOWN:
	  switch (event.key.keysym.sym)
	    {
	    case SDLK_ESCAPE:
	      paused = 1 - paused;
	      break;
	    case SDLK_s:
	      SDL_SaveBMP(sprite_global.display,"screenshot.bmp");
	      break;
	    default:
	      break;
	    }
	  break;
	default:
	  break;
	}
    }
  return ret;
}

static int wait_for_key(void)
{
  SDL_Event event;
  while (1)
    {
      while ( SDL_PollEvent(&event) >= 1 ) 
	{
	  switch (event.type) 
	    {
	    case SDL_QUIT: 
	      exit(EXIT_SUCCESS);
	      break;
	    case SDL_KEYDOWN:
	      return event.key.keysym.sym;
	      break;
	    default:
	      break;
	    }
	}
      SDL_Delay(sprite_global.display_dt);
    }
}

void draw_ui(void)
{
  char cbuf[256];
  float av_dt,s_dt;
  int i;
  /* left player stats */
  text_render(sprite_global.display,0,big_font,
	      54,sprite_global.display->h - 27,
	      ALIGN_LEFT,ALIGN_BOTTOM,
	      player_name[1]);
  if (sprite_isvalid(&player_sprite[1]))
    {
      sprite_signal(player_sprite[1],SIGNAL_STATSTRING,cbuf);
      text_render(sprite_global.display,0,small_font,
		  54,sprite_global.display->h - 17,
		  ALIGN_LEFT,ALIGN_BOTTOM,cbuf);
    }
  /* right player stats */
  text_render(sprite_global.display,0,big_font,
	      sprite_global.display->w - 54,
	      sprite_global.display->h - 27,
	      ALIGN_RIGHT,ALIGN_BOTTOM,player_name[0]);
  if (sprite_isvalid(&player_sprite[0]))
    {
      sprite_signal(player_sprite[0],SIGNAL_STATSTRING,cbuf);
      text_render(sprite_global.display,0,small_font,
		  sprite_global.display->w - 54,
		  sprite_global.display->h - 17,
		  ALIGN_RIGHT,ALIGN_BOTTOM,cbuf);
    }
  /* score */
  sprintf(cbuf,"%2.2i-%2.2i\nSCORE",max_points - 1 - player_points[0],
	  max_points - 1 - player_points[1]);
  text_render(sprite_global.display,0,big_font,
	      sprite_global.display->w/2,
	      sprite_global.display->h - 5,
	      ALIGN_CENTER,ALIGN_BOTTOM,cbuf);
  if (show_debug)
    {
      /* calculate frame time stats */
      av_dt = 0;
      s_dt = 0;
      for (i=0;i<32;i++)
	av_dt += frame_times[i];
      av_dt /= 32;
      for (i=0;i<32;i++)
	s_dt += (frame_times[i] - av_dt)*(frame_times[i] - av_dt);
      s_dt = sqrt(s_dt/32);
      sprintf(cbuf,"average dt = %3.2f (stddev = %3.2f)",av_dt,s_dt);
      text_render(sprite_global.display,0,small_font,
		  10,10,ALIGN_LEFT,ALIGN_TOP,cbuf);
    }
}

/* This draws things when we are in console (paused) mode */
void console_frame(void)
{  
  sprite_start_frame();     
  sprite_group_draw(mech_group);
  sprite_group_draw(bullet_group);
  sprite_group_draw(effects_group);
  sprite_group_draw(foreground_group);
  sprite_group_draw(ui_group);
  
  winds_change();
  
  draw_ui();
  console_draw();
  sprite_end_frame();
}

/* choices is null terminated. Return the
   index of the choosen item, or -1 if escape was
   pressed.*/
int select_mode(char *header, char *choices[])
{
  int i,key,choice = 0;
  while (1)
    {
      console_clear();
      console_write(header);
      console_write("\n");
      for (i=0;choices[i];i++)
	{
	  if (choice == i)
	    console_write("*");
	  else
	    console_write(" ");
	  console_write(choices[i]);
	  console_write("\n");
	}
      console_frame();
      key = wait_for_key();
      switch (key)
	{
	case SDLK_UP:
	  choice--;
	  if (choice < 0)
	    { 
	      /* seek last item */
	      choice = 0;
	      while (choices[choice + 1] != 0)
		choice++;
	    }
	  break;
	case SDLK_DOWN:
	  choice++;
	  if (choices[choice] == 0)
	    choice = 0;
	  break;
	case SDLK_ESCAPE:
	  return -1;
	default:
	  return choice;
	}
    }
}

void player_keys_mode(int p)
{
  int i;
  int key;
  char cbuf[2] = {0,0};
 again:
  console_clear();
  console_write("Setting up key bindings,\nhit the appropriate keys.\n");
  i = 0;
  while (control_key_names[i].signal)
    {
      console_write(control_key_names[i].name);
      console_write(" [");
      console_write(SDL_GetKeyName((SDLKey) player_keymap[p][i][0]));
      console_write("] ");
      console_frame();
      key = wait_for_key();
      console_write(SDL_GetKeyName((SDLKey) key));
      console_write("\n");
      player_keymap[p][i][0] = key;
      player_keymap[p][i][1] = control_key_names[i].signal;
      i++;
    }
  console_write("Done. Press any key to continue.\n");
  console_frame();
  key = wait_for_key();
}

void player_setup_mode(void)
{
  char *items[] = {0,0,"Left player keys",
		   "Right player keys","Go back",0};
  while (1)
    {
      if (player_ishuman[1])
	items[0] = "Left Player: Keyboard";
      else
	items[0] = "Left Player: AI";
      if (player_ishuman[0])
	items[1] = "Right Player: Keyboard";
      else
	items[1] = "Right Player: AI";

      switch (select_mode("Player setup",items))
	{
	case 0:
	  player_ishuman[1] = 1 - player_ishuman[1];
	  if (player_ishuman[1])
	    player_sethuman(1);
	  else
	    player_setai(1);
	  break;
	case 1:
	  player_ishuman[0] = 1 - player_ishuman[0];
	  if (player_ishuman[0])
	    player_sethuman(0);
	  else
	    player_setai(0);
	  break;
	case 2:
	  player_keys_mode(1);
	  break;
	case 3:
	  player_keys_mode(0);
	  break;
	default:
	  return;
	}
    }
}

/* Display the message and wait for a keypress */
void message_mode(char *message)
{
  sprite_start_frame();     
  sprite_group_draw(mech_group);
  sprite_group_draw(bullet_group);
  sprite_group_draw(effects_group);
  sprite_group_draw(foreground_group);
  sprite_group_draw(ui_group);
  text_render(sprite_global.display, 0, big_font,
	      sprite_global.display->w/2, sprite_global.display->h/2,
	      ALIGN_CENTER, ALIGN_CENTER, message);  
  sprite_end_frame();
  sprite_dirty_all();
  wait_for_key();
}

void about_mode(char *filename)
{
  FILE *message = fopen(path_to_data(filename),"r");
  char cbuf[2001];
  size_t n;
    
  if (message)
    {
      n = fread(cbuf,1,2000,message);
      cbuf[n] = 0;
      fclose(message);
    }
  else
    {
      sprintf(cbuf,"Error: could not open '%s'.",filename);
    }
  message_mode(cbuf);
}

void console_mode()
{
  char *items[] = 
    {
      "Resume game",
      "Players setup",
      "Toggle fullscreen",
      "About the game",
      "Release notes",
      "Toggle debug mode",
      "Quit game",
      0
    };
  while (1) 
    {
      switch (select_mode("Airstrike menu",items))
	{
	case 0:
	  return;
	  break;
	case 1:
	  player_setup_mode();
	  break;
	case 2:
	  displayflags ^= SDL_FULLSCREEN;
	  sprite_global.display = 
	    SDL_SetVideoMode(screen_w, screen_h, 0, displayflags);
	  assert(sprite_global.display);
	  sprite_dirty_all();
	  sprite_bg_dirty_all();
	  break;
	case 3:
	  about_mode("about.txt");
	  break;
	case 4:
	  about_mode("notes.txt");
	  break;
	case 5:
	  show_debug = 1 - show_debug;
	  break;
	case 6:
	  exit(EXIT_SUCCESS);
	  break;
	default:
	  return;
	  break;
	}
    }
}

/* Keeps track of dead planes, recreates them and award points */
void scorekeeper()
{
  char cbuf[200];
  sprite_t *s;
  int i;

  /* This loop relies on MAXPLAYERS = 2 (great programming..)*/
  for (i=0;i<MAXPLAYERS;i++)
    {
      if (!sprite_isvalid(&player_controller[i]->target))
	{
	  player_points[i]--;
	  if (player_points[i] <= 0)
	    {
	      if (i == 0)
		sprintf(cbuf,"And the winner is %s, by %i point(s)",
			player_name[1],player_points[1]);
	      else
		sprintf(cbuf,"And the winner is %s, by %i point(s)",
			player_name[0],player_points[0]);
	      message_mode(cbuf);
	      sprite_kill(player_sprite[0]);
	      sprite_kill(player_sprite[1]);
	      player_points[0] = max_points;
	      player_points[1] = max_points;
	      return;
	    }
	  else
	    {
	      sprite_release(&player_sprite[i]);
	      s = sprite_create(player_sprite_type[i]);
	      sprite_aquire(s,&player_sprite[i]);
	      sprite_aquire(player_sprite[i],
			    &player_controller[i]->target);
	      assert(sprite_isvalid(&player_controller[i]->target));
	      s = player_sprite[i];
	      sprite_group_insert(mech_group,s);
	      sprite_set_pos(s,player_startpos[i][0],player_startpos[i][1]);
	      if (!player_ishuman[i])	
		ai_controller_set_enemy(player_controller[i],
					player_sprite[1 - i]);
	      if (!player_ishuman[1-i])
		{
		  ai_controller_set_enemy(player_controller[1-i],
					  player_sprite[i]);
		}
	      s = sprite_create(&energymeter);	  
	      sprite_set_pos(s,
			     (sprite_global.display->w - 60)*(1 - i) + 30,
			     sprite_global.display->h - 30);
	      sprite_signal(s,SIGNAL_SPRITETARGET,player_sprite[i]);
	      sprite_group_insert(ui_group,s);
	    }
	}
    }
}

/* Creates a new game frame */
void game_frame()
{
  int i;
  /* collect frame time statistics */
  static Uint32 lasttime = 0;
  Uint32 now = SDL_GetTicks();
  frame_times[frame_time_i] = now - lasttime;
  frame_time_i = (frame_time_i + 1) & 31;
  lasttime = now;

  /*  sprite_viewport_center_on(player_sprite[0]);*/

  sprite_start_frame();  
  
  sprite_group_move(mech_group, sprite_global.dt);
  sprite_group_move(bullet_group, sprite_global.dt);
  sprite_group_move(effects_group, sprite_global.dt);
  sprite_group_move(foreground_group, sprite_global.dt);
  
  sprite_group_animate(mech_group, sprite_global.dt);
  sprite_group_animate(bullet_group, sprite_global.dt);
  sprite_group_animate(effects_group, sprite_global.dt);
  sprite_group_animate(foreground_group, sprite_global.dt);

  mech_gravity(mech_group); /* has to be before bg_coll */
  
  sprite_group_coll(mech_group,mech_sprite_collide);
  sprite_group_coll2(bullet_group,mech_group,0);
  sprite_group_bg_coll(mech_group,mech_sprite_bg_collide);
  sprite_group_bg_coll(bullet_group,0);
  
  for (i=0;i<MAXPLAYERS;i++)
    {
      player_controller[i]->update(player_controller[i]);
    }

  sprite_group_update(mech_group);
  sprite_group_update(bullet_group);
  sprite_group_update(effects_group);
  sprite_group_update(foreground_group);
  sprite_group_update(ui_group);
  
  sprite_group_cleanup(bullet_group);
  sprite_group_cleanup(mech_group);
  sprite_group_cleanup(effects_group);
  sprite_group_cleanup(foreground_group);
  sprite_group_cleanup(ui_group);
  
  /* Refresh sprite positions to reflect changes
     by collisions */
     
  sprite_group_pos_update(mech_group);

  sprite_group_draw(mech_group);
  sprite_group_draw(bullet_group);
  sprite_group_draw(effects_group);
  sprite_group_draw(foreground_group);
  sprite_group_draw(ui_group);
  
  draw_ui();

  if (!sprite_end_frame()) 
    {
      //printf("lag");
      //fflush(stdout);
    }
  scorekeeper();
}

/* Dirty function to save an animation of the game */
void saveanimframe()
{
  static int n = 0;
  static SDL_Surface *img = 0;
  SDL_Rect r;
  char filename[100];
  r.x = 325;
  r.y = 458;
  r.w = 256;
  r.h = 96;
  if (!img)
    {
      img = SDL_CreateRGBSurface(SDL_SWSURFACE,
				 r.w,r.h,8,0,0,0,0);
      img = SDL_DisplayFormat(img);
    }
  SDL_BlitSurface(sprite_global.display,&r,img,0);
  sprintf(filename,"%05i.bmp",n++);
  SDL_SaveBMP(img,filename);
}

int main(int argc, char *argv[])
{
  int res;
  if (argc > 1)
    {
      printf("All options are currently given in the config files, located at $HOME/.airstrikerc and $PWD/airstrikerc. Quitting.\n");
      exit(EXIT_SUCCESS);
    }
  res = general_setup() == 0;
  assert(res);
  engine_setup(); 
  message_mode("      Airstrike 1.0 pre 6\n\nIn the game press ESC for a menu\n  Winner is first to 5 points\n     Press any key to start");
  objects_setup();
  players_setup();  
  fprintf(stderr,"Entering main loop.\n");
  while(process_events()) 
    {
      if (!paused)
	{
	  game_frame();
	}
      else
	{
	  console_mode();
	  paused = 0;
	}
    }
  /* We probably never get here right now, because of exit() calls. */
  fprintf(stderr,"Left main loop.\n");
  sprite_group_free(mech_group);
  sprite_group_free(bullet_group);
  sprite_group_free(effects_group);
  sprite_group_free(foreground_group);
  return EXIT_SUCCESS;
}
