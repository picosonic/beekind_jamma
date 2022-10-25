//=============================================================================
//	FILE:					main.cpp
//	SYSTEM:
//	DESCRIPTION:
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C)Copyright 2022 Jasper Renow-Clarke. All Rights Reserved.
//	LICENCE:			MIT
//=============================================================================

#include <random>
#include <cmath>
#include <array>
#include "engine/engine.h"
//#include "engine/api.h"
#include "game_config.h"
#include "generated/game_assets.h"

#if defined(JAMMAGAME_PORT_SDL)
#include "generated/game_gbin.h"
#endif

#include "levels.h"
#include "font.h"
#include "timeline.h"

// Global constants
#define FPS 30

#define XMAX 320
#define YMAX 240
#define TILESIZE 16
#define TILESPERROW 10
#define BLACKCOLOUR 0, 0, 0
#define BGCOLOUR 252,223,205
#define DEBUGTXTCOLOUR 0, 0, 0, 128


#define STATEINTRO 0
#define STATEMENU 1
#define STATEPLAYING 2
#define STATENEWLEVEL 3
#define STATECOMPLETE 4

#define HEALTHZOMBEE 10
#define HEALTHGRUB 5
#define HEALTHPLANT 2
#define GROWTIME (15*FPS)
#define SPEEDBEE 0.5
#define SPEEDZOMBEE 0.25
#define SPEEDGRUB 0.25

#define SPAWNTIME (8*FPS)
#define MAXFLIES 15
#define MAXBEES 20

// Tiles list
//
// blanks
//   14, 17
// switcheroo
//   0
// JS13k logo
//   10
// clouds
//   1, 2 (big)
//   11 (small)
//   12 (double)
// lines
//   3 (top left)
//   4 (top)
//   5 (top right)
//   13 (left)
//   15 (right)
//   6 (earth top left)
//   7 (earth top)
//   8 (earth top right)
//   9 (small top fade)
//   16 (left fade)
//   18 (right fade)
//   19 (small top)
//   20 (thick top 1)
//   21 (thick top 2)
//   22 (thick top 3)
//   23 (bottom left)
//   24 (bottom right)
//   25 (earth bottom left)
//   26 (earth bottom right)
//   27 (top left fade)
//   28 (top right fade)
//   29 (small bottom fade)
// toadstool
//   30 (tall)
//   31 (short)
// flower
//   32 (double)
//   33
// plant
//   34 (double)
//   35 (tall)
// hive
//   36
//   37 (broken)
// tree
//   38 (big crown)
//   39 (dual branch)
//   47 (branch left)
//   48 (trunk)
//   49 (branch right)
//   57 (root left)
//   58 (root)
//   59 (root right)
// gun
//   50
// bee
//   51, 52
// zombee
//   53, 54
// grub
//   55, 56
// player
//   40, 41, 42 (with gun)
//   45, 46
// muzzle flash
//   43
// projectile
//   44

// Convenience macros
#define Math_floor(VAL) (static_cast<int>(floor(VAL)))

void intro(const float percent);
void endgame(const float percent);

static jammagame::assets::TileSet	sg_builtin_font;

// Character attributes
struct gamechar
{
	uint8_t id; // tile id
	float x; // x position
	float y; // y position
	bool flip; // if char is horizontally flipped
	float hs; // horizontal speed
	float vs; // vertical speed
	int32_t dwell; // time (in frames) to dwell before next AI
	int32_t htime; // hurt timer
	bool del; // if char needs deleting

	int32_t health; // remaining health
	int32_t growtime; // time (in frames) until next growth
	int32_t pollen; // amount of pollen carried/stored

	int32_t dx; // destination x position
	int32_t dy; // destination y position
	std::vector<int16_t> path; // pathfinding set of nodes
};

// Gun shots
struct shot
{
	uint8_t id; // tile id
	float x; // x position
	float y; // y position
	bool flip; // if char is horizontally flipped
	int8_t dir; //direction (-1=left, 0=none, 1=right)
	int32_t ttl; // time (in frames) to live
	bool del; // if shot needs deleting
};

// Particles
struct particle
{
	float x; // x position
	float y; // y position
	float ang; // angle
	float t; // travel (from centre point)

	uint8_t r; // Red
	uint8_t g; // Green
	uint8_t b; // Blue
	float a; // Alpha

	uint8_t s; // size of "chunk"
};

// Parallax
struct parallax
{
	uint32_t t; // type (tile group)
	float x; // x position
	float y; // y position
	float z; // z position
};

// Message box queue
struct msgboxitem
{
	std::string msgboxtext; // text to show in current messagebox
	uint32_t msgboxtime; // timer for showing current messagebox
};

// Spawn point
struct spawnpoint
{
	float x; // x position
	float y; // y position
};

// Game state
struct gamestate
{
	// physics in pixels per frame @ 60fps
	float gravity;
	float terminalvelocity;
	float friction;

	jammagame::gfx::Surface *surface;

	// Main character
	float x; // x position
	float y; // y position
	float px; // previous x position
	float py; // previous y position
	float sx; // start x position (for current level)
	float sy; // start y position (for current level)
	float vs; // vertical speed
	float hs; // horizontal speed
	bool jump; // jumping
	bool fall; // falling
	bool duck; // ducking
	int32_t htime; // hurt timer following enemy collision
	int32_t invtime; // invulnerable time following JS13k collection
	int8_t dir; //direction (-1=left, 0=none, 1=right)
	float hsp; // max horizontal speed
	float vsp; // max vertical speed
	float speed; // walking speed
	float jumpspeed; // jumping speed
	int32_t coyote; // coyote timer (time after leaving ground where you can still jump)
	int32_t life; // remaining "life force" as percentage
	uint8_t tileid; // current player tile
	bool flip; // if player is horizontally flipped
	bool gun; // if the player holds the gun
	std::vector<struct shot> shots; // an array of shots from the gun
	uint32_t gunheat; // countdown to next shot

	// Level attributes
	uint8_t level; // Level number (0 based)
	uint8_t width; // Width in tiles
	uint8_t height; // height in tiles
	int32_t xoffset; // current view offset from left (horizontal scroll)
	int32_t yoffset; // current view offset from top (vertical scroll)
	bool topdown; // is the level in top-down mode, otherwise it's 2D platformer
	int32_t spawntime; // time in frames until next spawn event

	// Characters
	std::vector<struct gamechar> chars;
	int32_t anim; // time until next character animation frame

	// Particles
	std::vector<struct particle> particles;

	// Parallax
	std::vector<struct parallax> parallax;

	// Game state
	uint8_t state; // state machine, 0=intro, 1=menu, 2=playing, 3=complete
	
	// Messagebox popup
	std::string msgboxtext; // text to show in current messagebox
	uint32_t msgboxtime; // timer for showing current messagebox
	std::vector<struct msgboxitem> msgqueue; // Message box queue

	// music
	// TODO
};

struct gamestate gs;

#include "pathfinder.h"

// Random number generator
double
rng()
{
  return ((double) rand() / (RAND_MAX));
}

void
reset_gamestate()
{
	gs.gravity=0.25;
	gs.terminalvelocity=10;
	gs.friction=1;

	gs.x=0;
	gs.y=0;
	gs.px=0;
	gs.py=0;
	gs.sx=0;
	gs.sy=0;
	gs.vs=0;
	gs.hs=0;
	gs.jump=false;
	gs.fall=false;
	gs.duck=false;
	gs.htime=0;
	gs.invtime=0;
	gs.dir=0;
	gs.hsp=1;
	gs.vsp=1;
	gs.speed=2;
	gs.jumpspeed=5;
	gs.coyote=0;
	gs.life=100;
	gs.tileid=45;
	gs.flip=false;
	gs.gun=false;
	gs.shots.clear();
	gs.gunheat=0;

	gs.level=0;
	gs.width=0;
	gs.height=0;
	gs.xoffset=0;
	gs.yoffset=0;
	gs.topdown=false;
	gs.spawntime=SPAWNTIME;

	gs.chars.clear();
	gs.anim=8;

	gs.particles.clear();

	gs.parallax.clear();

	gs.msgboxtext.clear();
	gs.msgboxtime=0;
	gs.msgqueue.clear();
}

void
drawsprite(const uint8_t id, const float x, const float y, const bool flip)
{
	// Don't draw sprite 0 (background)
	if (id==0) return;

	// Clip to what's visible
	if (((x-gs.xoffset)<-TILESIZE) || // clip left
		((x-gs.xoffset)>XMAX) || // clip right
		((y-gs.yoffset)<-TILESIZE) || // clip top
		((y-gs.yoffset)>YMAX))   // clip bottom
	return;

	gs.surface->image({Math_floor(x)-gs.xoffset, Math_floor(y)-gs.yoffset}, id, flip?2:1);
}

// Scroll level to player
void
scrolltoplayer(const bool dampened)
{
	float xmiddle=Math_floor((XMAX-TILESIZE)/2);
	float ymiddle=Math_floor((YMAX-TILESIZE)/2);
	uint32_t maxxoffs=((gs.width*TILESIZE)-XMAX);
	uint32_t maxyoffs=((gs.height*TILESIZE)-YMAX);

	// Work out where x and y offsets should be
	float newxoffs=gs.x-xmiddle;
	float newyoffs=gs.y-ymiddle;

	if (newxoffs>maxxoffs) newxoffs=maxxoffs;
	if (newyoffs>maxyoffs) newyoffs=maxyoffs;

	if (newxoffs<0) newxoffs=0;
	if (newyoffs<0) newyoffs=0;

	// Determine if xoffset should be changed
	if (newxoffs!=gs.xoffset)
	{
		if (dampened)
		{
			uint32_t xdelta=1;

			if (abs(gs.xoffset-newxoffs)>(XMAX/5)) xdelta=4;

			gs.xoffset+=newxoffs>gs.xoffset?xdelta:-xdelta;
		}
		else
			gs.xoffset=newxoffs;
	}

	// Determine if xoffset should be changed
	if (newyoffs!=gs.yoffset)
	{
		if (dampened)
		{
			uint32_t ydelta=1;

			if (abs(gs.yoffset-newyoffs)>(YMAX/5)) ydelta=4;

			gs.yoffset+=newyoffs>gs.yoffset?ydelta:-ydelta;
		}
		else
			gs.yoffset=newyoffs;
	}
}

// Sort the chars so sprites are last (so they appear in front of non-solid tiles)
bool
sortChars(const struct gamechar & a, const struct gamechar & b)
{
	if (a.id!=b.id) // extra processing if they are different ids
	{
		bool aspr=(((a.id>=40) && (a.id<=46)) || ((a.id>=50) && (a.id<=56))); // see if a is a sprite
		bool bspr=(((b.id>=40) && (b.id<=46)) || ((b.id>=50) && (b.id<=56))); // see if b is a sprite

		if (aspr==bspr) return false; // both sprites

		if (bspr)
			return true; // sort a before b
	}

	return false; // same id
}

// Load level
void
loadlevel()
{
	// Make sure it exists
	if (levels.size()-1<gs.level) return;

	// Get width/height of new level
	gs.width=levels[gs.level].width;
	gs.height=levels[gs.level].height;

	gs.chars.clear();

	// Populate chars (non solid tiles)
	for (int y=0;y<gs.height;y++)
	{
		for (int x=0;x<gs.width;x++)
		{
			uint8_t tile=levels[gs.level].chars[(y*gs.width)+x];
			if (tile!=0)
			{
				struct gamechar obj;

				obj.id=tile-1;
				obj.x=x*TILESIZE;
				obj.y=y*TILESIZE;
				obj.flip=false;
				obj.hs=0;
				obj.vs=0;
				obj.dwell=0;
				obj.htime=0;
				obj.del=false;
				obj.health=0;

				switch (tile-1)
				{
					case 40: // Player
					case 41:
					case 42:
					case 45:
					case 46:
						gs.x=obj.x; // Set current position
						gs.y=obj.y;

						gs.sx=obj.x; // Set start position
						gs.sy=obj.y;

						gs.vs=0; // Start not moving
						gs.hs=0;
						gs.jump=false;
						gs.fall=false;
						gs.dir=0;
						gs.flip=false;
						gs.gun=false;
						gs.shots.clear();
						gs.gunheat=0;
						gs.particles.clear();
						gs.topdown=false;
						gs.spawntime=SPAWNTIME;
						break;

					case 30: // toadstool
					case 31:
						obj.health=HEALTHPLANT;
						obj.growtime=(GROWTIME+Math_floor(rng()*120));
						gs.chars.push_back(obj);
						break;

					case 32: // flower
					case 33:
						obj.health=HEALTHPLANT;
						obj.growtime=(GROWTIME+Math_floor(rng()*120));
						gs.chars.push_back(obj);
						break;

					case 53: // zombee
					case 54:
						obj.health=HEALTHZOMBEE;
						obj.pollen=0;
						obj.dx=-1;
						obj.dy=-1;
						obj.dwell=Math_floor(rng()*FPS);
						obj.path.clear();
						gs.chars.push_back(obj);
						break;

					case 51: // bee
					case 52:
						obj.pollen=0;
						obj.dx=-1;
						obj.dy=-1;
						obj.dwell=Math_floor(rng()*FPS);
						obj.path.clear();
						gs.chars.push_back(obj);
						break;

					case 36: // hive
					case 37:
						obj.pollen=0;
						gs.chars.push_back(obj);
						break;

					case 55: // grub
					case 56:
						obj.health=HEALTHGRUB;
						obj.hs=(rng()<0.5)?0.25:-0.25;
						obj.flip=(obj.hs<0);
						gs.chars.push_back(obj);
						break;

					default:
						gs.chars.push_back(obj); // Everything else
						break;
				}
			}
		}
	}

	// Sort chars such sprites are at the end (so are drawn last, i.e on top)
	std::sort(gs.chars.begin(), gs.chars.end(), sortChars);

	// Populate parallax field
	gs.parallax.clear();
	for (int i=0; i<4; i++)
		for (int z=1; z<=2; z++)
		{
			struct parallax p;

			p.t=Math_floor(rng()*3);
			p.x=Math_floor((rng()*gs.width)*TILESIZE);
			p.y=Math_floor((rng()*(gs.height/2))*TILESIZE);
			p.z=z*10;

			gs.parallax.push_back(p);
		}

	// Move scroll offset to player with damping disabled
	scrolltoplayer(false);
}

void
write(const float x, const float y, const std::string & text, const uint8_t size, const uint8_t r, const uint8_t g, const uint8_t b, const float a)
{
	gs.surface->set_colour(jammagame::gfx::colour::colour(r, g, b, (a*255)));

	for (uint8_t i=0; i<text.length(); i++)
	{
		int16_t offs=(text[i]-32);

		// Don't try to draw characters outside our font set
		if ((offs<0) || (offs>94))
			continue;

		// Draw "pixels"
		uint16_t px=0;
		uint16_t py=0;

		// Iterate through the 4 bytes (columns) used to define character
		for (uint8_t j=0; j<font_width; j++)
		{
			uint8_t dual=font_8bit[(offs*font_width)+j];

			// Iterate through bits in byte
			for (uint8_t k=0; k<font_height; k++)
			{
				if (dual&(1<<(font_height-k)))
					gs.surface->solid_rectangle({Math_floor(x+(i*font_width*size)+(px*size)), Math_floor(y+(size*py)), size, size});

				px++;
				if (px==font_width)
				{
					px=0;
					py++;
				}
			}
		}
	}
}

// Draw level
void
drawlevel()
{
	for (int y=0;y<gs.height;y++)
		for (int x=0;x<gs.width;x++)
		{
			uint8_t tile=levels[gs.level].tiles[(y*gs.width)+x];
			if (tile>0)
				drawsprite(tile-1, x*TILESIZE, y*TILESIZE, false);
		}

	// Draw level number
	write(10, 10, std::string("Level ")+std::to_string(gs.level+1), 1, 0, 0, 0, 1);
}

// Draw chars
void
drawchars()
{
	for (uint32_t id=0; id<gs.chars.size(); id++)
	{
		drawsprite(gs.chars[id].id, gs.chars[id].x, gs.chars[id].y, gs.chars[id].flip);

		// Draw health bar
		if (((gs.chars[id].health)>0) && ((gs.chars[id].htime)>0))
		{
			float hmax=0;

			switch (gs.chars[id].id)
			{
				case 30:
				case 31:
					hmax=HEALTHPLANT;
					break;

				case 53:
				case 54:
					hmax=HEALTHZOMBEE;
					break;

				case 55:
				case 56:
					hmax=HEALTHGRUB;
					break;

				default:
					break;
			}

			if (hmax>0)
			{
				gs.surface->set_colour(jammagame::gfx::colour::colour(0, 255, 0, (0.75*255)));
				gs.surface->solid_rectangle({Math_floor(gs.chars[id].x)-gs.xoffset, Math_floor(gs.chars[id].y)-gs.yoffset, Math_floor(TILESIZE*(gs.chars[id].health/hmax))+1, 2});
			}
		}

		if (jammagame::input::is_pressed(jammagame::input::DIPSW1))
		{
			// Draw health above it
			if (gs.chars[id].health!=0)
				write(gs.chars[id].x-gs.xoffset, (gs.chars[id].y-gs.yoffset)-8, std::to_string(gs.chars[id].health), 1, 0,0,0, 1);

			// Draw pollen above it
			if (gs.chars[id].pollen!=0)
				write(gs.chars[id].x-gs.xoffset+(TILESIZE*0.75), (gs.chars[id].y-gs.yoffset)-8, std::to_string(gs.chars[id].pollen), 1, 255,0,255, 1);

			// Draw dwell below it
			if (gs.chars[id].dwell!=0)
				write(gs.chars[id].x-gs.xoffset+(TILESIZE*0.75), (gs.chars[id].y-gs.yoffset)+TILESIZE, std::to_string(gs.chars[id].dwell), 1, 0,255,0, 1);
		}
	}
}

// Draw shots
void
drawshots()
{
	for (uint32_t i=0; i<gs.shots.size(); i++)
	{
		if (gs.shots[i].ttl>=35)
			gs.shots[i].id=43; // muzzle flash sprite
		else
			gs.shots[i].id=44; // projectile sprite

		drawsprite(gs.shots[i].id, gs.shots[i].x, gs.shots[i].y, gs.shots[i].flip); // normal
	}
}

// Draw single particle
void
drawparticle(struct particle & particle)
{
	float x=particle.x+(particle.t*cos(particle.ang));
	float y=particle.y+(particle.t*sin(particle.ang));

	// Clip to what's visible
	if (((Math_floor(x)-gs.xoffset)<0) && // clip left
		((Math_floor(x)-gs.xoffset)>XMAX) && // clip right
		((Math_floor(y)-gs.yoffset)<0) && // clip top
		((Math_floor(y)-gs.yoffset)>YMAX))   // clip bottom
		return;

	gs.surface->set_colour(jammagame::gfx::colour::colour(particle.r, particle.g, particle.b, (particle.a*255)));
	gs.surface->solid_rectangle({Math_floor(x)-gs.xoffset, Math_floor(y)-gs.yoffset, particle.s, particle.s});
}

// Draw particles
void
drawparticles()
{
	for (uint32_t i=0; i<gs.particles.size(); i++)
		drawparticle(gs.particles[i]);
}

// Draw parallax
void
drawparallax()
{
	for (uint32_t i=0; i<gs.parallax.size(); i++)
	{
		switch (gs.parallax[i].t)
		{
			case 0:
			case 1:
				drawsprite(11+gs.parallax[i].t, gs.parallax[i].x-Math_floor(gs.xoffset/gs.parallax[i].z), gs.parallax[i].y-Math_floor(gs.yoffset/gs.parallax[i].z), false);
				break;

			case 2:
				drawsprite(1, gs.parallax[i].x-Math_floor(gs.xoffset/gs.parallax[i].z), gs.parallax[i].y-Math_floor(gs.yoffset/gs.parallax[i].z), false);
				drawsprite(2, gs.parallax[i].x-Math_floor(gs.xoffset/gs.parallax[i].z)+TILESIZE, gs.parallax[i].y-Math_floor(gs.yoffset/gs.parallax[i].z), false);
				break;

			default:
				break;
		}
	}
}

// Show messsage box
void
showmessagebox(const std::string & text, const uint32_t timing)
{
	if ((gs.msgboxtime==0) && (gs.state==STATEPLAYING))
	{
		// Set text to display
		gs.msgboxtext=text;

		// Set time to display messagebox
		gs.msgboxtime=timing;
	}
	else
	{
		struct msgboxitem m;

		m.msgboxtext=text;
		m.msgboxtime=timing;

		gs.msgqueue.push_back(m);
	}
}

std::vector<std::string>
strsplit(const std::string & str, const std::string & delimiter)
{
	std::vector<std::string> ostr;
	size_t pos=0;
	std::string token;
	std::string s=str;

	while ((pos=s.find(delimiter))!=std::string::npos)
	{
		token=s.substr(0, pos);
		ostr.push_back(token);
		s=s.substr(pos + delimiter.length());
	}

	if (s.length()>0)
		ostr.push_back(s);

	return ostr;
}

void
drawmsgbox()
{
	if (gs.msgboxtime>0)
	{
		uint8_t i;
		uint16_t width=0;
		uint16_t height=0;
		float top=0;
		int8_t icon=-1;
		uint16_t boxborder=1;

		// Draw box //
		// Split on \n
		std::vector<std::string> txtlines = strsplit(gs.msgboxtext, "\n");

		// Determine width (length of longest string + border)
		for (i=0; i<txtlines.size(); i++)
		{
			// Check for and remove icon from first line
			if ((i==0) && (txtlines[i][0]=='['))
			{
				size_t endbracket=txtlines[i].find(']');
				if (endbracket!=std::string::npos)
				{
					icon=std::stoi(txtlines[i].substr(1, endbracket), nullptr, 10);
					txtlines[i]=txtlines[i].substr(endbracket+1, std::string::npos);
				}
			}

			if (txtlines[i].length()>width)
				width=txtlines[i].length();
		}

		width+=(boxborder*2);

		// Determine height (number of lines + border)
		height=txtlines.size()+(boxborder*2);

		// Convert width/height into pixels
		width*=font_width;
		height*=(font_height+1);

		// Add space if sprite is to be drawn
		if (icon!=-1)
		{
			// Check for centering text when only one line and icon pads height
			if (txtlines.size()==1)
				top=0.5;
    
			width+=(TILESIZE+(font_width*2));

			if (height<(TILESIZE+(2*font_height)))
				height=TILESIZE+(2*font_height);
		}

		// Roll-up
		if (gs.msgboxtime<8)
			height=Math_floor(height*(gs.msgboxtime/8));

		// Draw box
		gs.surface->set_colour(jammagame::gfx::colour::colour(255, 255, 255, (0.75*255)));
		gs.surface->solid_rectangle({XMAX-(width+(boxborder*font_width)), 1*font_height, width, height});

		if (gs.msgboxtime>=8)
		{
			// Draw optional sprite
			if (icon!=-1)
				drawsprite(icon, (XMAX-width)+gs.xoffset, ((boxborder*2)*font_height)+gs.yoffset, false);

			// Draw text //
			for (i=0; i<txtlines.size(); i++)
				write(XMAX-width+(icon==-1?0:TILESIZE+font_width), (i+(boxborder*2)+top)*(font_height+1), txtlines[i], 1, 0, 0, 0, 0.75);
		}

		gs.msgboxtime--;
	}
	else
	{
		// Check if there are any message boxes queued up
		if ((gs.state==STATEPLAYING) && (gs.msgqueue.size()>0))
		{
			showmessagebox(gs.msgqueue[0].msgboxtext, gs.msgqueue[0].msgboxtime);
			gs.msgqueue.erase(gs.msgqueue.begin());
		}
	}
}

// Generate some particles around an origin
void
generateparticles(const float cx, const float cy, const float mt, const uint8_t count, const uint8_t r, const uint8_t g, const uint8_t b)
{
	for (uint8_t i=0; i<count; i++)
	{
		struct particle p;

		p.ang=Math_floor(rng()*360); // angle to eminate from
		p.t=Math_floor(rng()*mt); // travel from centre
		p.r=(r==0?rng()*255:r);
		p.g=(g==0?rng()*255:g);
		p.b=(b==0?rng()*255:b);
		p.a=1;

		p.x=cx;
		p.y=cy;

		p.s=(rng()<0.25)?2:1;

		gs.particles.push_back(p);
	}
}

// Check if area a overlaps with area b
bool
overlap(const float ax, const float ay, const float aw, const float ah, const float bx, const float by, const float bw, const float bh)
{
	// Check horizontally
	if ((ax<bx) && ((ax+aw))<=bx) return false; // a too far left of b
	if ((ax>bx) && ((bx+bw))<=ax) return false; // a too far right of b

	// Check vertically
	if ((ay<by) && ((ay+ah))<=by) return false; // a too far above b
	if ((ay>by) && ((by+bh))<=ay) return false; // a too far below b

	return true;
}

// Do processing for gun
void
guncheck()
{
	uint32_t i;

	// Cool gun down
	if (gs.gunheat>0) gs.gunheat--;

	// Check for having gun and want to use it
	if ((gs.gun==true) && (gs.gunheat==0) && (jammagame::input::is_pressed(jammagame::input::PLAYER1_BUTTON1)))
	{
		int16_t velocity=(gs.flip?-5:5);
		struct shot oneshot;

		oneshot.x=gs.x+velocity;
		oneshot.y=gs.y+3;
		oneshot.dir=velocity;
		oneshot.flip=gs.flip;
		oneshot.ttl=40;
		oneshot.id=44; // Gun shot sprite
		oneshot.del=false;

		gs.shots.push_back(oneshot);

		gs.gunheat=10; // Set time until next shot
	}

	// Move shots onwards / check for collisions
	for (i=0; i<gs.shots.size(); i++)
	{
		// Move shot onwards
		gs.shots[i].x+=gs.shots[i].dir;

		// Check shot collisions
		for (uint32_t id=0; id<gs.chars.size(); id++)
		{
			// Check for collision with this char
			if ((gs.shots[i].dir!=0) && (overlap(gs.shots[i].x, gs.shots[i].y, TILESIZE, TILESIZE, gs.chars[id].x, gs.chars[id].y, TILESIZE, TILESIZE)))
			{
				switch (gs.chars[id].id)
				{
					case 30: // toadstool
					case 31:
						gs.chars[id].htime=(2*FPS);
						gs.chars[id].health--;
						if (gs.chars[id].health<=0)
						{
							if (gs.chars[id].id==30) // If it's tall, change to small toadstool
							{
								gs.chars[id].health=HEALTHPLANT;
								gs.chars[id].growtime=(GROWTIME+Math_floor(rng()*120));
								gs.chars[id].id=31;
							}
							else
								gs.chars[id].del=true;
						}

						generateparticles(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 8, (gs.chars[id].health<=0)?16:2, 252, 104, 59);

						gs.shots[i].dir=0;
						gs.shots[i].ttl=3;
						break;

					case 53: // zombee
					case 54:
						gs.chars[id].htime=(2*FPS);
						gs.chars[id].health--;
						if (gs.chars[id].health<=0)
							gs.chars[id].del=true;

						generateparticles(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 16, (gs.chars[id].health<=0)?32:4, 44, 197, 246);

						gs.shots[i].dir=0;
						gs.shots[i].ttl=3;
						break;

					case 55: // grub
					case 56:
						gs.chars[id].htime=(2*FPS);
						gs.chars[id].health--;
						if (gs.chars[id].health<=0)
							gs.chars[id].del=true;

						generateparticles(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 16, (gs.chars[id].health<=0)?32:4, 252, 104, 59);

						gs.shots[i].dir=0;
						gs.shots[i].ttl=3;
						break;

					default:
						break;
				}
			}
		}

		// Decrease time-to-live, mark for deletion when expired
		gs.shots[i].ttl--;
		if (gs.shots[i].ttl<=0) gs.shots[i].del=true;
	}

	// Remove shots marked for deletion
	i=gs.shots.size();
	while (i--)
	{
		if (gs.shots[i].del)
			gs.shots.erase(gs.shots.begin()+i);
	}
}

// Check if player has left the map
void
offmapcheck()
{
	if ((gs.x<(0-TILESIZE)) || ((gs.x+1)>gs.width*TILESIZE) || (gs.y>gs.height*TILESIZE))
	{
		gs.x=gs.sx;
		gs.y=gs.sy;

		scrolltoplayer(false);
	}
}

bool
collide(const float px, const float py, const float pw, const float ph)
{
	// Check for screen edge collision
	if (px<=(0-(TILESIZE/5))) return true;
	if ((px+(TILESIZE/3))>=(gs.width*TILESIZE)) return true;

	// Look through all the tiles for a collision
	for (uint8_t y=0; y<gs.height; y++)
	{
		for (uint8_t x=0; x<gs.width; x++)
		{
			uint8_t tile=levels[gs.level].tiles[(y*gs.width)+x];
			if (tile>1)
			{
				if (overlap(px, py, pw, ph, x*TILESIZE, y*TILESIZE, TILESIZE, TILESIZE))
					return true;
			}
		}
	}

	return false;
}

// Collision check with player hitbox
bool
playercollide(const float x, const float y)
{
	return collide(x+(TILESIZE/3), y+((TILESIZE/5)*2), TILESIZE/3, (TILESIZE/5)*3);
}

// Check if player on the ground or falling
void
groundcheck()
{
	// Check for coyote time
	if (gs.coyote>0)
		gs.coyote--;

	// Check we are on the ground
	if (playercollide(gs.x, gs.y+1))
	{
		// If we just hit the ground after falling, create a few particles under foot
		if (gs.fall==true)
			generateparticles(gs.x+(TILESIZE/2), gs.y+TILESIZE, 4, 4, 170, 170, 170);

		gs.vs=0;
		gs.jump=false;
		gs.fall=false;
		gs.coyote=15;

		// Check for jump pressed, when not ducking
		if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)) && (!gs.duck))
		{
			gs.jump=true;
			gs.vs=-gs.jumpspeed;
		}
	}
	else
	{
		// Check for jump pressed, when not ducking, and coyote time not expired
		if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)) && (!gs.duck) && (gs.jump==false) && (gs.coyote>0))
		{
			gs.jump=true;
			gs.vs=-gs.jumpspeed;
		}

		// We're in the air, increase falling speed until we're at terminal velocity
		if (gs.vs<gs.terminalvelocity)
			gs.vs+=gs.gravity;

		// Set falling flag when vertical speed is positive
		if (gs.vs>0)
			gs.fall=true;
	}
}

// Process jumping
void
jumpcheck()
{
	// When jumping ..
	if (gs.jump)
	{
		// Check if loosing altitude
		if (gs.vs>=0)
		{
			gs.jump=false;
			gs.fall=true;
		}
	}
}

// Move player by appropriate amount, up to a collision
void
collisioncheck()
{
  uint8_t loop;

  // Check for horizontal collisions
  if ((gs.hs!=0) && (playercollide(gs.x+gs.hs, gs.y)))
  {
    loop=TILESIZE;
    // A collision occured, so move the character until it hits
    while ((!playercollide(gs.x+(gs.hs>0?1:-1), gs.y)) && (loop>0))
    {
      gs.x+=(gs.hs>0?1:-1);
      loop--;
    }

    // Stop horizontal movement
    gs.hs=0;
  }
  gs.x+=Math_floor(gs.hs);

  // Check for vertical collisions
  if ((gs.vs!=0) && (playercollide(gs.x, gs.y+gs.vs)))
  {
    loop=TILESIZE;
    // A collision occured, so move the character until it hits
    while ((!playercollide(gs.x, gs.y+(gs.vs>0?1:-1))) && (loop>0))
    {
      gs.y+=(gs.vs>0?1:-1);
      loop--;
    }

    // Stop vertical movement
    gs.vs=0;
  }
  gs.y+=Math_floor(gs.vs);
}

// If no input detected, slow the player using friction
void
standcheck()
{
  // Check for ducking, or injured
  if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN)) || (gs.htime>0))
    gs.duck=true;
  else
    gs.duck=false;

  // When no horizontal movement pressed, slow down by friction
  if (((!jammagame::input::is_pressed(jammagame::input::PLAYER1_LEFT)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_RIGHT))) ||
      ((jammagame::input::is_pressed(jammagame::input::PLAYER1_LEFT)) && (jammagame::input::is_pressed(jammagame::input::PLAYER1_RIGHT))))
  {
    // Going left
    if (gs.dir==-1)
    {
      if (gs.hs<0)
      {
        gs.hs+=gs.friction;
      }
      else
      {
        gs.hs=0;
        gs.dir=0;
      }
    }

    // Going right
    if (gs.dir==1)
    {
      if (gs.hs>0)
      {
        gs.hs-=gs.friction;
      }
      else
      {
        gs.hs=0;
        gs.dir=0;
      }
    }
  }

  // Extra checks for top-down levels
  if (gs.topdown)
  {
    // When no horizontal movement pressed, slow down by friction
    if (((!jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN))) ||
    ((jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)) && (jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN))))
    {
      // Going up
      if (gs.vs<0)
      {
        gs.vs+=gs.friction;
      }

      // Going down
      if (gs.vs>0)
      {
        gs.vs-=gs.friction;
      }
    }
  }
}

// Move animation frame onwards
void
updateanimation()
{
	if (gs.anim==0)
	{
		// Player animation
		if ((gs.hs!=0) || ((gs.topdown) && (gs.vs!=0)))
		{
			if (gs.gun)
			{
				gs.tileid++;

				if (gs.tileid>42)
					gs.tileid=40;
			}
			else
				gs.tileid==45?gs.tileid=46:gs.tileid=45;
		}
		else
		{
			if (gs.gun)
				gs.tileid=40;
			else
				gs.tileid=45;
		}

		// Char animation
		for (uint32_t id=0; id<gs.chars.size(); id++)
		{
			switch (gs.chars[id].id)
			{
				case 51:
					gs.chars[id].id=52;
					break;

				case 52:
					gs.chars[id].id=51;
					break;

				case 53:
					gs.chars[id].id=54;
					break;

				case 54:
					gs.chars[id].id=53;
					break;

				case 55:
					gs.chars[id].id=56;
					break;

				case 56:
					gs.chars[id].id=55;
					break;

				default:
					break;
			}
		}

		gs.anim=8;
	}
	else
		gs.anim--;
}

// Do processing for particles
void
particlecheck()
{
	uint16_t i=0;

	// Process particles
	for (i=0; i<gs.particles.size(); i++)
	{
		// Move particle
		gs.particles[i].t+=0.5;
		gs.particles[i].y+=(gs.gravity*2);

		// Decay particle
		gs.particles[i].a-=0.007;
	}

	// Remove particles which have decayed
	i=gs.particles.size();
	while (i--)
	{
		if (gs.particles[i].a<=0)
			gs.particles.erase(gs.particles.begin()+i);
	}
}

bool
anymovementkeypressed()
{
	return (
		jammagame::input::is_pressed(jammagame::input::PLAYER1_UP) ||
		jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN) ||
		jammagame::input::is_pressed(jammagame::input::PLAYER1_LEFT) ||
		jammagame::input::is_pressed(jammagame::input::PLAYER1_RIGHT) 
	);
}

// Update player movements
void
updatemovements()
{
	// Check if player has left the map
	offmapcheck();

	// Only apply 2D physics when not in top-down mode
	if (!gs.topdown)
	{
		// Check if player on the ground or falling
		groundcheck();

		// Process jumping
		jumpcheck();
	}

	// Move player by appropriate amount, up to a collision
	collisioncheck();

	// If no input detected, slow the player using friction
	standcheck();

	// Check for gun usage
	guncheck();

	// Check for particle usage
	particlecheck();

	// When a movement key is pressed, adjust players speed and direction
	if (anymovementkeypressed())
	{
		// Left key
		if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_LEFT)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_RIGHT)))
		{
			gs.hs=gs.htime==0?-gs.speed:-1;
			gs.dir=-1;
			gs.flip=true;
		}

		// Right key
		if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_RIGHT)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_LEFT)))
		{
			gs.hs=gs.htime==0?gs.speed:1;
			gs.dir=1;
			gs.flip=false;
		}

		// Extra processing for top-down levels
		if (gs.topdown)
		{
			// Up key
			if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN)))
			{
				gs.vs=gs.htime==0?-gs.speed:-1;
			}

			// Down key
			if ((jammagame::input::is_pressed(jammagame::input::PLAYER1_DOWN)) && (!jammagame::input::is_pressed(jammagame::input::PLAYER1_UP)))
			{
				gs.vs=gs.htime==0?gs.speed:1;
			}
		}
	}

	// Decrease hurt timer
	if (gs.htime>0) gs.htime--;

	// Decrease invulnerability timer
	if (gs.invtime>0) gs.invtime--;

	// Update any animation frames
	updateanimation();
}

// Check for collision between player and character/collectable
void
updateplayerchar()
{
	// Generate player hitbox
	float px=gs.x+(TILESIZE/3);
	float py=gs.y+((TILESIZE/5)*2);
	float pw=(TILESIZE/3);
	float ph=(TILESIZE/5)*3;

	for (uint32_t id=0; id<gs.chars.size(); id++)
	{
		// Check for collision with this char
		if (overlap(px, py, pw, ph, gs.chars[id].x, gs.chars[id].y, TILESIZE, TILESIZE))
		{
			switch (gs.chars[id].id)
			{
				case 0: // flip between 2D and topdown
					gs.topdown=(
						((levels[gs.level].tiles[(Math_floor((gs.chars[id].y-TILESIZE)/TILESIZE)*gs.width)+Math_floor(gs.chars[id].x/TILESIZE)])<=1) && // Tile above this toggle needs to be empty
						(gs.vs<0)); // pass over moving up for topdown, otherwise 2D
					break;

				case 10: // JS13K - invulnerability
					gs.htime=0; // cure player
					gs.invtime+=(10*FPS);
					gs.chars[id].del=true;
					break;

				case 53: // Zombee
				case 54:
					if ((gs.invtime==0) && (gs.htime==0))
						gs.htime=(5*FPS); // Set hurt timer

					if (gs.gun)
					{
						// Drop gun
						struct gamechar obj;

						obj.id=50;
						obj.x=gs.x;
						obj.y=gs.y;
						obj.flip=false;
						obj.hs=0;
						obj.vs=0;
						obj.dwell=0;
						obj.htime=0;
						obj.del=false;
						obj.health=0;

						obj.growtime=0;
						obj.pollen=0;
						obj.dx=-1;
						obj.dy=-1;

						gs.chars.push_back(obj);

						gs.gun=false;
					}
					break;

				case 55: // Grub
				case 56:
					if ((gs.invtime==0) && (gs.htime==0))
						gs.htime=(2*FPS); // Set hurt timer
					break;

				case 50: // gun
					if ((gs.invtime>0) || (gs.htime==0))
					{
						gs.gun=true;
						gs.tileid=40;
						gs.chars[id].del=true;
					}
					break;

				default:
					break;
			}
		}
	}
}

// Determine distance (Hypotenuse) between two lengths in 2D space (using Pythagoras)
float
calcHypotenuse(const float a, const float b)
{
	return (sqrt((a * a) + (b * b)));
}

// Find the nearst char of type included in tileids to given x, y point or -1
int16_t
findnearestchar(const float x, const float y, const std::vector<uint16_t> & tileids)
{
  float closest=(gs.width*gs.height*TILESIZE);
  int16_t charid=-1;
  float dist;

  for (uint32_t id=0; id<gs.chars.size(); id++)
  {
    if (std::count(tileids.begin(), tileids.end(), gs.chars[id].id)>0)
    {
      dist=calcHypotenuse(abs(x-gs.chars[id].x), abs(y-gs.chars[id].y));

      if (dist<closest)
      {
        charid=id;
        closest=dist;
      }
    }
  }

  return charid;
}

uint32_t
countchars(const std::vector<uint16_t> & tileids)
{
	uint32_t found=0;

	for (uint32_t id=0; id<gs.chars.size(); id++)
		if (std::count(tileids.begin(), tileids.end(), gs.chars[id].id)>0)
			found++;

	return found;
}

bool
islevelcompleted()
{
  // This is defined as ..
  //   no grubs
  //   no flies
  //   5 or more bees for level 1, then 6, 7, 8, e.t.c.

  return (
	(countchars({55,56})==0) &&
	(countchars({53,54})==0) &&
	(countchars({51,52})>=(uint8_t)(gs.level+5))
	);
}

void
updatecharAI()
{
	uint32_t id;
	float nx; // new x position
	float ny; // new y position ( UNUSED ?? )
 
	for (id=0; id<gs.chars.size(); id++)
	{
		bool eaten=false;

		// Decrease hurt timer
		if ((gs.chars[id].htime)>0) gs.chars[id].htime--;

		switch (gs.chars[id].id)
		{
			case 31: // toadstool
			case 33: // flower
				gs.chars[id].growtime--;
				if (gs.chars[id].growtime<=0)
				{
					gs.chars[id].health=HEALTHPLANT;
					gs.chars[id].id--; // Switch tile to bigger version of plant
				}
				break;

			case 51: // bee
			case 52:
			{
				// Check if dwelling
				if (gs.chars[id].dwell>0)
				{
					gs.chars[id].dwell--;

					continue;
				}

				// Check if following a path, then move to next node
				if (gs.chars[id].path.size()>0)
				{
					int16_t nextx=Math_floor(gs.chars[id].path[0]%gs.width)*TILESIZE;
					int16_t nexty=Math_floor(gs.chars[id].path[0]/gs.width)*TILESIZE;
					int16_t deltax=abs(nextx-gs.chars[id].x);
					int16_t deltay=abs(nexty-gs.chars[id].y);

					// Check if we have arrived at the current path node
					if ((deltax<=(TILESIZE/2)) && (deltay<=(TILESIZE/2)))
					{
						// We are here, so move on to next path node
						gs.chars[id].path.erase(gs.chars[id].path.begin());

						// Check for being at end of path
						if (gs.chars[id].path.size()==0)
						{
							// If following player, wait a bit here
							if (gs.chars[id].dx==-1)
								gs.chars[id].dwell=(2*FPS);

							// Set a null destination
							gs.chars[id].dx=-1;
							gs.chars[id].dy=-1;
						}
					}
					else
					{
						// Move onwards, following path
						if (deltax!=0)
						{
							gs.chars[id].hs=(nextx<gs.chars[id].x)?-SPEEDBEE:SPEEDBEE;
							gs.chars[id].x+=gs.chars[id].hs;
							gs.chars[id].flip=(gs.chars[id].hs<0);

							if (gs.chars[id].x<0)
								gs.chars[id].x=0;
						}

						if (deltay!=0)
						{
							gs.chars[id].y+=(nexty<gs.chars[id].y)?-SPEEDBEE:SPEEDBEE;

							if (gs.chars[id].x<0)
								gs.chars[id].x=0;
						}
					}
				}
				else
				{
					// Not following a path

					// Check if overlapping hive or flowers not in use or already being used by this bee
					for (uint32_t id2=0; id2<gs.chars.size(); id2++)
					{
						if (((gs.chars[id2].id==32) || (gs.chars[id2].id==33) || (gs.chars[id2].id==36) || (gs.chars[id2].id==37)) &&
							(overlap(gs.chars[id].x, gs.chars[id].y, TILESIZE, TILESIZE, gs.chars[id2].x, gs.chars[id2].y, TILESIZE, TILESIZE)))
						{
							switch (gs.chars[id2].id)
							{
								case 32: // flower
								case 33:
									gs.chars[id].dwell=(2*FPS);

									gs.chars[id].pollen++; // Increase pollen that the bee is carrying

									gs.chars[id2].health--; // Decrease flower health
									if (gs.chars[id2].health<=0)
									{
										if (gs.chars[id2].id==32) // If it's big flower, change to small flower, then the bee can get a bit more pollen
										{
											gs.chars[id2].health=HEALTHPLANT;
											gs.chars[id2].growtime=(GROWTIME+Math_floor(rng()*120));
											gs.chars[id2].id=33;
										}
										else
											gs.chars[id2].del=true; // Remove plant
									}
									break;

								case 36: // hive
								case 37:
									// Only use this hive if this bee has pollen
									if (gs.chars[id].pollen>0)
									{
										gs.chars[id].dwell=(2*FPS);

										// Transfer pollen from bee to hive
										gs.chars[id2].pollen+=gs.chars[id].pollen;
										gs.chars[id].pollen=0;

										// If hive has enough pollen, spawn another bee
										if ((gs.chars[id2].pollen>10) && (countchars({51,52})<MAXBEES))
										{
											struct gamechar obj;

											obj.id=51;
											obj.x=gs.chars[id2].x;
											obj.y=gs.chars[id2].y;
											obj.flip=false;
											obj.hs=0;
											obj.vs=0;
											obj.dwell=(5*FPS);
											obj.htime=0;
											obj.pollen=0;
											obj.dx=-1;
											obj.dy=-1;
											obj.del=false;
											obj.health=0;
											obj.growtime=0;

											gs.chars[id2].pollen-=10;
											gs.chars.push_back(obj);

											generateparticles(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 16, 16, 0, 0, 0);
                      
											int16_t beesneeded=((gs.level+5)-(countchars({51,52})));

											if (beesneeded<=0)
											{
												if (!islevelcompleted())
													showmessagebox("[53]Remove all threats", 3*FPS);
											}
											else
												showmessagebox("[51]"+std::to_string(beesneeded)+" more bees needed", 3*FPS);
										}
									}

									break;

								default: // Something we are not interested in
									break;
							}
						}
					}

					// Only look for some place to go if not dwelling due to collision
					if (gs.chars[id].dwell==0)
					{
						int16_t nid=-1; // next target id
						int16_t hid=-1; // next hive id
						int16_t fid=-1; // next flower id

						// Find nearest hive
						hid=findnearestchar(gs.chars[id].x, gs.chars[id].y, {36, 37});
    
						// Find nearest flower
						fid=findnearestchar(gs.chars[id].x, gs.chars[id].y, {32, 33});
    
						// If we have any pollen, go to nearest hive (if there is one)
						if ((hid!=-1) && (gs.chars[id].pollen>0))
							nid=hid;
    
						// However, if we need more pollen and there is a flower available, go there first
						if ((fid!=-1) && (gs.chars[id].pollen<5))
							nid=fid;
  
						// If something was found, check if we are already going there
						if (nid!=-1)
						{
							// If our next point of interest is not where we are already headed, then re-route
							if ((gs.chars[id].dx!=gs.chars[nid].x) && (gs.chars[id].dy!=gs.chars[nid].y))
							{
								gs.chars[id].path=pathfinder(
								(Math_floor(gs.chars[id].y/TILESIZE)*gs.width)+Math_floor(gs.chars[id].x/TILESIZE)
								,
								(Math_floor(gs.chars[nid].y/TILESIZE)*gs.width)+Math_floor(gs.chars[nid].x/TILESIZE)
								);

								gs.chars[id].dx=gs.chars[nid].x;
								gs.chars[id].dy=gs.chars[nid].y;
							}
						}
						else
						{
							// No new targets found
							if (gs.chars[id].path.size()==0)
							{
								// Go to player
								gs.chars[id].path=pathfinder(
								(Math_floor(gs.chars[id].y/TILESIZE)*gs.width)+Math_floor(gs.chars[id].x/TILESIZE)
								,
								(Math_floor(gs.y/TILESIZE)*gs.width)+Math_floor(gs.x/TILESIZE)
								);
    
								// Check if we didn't find the player on the map
								if (gs.chars[id].path.size()<=1)
								{
									// If not, dwell a bit to stop pathfinder running constantly
									gs.chars[id].dwell=(2*FPS);
								}
							}
						}
					}
				}
			} // bee scope
				break;

			case 53: // zombee
			case 54:
			{
				int16_t nid=-1; // next target id

				// If we are allowed to collide
				if (gs.chars[id].dwell==0)
				{
					// Check for collision
					for (uint32_t id2=0; id2<gs.chars.size(); id2++)
					{
						if (((gs.chars[id2].id==51) || (gs.chars[id2].id==52) || (gs.chars[id2].id==36) || (gs.chars[id2].id==37)) &&
						(overlap(gs.chars[id].x, gs.chars[id].y, TILESIZE, TILESIZE, gs.chars[id2].x, gs.chars[id2].y, TILESIZE, TILESIZE)) &&
						(gs.chars[id].dwell==0))
						{
							switch (gs.chars[id2].id)
							{
								case 51: // bee
								case 52:
									// Steal some pollen if it has any
									if (gs.chars[id2].pollen>0)
									{
										gs.chars[id2].pollen--;
										gs.chars[id].pollen++;

										// Don't allow further collisions for a while
										gs.chars[id].dwell=(5*FPS);
									}
									break;

								case 36: // hive
									// Break hive
									gs.chars[id2].id++;

									// See if there is any pollen in the hive
									if (gs.chars[id2].pollen>0)
									{
										// Loose half the pollen in the hive
										gs.chars[id2].pollen=Math_floor(gs.chars[id2].pollen/2);
									}

									// Don't allow further collisions for a while
									gs.chars[id].dwell=(10*FPS);
									break;

								default:
									break;
							}
						}
					}
				}
				else
				{
					gs.chars[id].dwell--; // Reduce collision preventer

					continue; // Stop further processing, we are still dwelling
				}

				// Find nearest hive/bee
				nid=findnearestchar(gs.chars[id].x, gs.chars[id].y, {36, 51, 52});

				// If something was found, check if we are already going there
				if (nid!=-1)
				{
					// If our next point of interest is not where we are already headed, then re-route
					if ((gs.chars[id].dx!=gs.chars[nid].x) && (gs.chars[id].dy!=gs.chars[nid].y))
					{
						gs.chars[id].path=pathfinder(
						(Math_floor(gs.chars[id].y/TILESIZE)*gs.width)+Math_floor(gs.chars[id].x/TILESIZE)
						,
						(Math_floor(gs.chars[nid].y/TILESIZE)*gs.width)+Math_floor(gs.chars[nid].x/TILESIZE)
						);

						gs.chars[id].dx=gs.chars[nid].x;
						gs.chars[id].dy=gs.chars[nid].y;
					}
				}
				else
				{
					// Nowhere to go next, dwell a bit to stop pathfinder running constantly
					gs.chars[id].dwell=(2*FPS);
				}

				// Check if following a path, if so do move to next node
				if (gs.chars[id].path.size()>0)
				{
					int16_t nextx=Math_floor(gs.chars[id].path[0]%gs.width)*TILESIZE;
					int16_t nexty=Math_floor(gs.chars[id].path[0]/gs.width)*TILESIZE;
					int16_t deltax=abs(nextx-gs.chars[id].x);
					int16_t deltay=abs(nexty-gs.chars[id].y);

					// Check if we have arrived at the current path node
					if ((deltax<=(TILESIZE/2)) && (deltay<=(TILESIZE/2)))
					{
						// We are here, so move on to next path node
						gs.chars[id].path.erase(gs.chars[id].path.begin());

						// Check for being at end of path
						if (gs.chars[id].path.size()==0)
						{
							// Path completed so wait a bit
							gs.chars[id].dwell=(2*FPS);

							// Set a null destination
							gs.chars[id].dx=-1;
							gs.chars[id].dy=-1;
						}
					}
					else
					{
						// Move onwards, following path
						if (deltax!=0)
						{
							if (nextx!=gs.chars[id].x)
							{
								gs.chars[id].hs=(nextx<gs.chars[id].x)?-SPEEDZOMBEE:SPEEDZOMBEE;
								gs.chars[id].x+=gs.chars[id].hs;
								gs.chars[id].flip=(gs.chars[id].hs<0);
							}

							if (gs.chars[id].x<0)
								gs.chars[id].x=0;
						}

						if (deltay!=0)
						{
							gs.chars[id].y+=(nexty<gs.chars[id].y)?-SPEEDZOMBEE:SPEEDZOMBEE;

							if (gs.chars[id].x<0)
								gs.chars[id].x=0;
						}
					}
				}
			} // zombee scope
				break;

			case 55: // grub
			case 56:
				// If dwelling, don't process any further
				if (gs.chars[id].dwell>0)
				{
					gs.chars[id].dwell--;
					gs.chars[id].hs=0; // Prevent movement

					continue;
				}

				// Check if overlapping a toadstool, if so stop and eat some
				for (uint32_t id2=0; id2<gs.chars.size(); id2++)
				{
					if ((eaten==false) && ((gs.chars[id2].id==30) || (gs.chars[id2].id==31)) &&
					(overlap(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 1, 1, gs.chars[id2].x, gs.chars[id2].y, TILESIZE, TILESIZE)))
					{
						gs.chars[id].health++; // Increase grub health
						eaten=true;
						gs.chars[id].dwell=(3*FPS);

						gs.chars[id2].health--; // Decrease toadstool health
						if (gs.chars[id2].health<=0)
						{
							if (gs.chars[id2].id==30) // If it's a tall toadstool, change to small toadstool, then eat a bit more
							{
								gs.chars[id2].health=HEALTHPLANT;
								gs.chars[id2].growtime=(GROWTIME+Math_floor(rng()*120));
								gs.chars[id2].id=31;
							}
							else
								gs.chars[id2].del=true;
						}

						break;
					}
				}

				if (((gs.chars[id].dwell==0)) && (eaten==false))
				{
					// Not eating, nor moving
					if (gs.chars[id].hs==0)
					{
						gs.chars[id].hs=(rng()<0.5)?-SPEEDGRUB:SPEEDGRUB; // Nothing eaten so move onwards
						gs.chars[id].flip=(gs.chars[id].hs<0);

						// If this grub is well fed, turn it into a zombee
						if ((gs.chars[id].health>(HEALTHGRUB*1.5)) && (countchars({53,54})<MAXFLIES))
						{
							gs.chars[id].id=53;
							gs.chars[id].health=HEALTHZOMBEE;
							gs.chars[id].pollen=0;
							gs.chars[id].dwell=(5*FPS);

							generateparticles(gs.chars[id].x+(TILESIZE/2), gs.chars[id].y+(TILESIZE/2), 16, 16, 0, 0, 0);

							return;
						}
					}

					nx=(gs.chars[id].x+=gs.chars[id].hs); // calculate new x position
					if ((collide(nx, gs.chars[id].y, TILESIZE, TILESIZE)) || // blocked by something
					(
					(!collide(nx+(gs.chars[id].flip?(TILESIZE/2)*-1:(TILESIZE)/2), gs.chars[id].y, TILESIZE, TILESIZE)) && // not blocked forwards
					(!collide(nx+(gs.chars[id].flip?(TILESIZE/2)*-1:(TILESIZE)/2), gs.chars[id].y+(TILESIZE/2), TILESIZE, TILESIZE)) // not blocked forwards+down (i.e. edge)
					))
					{
						// Turn around
						gs.chars[id].hs*=-1;
						gs.chars[id].flip=!gs.chars[id].flip;
					}
					else
						gs.chars[id].x=nx;
				}
				break;

			default:
				break;
		}
	}

	// Remove anything marked for deletion
	id=gs.chars.size();
	while (id--)
	{
		if (gs.chars[id].del)
			gs.chars.erase(gs.chars.begin()+id);
	}
}

void
checkspawn()
{
	gs.spawntime--;
	if (gs.spawntime<=0)
	{
		std::vector<struct spawnpoint> sps;

		// Create list of all possible spawn points
		for (int y=1; y<gs.height; y++)
		{
			for (int x=0; x<gs.width; x++)
			{
				uint8_t tile=levels[gs.level].tiles[(y*gs.width)+x];
				uint8_t tileabove=levels[gs.level].tiles[((y-1)*gs.width)+x];

				// Must be above flat edge
				switch (tile-1)
				{
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
					case 19:
					case 20:
					case 21:
					case 22:
					case 27:
					case 28:
					// Must have no tile above it
					if (tileabove<=1)
					{
						bool skip=false;

						// Must be certain distance away from all other chars
						for (uint32_t id=0; id<gs.chars.size(); id++)
						{
							if (calcHypotenuse(abs((x*TILESIZE)-gs.chars[id].x), abs((y*TILESIZE)-gs.chars[id].y))<(((rng()<0.5)?3:4)*TILESIZE))
								skip=true;
						}

						// Add to list of potential spawn points
						if (!skip)
						{
							struct spawnpoint sp;

							sp.x=x;
							sp.y=y-1;

							sps.push_back(sp);
						}
					}
					break;

					default:
						break;
				}
			}
		}

		if (sps.size()>0)
		{
			uint16_t spid=Math_floor(rng()*sps.size()); // Pick random spawn point from list
			uint8_t spawnid=(rng()<0.6)?33:31; // Pick randomly between flowers and toadstools
			struct gamechar obj;

			obj.id=spawnid;
			obj.x=(sps[spid].x*TILESIZE);
			obj.y=(sps[spid].y*TILESIZE);
			obj.flip=false;
			obj.hs=0;
			obj.vs=0;
			obj.dwell=(5*FPS);
			obj.htime=0;
			obj.pollen=0;
			obj.dx=-1;
			obj.dy=-1;
			obj.del=false;
			obj.health=HEALTHPLANT;
			obj.growtime=GROWTIME;

			// Add spawned item to front of chars
			gs.chars.insert(gs.chars.begin(), obj);
		}

		gs.spawntime=SPAWNTIME; // Set up for next spawn check
	}
}

void
levelinfo()
{
	// Write level number and title
	write((3*3)*13, 40, std::string("Level ")+std::to_string(gs.level+1), 3, 255,191,0, 1);
	write((XMAX/2)-((levels[gs.level].title.length()/2)*8), YMAX/2, levels[gs.level].title, 2, 255,255,255, 1);

	// Indicate what is required to progress to next level
	write(9*12, YMAX-20, std::string("Increase colony to ")+std::to_string(gs.level+5)+std::string(" bees"), 1, 255,191,0, 1);
}

void
startplaying()
{
	gs.state=STATEPLAYING;
	loadlevel();
}

void
newlevel(const uint8_t level)
{
  std::vector<std::string> hints;

	if (level>=levels.size())
		return;

	// Ensure timeline is stopped
	timeline_reset();

	gs.state=STATENEWLEVEL;

	// Set current level to new one
	gs.level=level;

	// Clear any messageboxes left on screen
	gs.msgqueue.clear();
	gs.msgboxtime=0;

	timeline_add(3*FPS, startplaying);

	// Add hints depending on level
	switch (level)
	{
		case 0:
			hints.push_back("[10]Welcome to JS13K entry\nby picosonic");
			hints.push_back("[50]Shoot enemies\nwith the honey gun");
			hints.push_back("[55]Grubs turn into Zombees\nwhen they eat toadstools");
			hints.push_back("[53]Zombees chase bees\nsteal pollen and honey\nand break hives");
			hints.push_back("[51]Bees collect pollen from flowers\nto make pollen in their hives");
			hints.push_back("[30]Clear away toadstools to prevent\ngrubs turning into ZomBees and\nmake space for flowers to grow");
			break;

		case 1:
			hints.push_back("[45]Watch out for gravity toggles");
			break;

		case 2:
			hints.push_back("[50]Solve the maze\nto find your prize");
			break;

		case 3:
			hints.push_back("[50]Use gravity toggle\nto get honey gun");
			break;

		case 4:
			hints.push_back("[45]Race to the top\nwith care");
			break;

		case 5:
			hints.push_back("[55]Hop to it before the\ngrubs change to Zombees");
			break;

		case 6:
			hints.push_back("[40]Take a leap of faith");
			break;

		default:
			break;
	}

	// Queue up all the hints as message boxes one after the other
	for (uint32_t n=0; n<hints.size(); n++)
		showmessagebox(hints[n], 3*FPS);
	
	timeline_begin(1);
}

// Set up intro animation callback
void
resettointro()
{
	timeline_reset();
	timeline_add(10*FPS, NULL);
	timeline_addcallback(intro);
	timeline_begin(1);
}

// End game animation
void
endgame(const float percent)
{
	if (gs.state!=STATECOMPLETE)
		return;

	// Check if done or control key/gamepad pressed
	if ((percent>=98) || (anymovementkeypressed()))
	{
		gs.state=STATEINTRO;

		timeline_add(0, resettointro);
	}
	else
	{
		if (percent==0)
		{
			// Add Bees
			gs.chars.clear();

			for (int n=0; n<50; n++)
			{
				struct gamechar obj;

				obj.id=51;
				obj.x=Math_floor(rng()*XMAX);
				obj.y=Math_floor(rng()*XMAX);
				obj.flip=false;
				obj.hs=rng()<0.5?-SPEEDBEE*2:SPEEDBEE*2;
				obj.vs=rng()<0.5?-SPEEDBEE*2:SPEEDBEE*2;
				obj.dwell=0;
				obj.htime=0;
				obj.del=false;
				obj.health=0;

				obj.growtime=0;
				obj.pollen=0;
				obj.dx=-1;
				obj.dy=-1;

				gs.chars.push_back(obj);
			}
		}

		write(35, 30, "CONGRATULATIONS", 4, 255,191,0, 1);
		write(15, (YMAX/2)+20, "The Queen Bee thanks you for helping", 2, 255,255,255, 1);
		write(50, (YMAX/2)+40, "to save the bees and planet", 2, 255,255,255, 1);

		// Draw rabbit
		drawsprite(((Math_floor(percent/2)%2)==1)?45:46, XMAX/2, Math_floor((YMAX/2)-(TILESIZE/2)), false);

		// Draw bees
		for (uint32_t i=0; i<gs.chars.size(); i++)
		{
			drawsprite(((Math_floor(percent/2)%2)==1)?51:52, gs.chars[i].x, gs.chars[i].y, false);

			// move bee onwards
			gs.chars[i].x+=gs.chars[i].hs;
			if ((gs.chars[i].x<0) || (gs.chars[i].x+TILESIZE>XMAX))
				gs.chars[i].hs*=-1;

			gs.chars[i].y+=gs.chars[i].vs;
			if ((gs.chars[i].y<0) || (gs.chars[i].y+TILESIZE>YMAX))
				gs.chars[i].vs*=-1;
		}
	}
}

// Update function called once per frame
void
update()
{
	if (gs.state==STATEPLAYING)
	{
		// Apply keystate/physics to player
		updatemovements();

		// Update other character movements / AI
		updatecharAI();

		// Check for player/character/collectable collisions
		updateplayerchar();

		// Check for spawn event
		checkspawn();

		// Check for level completed
		if ((gs.state==STATEPLAYING) && (islevelcompleted()))
		{
			gs.xoffset=0;
			gs.yoffset=0;

			if ((uint8_t)(gs.level+1)==levels.size())
			{
				// End of game
				gs.state=STATECOMPLETE;

				timeline_reset();
				timeline_add(10*FPS, NULL);
				timeline_addcallback(endgame);
				timeline_begin(0);
			}
			else
				newlevel(gs.level+1);
		}
	}
}

void
intro(const float percent)
{
	// Check if done or control key/gamepad pressed
	if ((percent>=98) || (anymovementkeypressed()))
	{
		newlevel(0);
	}
	else
	{
		float tenth=Math_floor(percent/10);
		std::string title=" BEE KIND ";
		char curchar=std::string(title)[tenth];

		for (int cc=0; cc<10; cc++)
		{
			if (cc<tenth)
				write(cc*(8*4), 30, std::string("")+std::string(title)[cc], 5, 255,191,0, 1);
		}

		if (curchar!=' ')
			generateparticles((tenth+0.4)*(8*4), 30, 4, 8, 255, 191, 0);

		// Introduce characters
		// grub
		drawsprite(((Math_floor(percent/2)%2)==1)?55:56, XMAX-Math_floor((percent/100)*XMAX)+50, Math_floor((YMAX/2)+(TILESIZE*2)), true);
		write(XMAX-Math_floor((percent/100)*XMAX)+50+TILESIZE, Math_floor((YMAX/2)+(TILESIZE*2.5)), "GRUB - eats toadstools, becomes ZOMBEE", 1, 240,240,240, 1);

		// zombee
		drawsprite(((Math_floor(percent/2)%2)==1)?53:54, XMAX-Math_floor((percent/100)*XMAX)+TILESIZE+50, Math_floor((YMAX/2)+TILESIZE), true);
		write(XMAX-Math_floor((percent/100)*XMAX)+(TILESIZE*2)+50, Math_floor((YMAX/2)+(TILESIZE*1.3)), "ZOMBEE - steals pollen, breaks hives", 1, 240,240,240, 1);

		// Draw rabbit
		drawsprite(((Math_floor(percent/2)%2)==1)?45:46, Math_floor((percent/100)*XMAX), Math_floor((YMAX/2)-(TILESIZE/2)), false);

		// Draw bees
		drawsprite(((Math_floor(percent/2)%2)==1)?51:52, XMAX-Math_floor((percent/100)*XMAX), Math_floor((YMAX/2)+(TILESIZE*2)), true);
		drawsprite(((Math_floor(percent/2)%2)==1)?52:51, XMAX-Math_floor((percent/100)*XMAX)+TILESIZE, Math_floor((YMAX/2)+TILESIZE), true);

		// Draw controls
		if ((Math_floor(percent)%16)<=8)
		{
			std::string keys=((Math_floor(percent/2)%32)<16)?"WASD":"ZQSD";
			write((XMAX/4)+(TILESIZE*2), YMAX-20, keys+"/CURSORS + ENTER/SPACE/SHIFT", 1, 240,240,240, 1);
			write((XMAX/4)+(TILESIZE*2), YMAX-10, "or use GAMEPAD", 1, 240,240,240, 1);

			// Draw JS13k gamepad
			drawsprite(10, (XMAX/4)+(TILESIZE/2), YMAX-TILESIZE, false);
		}

		// Animate the particles
		drawparticles();
		particlecheck();
	}
}

int	
jammagame_initialise()
{
#if defined(JAMMAGAME_PORT_SDL)
	jammagame::assets::install_assets(0,jammagame::assets::Assets(&sg_gbin_game[0]));
#endif

	sg_builtin_font	= jammagame::assets::assets(jammagame::assets::SLOT_BUILT_IN).get_tileset(0);

	reset_gamestate();

	resettointro();

	return 0;
}

void
jammagame_draw(jammagame::gfx::Surface & surface)
{
	// Cache surface for later use
	gs.surface=&surface;

	// Clear screen
	if (gs.state==STATEPLAYING)
		surface.set_colour(jammagame::gfx::colour::colour(BGCOLOUR));
	else
		surface.set_colour(jammagame::gfx::colour::colour(BLACKCOLOUR));

	gs.surface->clear();

	// Draw what needs drawing
	switch (gs.state)
	{
		case STATENEWLEVEL:
			levelinfo();
			break;

		case STATEPLAYING:
			// Scroll to keep player in view
			scrolltoplayer(true);

			// Draw the parallax
			drawparallax();

			// Draw the level
			drawlevel();

			// Draw the chars
			drawchars();

			// Draw the player
			if (gs.invtime>0)
				generateparticles(gs.x+(TILESIZE/2), gs.y+TILESIZE, 4, 2, 44, 197, 246); // leave a trail when invulnerable

			if ((gs.htime==0) || ((gs.htime%30)<=15)) // Flash when hurt
				drawsprite(gs.tileid, gs.x, gs.y, gs.flip);

			// Draw the shots
			drawshots();

			// Draw the particles
			drawparticles();

			// Draw any visible messagebox
			drawmsgbox();

			// Draw game stats
			if (jammagame::input::is_pressed(jammagame::input::DIPSW1))
			{
				uint8_t dtop=1;

				write(XMAX-(12*font_width), font_height*(dtop++), "GRB : "+std::to_string(countchars({55, 56})), 1, DEBUGTXTCOLOUR);
				write(XMAX-(12*font_width), font_height*(dtop++), "ZOM : "+std::to_string(countchars({53, 54})), 1, DEBUGTXTCOLOUR);
				write(XMAX-(12*font_width), font_height*(dtop++), "BEE : "+std::to_string(countchars({51, 52})), 1, DEBUGTXTCOLOUR);
			}
			break;

		default:
			break;
	}

	// Run a timeline step
	timeline_call();
}

void
jammagame_shutdown()
{
}

void
jammagame_update()
{
	update();
	update(); // simulate 60 fps (kinda)
}

