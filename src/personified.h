/*
  Copyright (C) 2004 Chris Cuthbertson

  This file is part of Personified.

  Personified is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Personified is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Personified.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#ifdef WIN32
#include <winsock.h>
#endif
#ifdef UNIX
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <time.h>
#include "util.h"

#define PER_HEADER "Personified"
#define PSV_HEADER "Personification"
#define PER_VERSION "1.25"
#define PSV_VERSION "1.22"
#define COPYRIGHT "2004"
#define AUTHOR "bliP"
#define WEB "http://nisda.net"

#define A2A_PER_ID '\xFA'
#define A2A_QUAKE_ID '\xFF'

#define C2S_PING 'z'
#define C2S_CONNECT 'x'
#define C2S_DISCONNECT 'c'
#define C2S_UPDATE_STAT 'v'
#define C2S_REQUEST_STAT 'b'
#define C2S_NO_UPDATE 'r'

#define S2C_PONG 'a'
#define S2C_BAD_CONNECT 's'
#define S2C_GOOD_CONNECT 'd'
#define S2C_STAT_PRINT 'j'
#define S2C_PRINT 'g'
#define S2C_SHUTDOWN 'h'

#define PSV_CFG_FILE "personification.cfg"
#define PER_CFG_FILE "personified.cfg"
#define PSV_SERVER_PORT 27105
#define PER_CLIENT_PORT 27104
#define QUAKE_CLIENT_PORT 27001
#define PROTOCOL_VERSION "1.3"

#define MAX_CLIENTS 8
#define MAX_NAME 8
#define MAX_PASSWORD 16
#define MAX_VERSION 6
#define MAX_BYTE_SIZE 4
#define MAX_FLOAT_SIZE 4
#define MAX_FILE_NAME 32
#define MAX_FILE_PATH 256
#define MAX_LINE 128
#define MAX_LOC_NAME 64
#define MAX_LOC_LINE 256
#define MAX_MACRO MAX_LOC_NAME

#define MAX_IP 15
#define MAX_ADDRESS 21
#define MAX_KEY 32
#define MAX_VALUE 256
#define MAX_REPORT_LINE 128
#define MAX_STAT_SIZE 23
#define MAX_SINGLE_STAT (MAX_STAT_SIZE + MAX_NAME + 2)
#define MAX_FULL_STAT ((MAX_SINGLE_STAT * MAX_CLIENTS) + 1)

//built in cvar names
#define CVAR_NAME "name"
#define CVAR_SERVER "server"
#define CVAR_PASSWORD "password"
#define CVAR_LOC_DIR "loc_dir"
#define CVAR_REPORT "report"
#define CVAR_AUTO_CONNECT "auto_connect"
#define CVAR_AUTO_CLOSE "auto_close"
#define CVAR_ALIGN "align"
#define CVAR_MESSAGES "messages"
#define CVAR_LOCAL_PORT "local_port"
#define CVAR_SELF "self"
#define CVAR_HOTKEY "hotkey"
#define CVAR_HIGHLIGHT "highlight"
#define CVAR_HOSTNAME "hostname"
#define CVAR_MAX_CLIENTS "max_clients"
#define CVAR_NAME_GREEN_ARMOUR "name_green_armour"
#define CVAR_NAME_YELLOW_ARMOUR "name_yellow_armour"
#define CVAR_NAME_RED_ARMOUR "name_red_armour"
#define CVAR_NAME_NO_ARMOUR "name_no_armour"
#define CVAR_NAME_ARMOUR "name_armour"
#define CVAR_NAME_HEALTH "name_health"
#define CVAR_NAME_WEAPON "name_weapon"
#define CVAR_NAME_WEAPON_AXE "name_weapon_axe"
#define CVAR_NAME_WEAPON_SHOTGUN "name_weapon_shotgun"
#define CVAR_NAME_WEAPON_SUPER_SHOTGUN "name_weapon_super_shotgun"
#define CVAR_NAME_WEAPON_NAILGUN "name_weapon_nailgun"
#define CVAR_NAME_WEAPON_SUPER_NAILGUN "name_weapon_super_nailgun"
#define CVAR_NAME_WEAPON_GRENADE_LAUNCHER "name_weapon_grenade_launcher"
#define CVAR_NAME_WEAPON_ROCKET_LAUNCHER "name_weapon_rocket_launcher"
#define CVAR_NAME_WEAPON_LIGHTNING_GUN "name_weapon_lightning_gun"
#define CVAR_NAME_POWERUP_KEY "name_powerup_key"
#define CVAR_NAME_POWERUP_QUAD_DAMAGE "name_powerup_quad_damage"
#define CVAR_NAME_POWERUP_PENTAGRAM "name_powerup_pentagram"
#define CVAR_NAME_POWERUP_INVISIBILITY "name_powerup_invisibility"
//#define CVAR_NAME_POWERUP_MEGA_HEALTH "name_powerup_mega_health"
//#define CVAR_NAME_POWERUP_SUIT "name_powerup_suit"
#define CVAR_NAME_ROCKETS "name_rockets"
#define CVAR_NAME_CELLS "name_cells"
#define CVAR_NAME_NAILS "name_nails"
#define CVAR_NAME_SHELLS "name_shells"
#define CVAR_NAME_SOMEPLACE "name_someplace"
#define CVAR_NAME_NOTHING "name_nothing"
#define CVAR_NEED_ARMOUR "need_armour"
#define CVAR_NEED_HEALTH "need_health"
#define CVAR_NEED_WEAPON "need_weapon"
#define CVAR_NEED_ROCKETS "need_rockets"
#define CVAR_NEED_CELLS "need_cells"
#define CVAR_NEED_NAILS "need_nails"
#define CVAR_NEED_SHELLS "need_shells"
#define CVAR_SEPARATOR "separator"

#define MACRO_ARMOUR_TYPE 0
#define MACRO_ARMOUR 1
#define MACRO_HEALTH 2
#define MACRO_GOLD_ARMOUR 3
#define MACRO_GOLD_HEALTH 4
#define MACRO_CURRENT_WEAPON 5
#define MACRO_BEST_WEAPON 6
#define MACRO_BEST_WEAPON_AMMO 7
#define MACRO_SHELLS 8
#define MACRO_NAILS 9
#define MACRO_ROCKETS 10
#define MACRO_CELLS 11
#define MACRO_POWERUPS 12
#define MACRO_GOLD_POWERUPS 13
#define MACRO_LED 14
#define MACRO_DISTANCE 15
#define MACRO_NEED 16
#define MACRO_LOCATION 17
#define MACRO_NAME 18

#define	IT_SHOTGUN 1
#define	IT_SUPER_SHOTGUN 2
#define	IT_NAILGUN 4
#define	IT_SUPER_NAILGUN 8
#define	IT_GRENADE_LAUNCHER 16
#define	IT_ROCKET_LAUNCHER 32
#define	IT_LIGHTNING 64
#define	IT_SUPER_LIGHTNING 128
#define	IT_SHELLS 256
#define	IT_NAILS 512
#define	IT_ROCKETS 1024
#define	IT_CELLS 2048
#define	IT_AXE 4096
#define	IT_ARMOR1 8192
#define	IT_ARMOR2 16384
#define	IT_ARMOR3 32768
#define	IT_SUPERHEALTH 65536
#define	IT_KEY1 131072
#define	IT_KEY2 262144
#define	IT_INVISIBILITY 524288
#define	IT_INVULNERABILITY 1048576
#define	IT_SUIT 2097152
#define	IT_QUAD 4194304

#define MAX_MEM 22
#define MEM_SIMORG_0 0
#define MEM_SIMORG_1 1
#define MEM_SIMORG_2 2
#define	MEM_STAT_HEALTH 0
#define	MEM_STAT_FRAGS 1
#define	MEM_STAT_WEAPON 2
#define	MEM_STAT_AMMO 3
#define	MEM_STAT_ARMOR 4
#define	MEM_STAT_WEAPONFRAME 5
#define	MEM_STAT_SHELLS 6
#define	MEM_STAT_NAILS 7
#define	MEM_STAT_ROCKETS 8
#define	MEM_STAT_CELLS 9
#define	MEM_STAT_ACTIVEWEAPON 10
#define	MEM_STAT_TOTALSECRETS 11
#define	MEM_STAT_TOTALMONSTERS 12
#define	MEM_STAT_SECRETS 13
#define	MEM_STAT_MONSTERS 14
#define	MEM_STAT_ITEMS 15
#define	MEM_STAT_VIEWHEIGHT 16
#define MEM_PRINT1_ON '%'
#define MEM_PRINT2_ON 'p'
#define MEM_PRINT1_OFF '\0'
#define MEM_PRINT2_OFF '\0'
#define MEM_STATE_DISCONNECTED 0
#define MEM_STATE_DEMOSTART 1
#define MEM_STATE_CONNECTED 2
#define MEM_STATE_ONSERVER 3
#define MEM_STATE_ACTIVE 4

#define QUAKE_WINDOW_TITLE "Quake"

//how do find hex codes (position=float, state,stats=4byte int):
//version: make it up
//engine: make it up
//print1: print
//print2: %s:
//stats: shoot yourself and search for number, shoot yourself again and watch for changes, choose lowest in list
//position: showloc middle number .0 is easier to find, move and watch numbers move. choose number closest to stats
//mapname: use *1/mapname.bsp (2nd search)
//state: connect,find qport (by server ip:port), search for 4, address by qport that changes to 0 when you disconnect

//FuhQuake 0.31 build 650 GL
#define FUHQUAKE_031_650_GL_MAPNAME 0x2BBA6BD
#define FUHQUAKE_031_650_GL_STATE 0x2C6D3A0
#define FUHQUAKE_031_650_GL_POSITION 0x2BBA620
#define FUHQUAKE_031_650_GL_STATS 0x2BBA4C8
#define FUHQUAKE_031_650_GL_ENGINE 0x4A542C
#define FUHQUAKE_031_650_GL_VERSION 0x9266BC
#define FUHQUAKE_031_650_GL_PRINT1 0x4AEA78
#define FUHQUAKE_031_650_GL_PRINT2 0x4A726C
#define FUHQUAKE_031_650_GL_ENGINETEXT "WIN32:GL"
#define FUHQUAKE_031_650_GL_VERSIONTEXT "0.31 (build 650)"
#define FUHQUAKE_031_650_GL_TITLE "FuhQuake 0.31 (GL)"
//#define FUHQUAKE_031_650_GL_WINDOW "FuhQuake"

//FuhQuake 0.31 build 650 Soft
#define FUHQUAKE_031_650_SOFT_MAPNAME 0x243F365
#define FUHQUAKE_031_650_SOFT_STATE 0x2655740
#define FUHQUAKE_031_650_SOFT_POSITION 0x25A29C0
#define FUHQUAKE_031_650_SOFT_STATS 0x25A2868
#define FUHQUAKE_031_650_SOFT_VERSION 0x50EA8C
#define FUHQUAKE_031_650_SOFT_ENGINE 0x4CA444
#define FUHQUAKE_031_650_SOFT_PRINT1 0x4D4794
#define FUHQUAKE_031_650_SOFT_PRINT2 0x4CC204
#define FUHQUAKE_031_650_SOFT_ENGINETEXT "WIN32:SOFT"
#define FUHQUAKE_031_650_SOFT_VERSIONTEXT "0.31 (build 650)"
#define FUHQUAKE_031_650_SOFT_TITLE "FuhQuake 0.31 (Soft)"
//#define FUHQUAKE_031_650_SOFT_WINDOW "FuhQuake"

//GreyMQuake 0.11s GL
#define GREYMQUAKE_011S_GL_MAPNAME 0xC8D5F5
#define GREYMQUAKE_011S_GL_STATE 0xDA96E0
#define GREYMQUAKE_011S_GL_POSITION 0xE8EE88
#define GREYMQUAKE_011S_GL_STATS 0xE8ECF0
#define GREYMQUAKE_011S_GL_VERSION 0x498C86
#define GREYMQUAKE_011S_GL_ENGINE 0x48E96F
#define GREYMQUAKE_011S_GL_PRINT1 0x485BA0
#define GREYMQUAKE_011S_GL_PRINT2 0x485A00
#define GREYMQUAKE_011S_GL_ENGINETEXT "GLQUAKE"
#define GREYMQUAKE_011S_GL_VERSIONTEXT "version 0.11b"
#define GREYMQUAKE_011S_GL_TITLE "GreyMQuake 0.11b (GL)"
//#define GREYMQUAKE_011S_GL_WINDOW "More QuakeWorld"

//MoreQuakeWorld 0.96 Soft
#define MOREQUAKEWORLD_096_SOFT_MAPNAME 0x4E27F1
#define MOREQUAKEWORLD_096_SOFT_STATE 0x56B9E0
#define MOREQUAKEWORLD_096_SOFT_POSITION 0x651188
#define MOREQUAKEWORLD_096_SOFT_STATS 0x650FF0
#define MOREQUAKEWORLD_096_SOFT_VERSION 0x491E96
#define MOREQUAKEWORLD_096_SOFT_ENGINE 0x4AD6BC
#define MOREQUAKEWORLD_096_SOFT_PRINT1 0x484A90
#define MOREQUAKEWORLD_096_SOFT_PRINT2 0x4848F0
#define MOREQUAKEWORLD_096_SOFT_ENGINETEXT "SOFT.EXE"
#define MOREQUAKEWORLD_096_SOFT_VERSIONTEXT "0.96b"
#define MOREQUAKEWORLD_096_SOFT_TITLE "More QuakeWorld 0.96b (Soft)"
//#define MOREQUAKEWORLD_096_SOFT_WINDOW "More QuakeWorld"

//MoreQuakeWorld 0.96 Soft2
#define MOREQUAKEWORLD_096_SOFT2_MAPNAME 0x6F9011
#define MOREQUAKEWORLD_096_SOFT2_STATE 0x782200
#define MOREQUAKEWORLD_096_SOFT2_POSITION 0x8679A8
#define MOREQUAKEWORLD_096_SOFT2_STATS 0x867810
#define MOREQUAKEWORLD_096_SOFT2_VERSION 0x4C1EEE
#define MOREQUAKEWORLD_096_SOFT2_ENGINE 0x4EA2C0
#define MOREQUAKEWORLD_096_SOFT2_PRINT1 0x4B4A90
#define MOREQUAKEWORLD_096_SOFT2_PRINT2 0x4B48F0 
#define MOREQUAKEWORLD_096_SOFT2_ENGINETEXT "SOFT2.EXE"
#define MOREQUAKEWORLD_096_SOFT2_VERSIONTEXT "0.96b"
#define MOREQUAKEWORLD_096_SOFT2_TITLE "More QuakeWorld 0.96b (Soft2)"
//#define MOREQUAKEWORLD_096_SOFT2_WINDOW "More QuakeWorld"

//MoreQuakeWorld 0.96 GL
#define MOREQUAKEWORLD_096_GL_MAPNAME 0xC8BD15
#define MOREQUAKEWORLD_096_GL_STATE 0xDA8240
#define MOREQUAKEWORLD_096_GL_POSITION 0xE8D9E8
#define MOREQUAKEWORLD_096_GL_STATS 0xE8D850
#define MOREQUAKEWORLD_096_GL_VERSION 0x49749E
#define MOREQUAKEWORLD_096_GL_ENGINE 0x852E44
#define MOREQUAKEWORLD_096_GL_PRINT1 0x484A30
#define MOREQUAKEWORLD_096_GL_PRINT2 0x484890
#define MOREQUAKEWORLD_096_GL_ENGINETEXT "GL.EXE"
#define MOREQUAKEWORLD_096_GL_VERSIONTEXT "0.96b"
#define MOREQUAKEWORLD_096_GL_TITLE "More QuakeWorld 0.96b (GL)"
//#define MOREQUAKEWORLD_096_GL_WINDOW "More QuakeWorld"

//MoreQuakeWorld 0.96 GL2
#define MOREQUAKEWORLD_096_GL2_MAPNAME 0xEA6895
#define MOREQUAKEWORLD_096_GL2_STATE 0xFC2DC0
#define MOREQUAKEWORLD_096_GL2_POSITION 0x10B0DF8
#define MOREQUAKEWORLD_096_GL2_STATS 0x10A83D0
#define MOREQUAKEWORLD_096_GL2_VERSION 0x4CAC26
#define MOREQUAKEWORLD_096_GL2_ENGINE 0x893D90
#define MOREQUAKEWORLD_096_GL2_PRINT1 0x4B4C0C
#define MOREQUAKEWORLD_096_GL2_PRINT2 0x4B4E10
#define MOREQUAKEWORLD_096_GL2_ENGINETEXT "GL2.EXE"
#define MOREQUAKEWORLD_096_GL2_VERSIONTEXT "0.96b"
#define MOREQUAKEWORLD_096_GL2_TITLE "More QuakeWorld 0.96b (GL2)"
//#define MOREQUAKEWORLD_096_GL2_WINDOW "More QuakeWorld"

//QuakeWorld 2.33 Soft
#define QUAKEWORLD_233_SOFT_MAPNAME 0x4CD171
#define QUAKEWORLD_233_SOFT_STATE 0x536D40
#define QUAKEWORLD_233_SOFT_POSITION 0x5AFCC0
#define QUAKEWORLD_233_SOFT_STATS 0x5AFB28
#define QUAKEWORLD_233_SOFT_VERSION 0x12E5E0
#define QUAKEWORLD_233_SOFT_ENGINE 0x4889C9
#define QUAKEWORLD_233_SOFT_PRINT1 0x47B0FC
#define QUAKEWORLD_233_SOFT_PRINT2 0x47AF14
#define QUAKEWORLD_233_SOFT_ENGINETEXT "SOFTWARE"
#define QUAKEWORLD_233_SOFT_VERSIONTEXT "QuakeWorld 2.33"
#define QUAKEWORLD_233_SOFT_TITLE "QuakeWorld 2.33 (Soft)"
//#define QUAKEWORLD_233_SOFT_WINDOW "Quake"

//QuakeWorld 2.33 GL
#define QUAKEWORLD_233_GL_MAPNAME 0xC480F1
#define QUAKEWORLD_233_GL_STATE 0xD0A740
#define QUAKEWORLD_233_GL_POSITION 0xD836C0
#define QUAKEWORLD_233_GL_STATS 0xD83528
#define QUAKEWORLD_233_GL_VERSION 0x456D65
#define QUAKEWORLD_233_GL_ENGINE 0x44EC80
#define QUAKEWORLD_233_GL_PRINT1 0x44C3EC
#define QUAKEWORLD_233_GL_PRINT2 0x44C5F8
#define QUAKEWORLD_233_GL_ENGINETEXT "GL"
#define QUAKEWORLD_233_GL_VERSIONTEXT "version 2.33"
#define QUAKEWORLD_233_GL_TITLE "QuakeWorld 2.33 (GL)"
//#define QUAKEWORLD_233_GL_WINDOW "Quake"

typedef struct quake_s {
  char *title;
  ///char *window;
  char *versiontext;
  char *enginetext;
  unsigned long version;
  unsigned long engine;
  unsigned long mapname;
  unsigned long state;
  unsigned long posx;
  unsigned long posy;
  unsigned long posz;
  unsigned long health;
  unsigned long frags;
  unsigned long weapon;
  unsigned long ammo;
  unsigned long armour;
  unsigned long weaponframe;
  unsigned long shells;
  unsigned long nails;
  unsigned long rockets;
  unsigned long cells;
  unsigned long activeweapon;
  unsigned long totalsecrets;
  unsigned long totalmonsters;
  unsigned long secrets;
  unsigned long monsters;
  unsigned long items;
  unsigned long viewheight;
  unsigned long print1;
  unsigned long print2;
} quake_t;

#define MAX_SUPPORTED_QUAKE 9
quake_t quake[MAX_SUPPORTED_QUAKE] =
{
  {
    FUHQUAKE_031_650_GL_TITLE,
    //FUHQUAKE_031_650_GL_WINDOW,
    FUHQUAKE_031_650_GL_VERSIONTEXT,
    FUHQUAKE_031_650_GL_ENGINETEXT,
    FUHQUAKE_031_650_GL_VERSION,
    FUHQUAKE_031_650_GL_ENGINE,
    FUHQUAKE_031_650_GL_MAPNAME,
    FUHQUAKE_031_650_GL_STATE,

    FUHQUAKE_031_650_GL_POSITION,
    FUHQUAKE_031_650_GL_POSITION + 4,
    FUHQUAKE_031_650_GL_POSITION + 8,

    FUHQUAKE_031_650_GL_STATS,
    FUHQUAKE_031_650_GL_STATS + 4,
    FUHQUAKE_031_650_GL_STATS + 8,
    FUHQUAKE_031_650_GL_STATS + 12,
    FUHQUAKE_031_650_GL_STATS + 16,
    FUHQUAKE_031_650_GL_STATS + 20,
    FUHQUAKE_031_650_GL_STATS + 24,
    FUHQUAKE_031_650_GL_STATS + 28,
    FUHQUAKE_031_650_GL_STATS + 32,
    FUHQUAKE_031_650_GL_STATS + 36,
    FUHQUAKE_031_650_GL_STATS + 40,
    FUHQUAKE_031_650_GL_STATS + 44,
    FUHQUAKE_031_650_GL_STATS + 48,
    FUHQUAKE_031_650_GL_STATS + 52,
    FUHQUAKE_031_650_GL_STATS + 56,
    FUHQUAKE_031_650_GL_STATS + 60,
    FUHQUAKE_031_650_GL_STATS + 64,

    FUHQUAKE_031_650_GL_PRINT1,
    FUHQUAKE_031_650_GL_PRINT2,
  },
  {
    FUHQUAKE_031_650_SOFT_TITLE,
    //FUHQUAKE_031_650_SOFT_WINDOW,
    FUHQUAKE_031_650_SOFT_VERSIONTEXT,
    FUHQUAKE_031_650_SOFT_ENGINETEXT,    
    FUHQUAKE_031_650_SOFT_VERSION,
    FUHQUAKE_031_650_SOFT_ENGINE,
    FUHQUAKE_031_650_SOFT_MAPNAME,
    FUHQUAKE_031_650_SOFT_STATE,

    FUHQUAKE_031_650_SOFT_POSITION,
    FUHQUAKE_031_650_SOFT_POSITION + 4,
    FUHQUAKE_031_650_SOFT_POSITION + 8,

    FUHQUAKE_031_650_SOFT_STATS,
    FUHQUAKE_031_650_SOFT_STATS + 4,
    FUHQUAKE_031_650_SOFT_STATS + 8,
    FUHQUAKE_031_650_SOFT_STATS + 12,
    FUHQUAKE_031_650_SOFT_STATS + 16,
    FUHQUAKE_031_650_SOFT_STATS + 20,
    FUHQUAKE_031_650_SOFT_STATS + 24,
    FUHQUAKE_031_650_SOFT_STATS + 28,
    FUHQUAKE_031_650_SOFT_STATS + 32,
    FUHQUAKE_031_650_SOFT_STATS + 36,
    FUHQUAKE_031_650_SOFT_STATS + 40,
    FUHQUAKE_031_650_SOFT_STATS + 44,
    FUHQUAKE_031_650_SOFT_STATS + 48,
    FUHQUAKE_031_650_SOFT_STATS + 52,
    FUHQUAKE_031_650_SOFT_STATS + 56,
    FUHQUAKE_031_650_SOFT_STATS + 60,
    FUHQUAKE_031_650_SOFT_STATS + 64,

    FUHQUAKE_031_650_SOFT_PRINT1,
    FUHQUAKE_031_650_SOFT_PRINT2,
  },
  {
    MOREQUAKEWORLD_096_SOFT_TITLE,
    //MOREQUAKEWORLD_096_SOFT_WINDOW,
    MOREQUAKEWORLD_096_SOFT_VERSIONTEXT,
    MOREQUAKEWORLD_096_SOFT_ENGINETEXT,    
    MOREQUAKEWORLD_096_SOFT_VERSION,
    MOREQUAKEWORLD_096_SOFT_ENGINE,
    MOREQUAKEWORLD_096_SOFT_MAPNAME,
    MOREQUAKEWORLD_096_SOFT_STATE,

    MOREQUAKEWORLD_096_SOFT_POSITION,
    MOREQUAKEWORLD_096_SOFT_POSITION + 4,
    MOREQUAKEWORLD_096_SOFT_POSITION + 8,

    MOREQUAKEWORLD_096_SOFT_STATS,
    MOREQUAKEWORLD_096_SOFT_STATS + 4,
    MOREQUAKEWORLD_096_SOFT_STATS + 8,
    MOREQUAKEWORLD_096_SOFT_STATS + 12,
    MOREQUAKEWORLD_096_SOFT_STATS + 16,
    MOREQUAKEWORLD_096_SOFT_STATS + 20,
    MOREQUAKEWORLD_096_SOFT_STATS + 24,
    MOREQUAKEWORLD_096_SOFT_STATS + 28,
    MOREQUAKEWORLD_096_SOFT_STATS + 32,
    MOREQUAKEWORLD_096_SOFT_STATS + 36,
    MOREQUAKEWORLD_096_SOFT_STATS + 40,
    MOREQUAKEWORLD_096_SOFT_STATS + 44,
    MOREQUAKEWORLD_096_SOFT_STATS + 48,
    MOREQUAKEWORLD_096_SOFT_STATS + 52,
    MOREQUAKEWORLD_096_SOFT_STATS + 56,
    MOREQUAKEWORLD_096_SOFT_STATS + 60,
    MOREQUAKEWORLD_096_SOFT_STATS + 64,

    MOREQUAKEWORLD_096_SOFT_PRINT1,
    MOREQUAKEWORLD_096_SOFT_PRINT2,
  },
  {
    MOREQUAKEWORLD_096_SOFT2_TITLE,
    //MOREQUAKEWORLD_096_SOFT2_WINDOW,
    MOREQUAKEWORLD_096_SOFT2_VERSIONTEXT,
    MOREQUAKEWORLD_096_SOFT2_ENGINETEXT,    
    MOREQUAKEWORLD_096_SOFT2_VERSION,
    MOREQUAKEWORLD_096_SOFT2_ENGINE,
    MOREQUAKEWORLD_096_SOFT2_MAPNAME,
    MOREQUAKEWORLD_096_SOFT2_STATE,

    MOREQUAKEWORLD_096_SOFT2_POSITION,
    MOREQUAKEWORLD_096_SOFT2_POSITION + 4,
    MOREQUAKEWORLD_096_SOFT2_POSITION + 8,

    MOREQUAKEWORLD_096_SOFT2_STATS,
    MOREQUAKEWORLD_096_SOFT2_STATS + 4,
    MOREQUAKEWORLD_096_SOFT2_STATS + 8,
    MOREQUAKEWORLD_096_SOFT2_STATS + 12,
    MOREQUAKEWORLD_096_SOFT2_STATS + 16,
    MOREQUAKEWORLD_096_SOFT2_STATS + 20,
    MOREQUAKEWORLD_096_SOFT2_STATS + 24,
    MOREQUAKEWORLD_096_SOFT2_STATS + 28,
    MOREQUAKEWORLD_096_SOFT2_STATS + 32,
    MOREQUAKEWORLD_096_SOFT2_STATS + 36,
    MOREQUAKEWORLD_096_SOFT2_STATS + 40,
    MOREQUAKEWORLD_096_SOFT2_STATS + 44,
    MOREQUAKEWORLD_096_SOFT2_STATS + 48,
    MOREQUAKEWORLD_096_SOFT2_STATS + 52,
    MOREQUAKEWORLD_096_SOFT2_STATS + 56,
    MOREQUAKEWORLD_096_SOFT2_STATS + 60,
    MOREQUAKEWORLD_096_SOFT2_STATS + 64,

    MOREQUAKEWORLD_096_SOFT2_PRINT1,
    MOREQUAKEWORLD_096_SOFT2_PRINT2,
  },
  {
    MOREQUAKEWORLD_096_GL_TITLE,
    //MOREQUAKEWORLD_096_GL_WINDOW,
    MOREQUAKEWORLD_096_GL_VERSIONTEXT,
    MOREQUAKEWORLD_096_GL_ENGINETEXT,    
    MOREQUAKEWORLD_096_GL_VERSION,
    MOREQUAKEWORLD_096_GL_ENGINE,
    MOREQUAKEWORLD_096_GL_MAPNAME,
    MOREQUAKEWORLD_096_GL_STATE,

    MOREQUAKEWORLD_096_GL_POSITION,
    MOREQUAKEWORLD_096_GL_POSITION + 4,
    MOREQUAKEWORLD_096_GL_POSITION + 8,

    MOREQUAKEWORLD_096_GL_STATS,
    MOREQUAKEWORLD_096_GL_STATS + 4,
    MOREQUAKEWORLD_096_GL_STATS + 8,
    MOREQUAKEWORLD_096_GL_STATS + 12,
    MOREQUAKEWORLD_096_GL_STATS + 16,
    MOREQUAKEWORLD_096_GL_STATS + 20,
    MOREQUAKEWORLD_096_GL_STATS + 24,
    MOREQUAKEWORLD_096_GL_STATS + 28,
    MOREQUAKEWORLD_096_GL_STATS + 32,
    MOREQUAKEWORLD_096_GL_STATS + 36,
    MOREQUAKEWORLD_096_GL_STATS + 40,
    MOREQUAKEWORLD_096_GL_STATS + 44,
    MOREQUAKEWORLD_096_GL_STATS + 48,
    MOREQUAKEWORLD_096_GL_STATS + 52,
    MOREQUAKEWORLD_096_GL_STATS + 56,
    MOREQUAKEWORLD_096_GL_STATS + 60,
    MOREQUAKEWORLD_096_GL_STATS + 64,

    MOREQUAKEWORLD_096_GL_PRINT1,
    MOREQUAKEWORLD_096_GL_PRINT2,
  },
  {
    MOREQUAKEWORLD_096_GL2_TITLE,
    //MOREQUAKEWORLD_096_GL2_WINDOW,
    MOREQUAKEWORLD_096_GL2_VERSIONTEXT,
    MOREQUAKEWORLD_096_GL2_ENGINETEXT,    
    MOREQUAKEWORLD_096_GL2_VERSION,
    MOREQUAKEWORLD_096_GL2_ENGINE,
    MOREQUAKEWORLD_096_GL2_MAPNAME,
    MOREQUAKEWORLD_096_GL2_STATE,

    MOREQUAKEWORLD_096_GL2_POSITION,
    MOREQUAKEWORLD_096_GL2_POSITION + 4,
    MOREQUAKEWORLD_096_GL2_POSITION + 8,

    MOREQUAKEWORLD_096_GL2_STATS,
    MOREQUAKEWORLD_096_GL2_STATS + 4,
    MOREQUAKEWORLD_096_GL2_STATS + 8,
    MOREQUAKEWORLD_096_GL2_STATS + 12,
    MOREQUAKEWORLD_096_GL2_STATS + 16,
    MOREQUAKEWORLD_096_GL2_STATS + 20,
    MOREQUAKEWORLD_096_GL2_STATS + 24,
    MOREQUAKEWORLD_096_GL2_STATS + 28,
    MOREQUAKEWORLD_096_GL2_STATS + 32,
    MOREQUAKEWORLD_096_GL2_STATS + 36,
    MOREQUAKEWORLD_096_GL2_STATS + 40,
    MOREQUAKEWORLD_096_GL2_STATS + 44,
    MOREQUAKEWORLD_096_GL2_STATS + 48,
    MOREQUAKEWORLD_096_GL2_STATS + 52,
    MOREQUAKEWORLD_096_GL2_STATS + 56,
    MOREQUAKEWORLD_096_GL2_STATS + 60,
    MOREQUAKEWORLD_096_GL2_STATS + 64,

    MOREQUAKEWORLD_096_GL2_PRINT1,
    MOREQUAKEWORLD_096_GL2_PRINT2,
  },
  {
    GREYMQUAKE_011S_GL_TITLE,
    //GREYMQUAKE_011S_GL_WINDOW,
    GREYMQUAKE_011S_GL_VERSIONTEXT,
    GREYMQUAKE_011S_GL_ENGINETEXT,
    GREYMQUAKE_011S_GL_VERSION,
    GREYMQUAKE_011S_GL_ENGINE,
    GREYMQUAKE_011S_GL_MAPNAME,
    GREYMQUAKE_011S_GL_STATE,

    GREYMQUAKE_011S_GL_POSITION,
    GREYMQUAKE_011S_GL_POSITION + 4,
    GREYMQUAKE_011S_GL_POSITION + 8,

    GREYMQUAKE_011S_GL_STATS,
    GREYMQUAKE_011S_GL_STATS + 4,
    GREYMQUAKE_011S_GL_STATS + 8,
    GREYMQUAKE_011S_GL_STATS + 12,
    GREYMQUAKE_011S_GL_STATS + 16,
    GREYMQUAKE_011S_GL_STATS + 20,
    GREYMQUAKE_011S_GL_STATS + 24,
    GREYMQUAKE_011S_GL_STATS + 28,
    GREYMQUAKE_011S_GL_STATS + 32,
    GREYMQUAKE_011S_GL_STATS + 36,
    GREYMQUAKE_011S_GL_STATS + 40,
    GREYMQUAKE_011S_GL_STATS + 44,
    GREYMQUAKE_011S_GL_STATS + 48,
    GREYMQUAKE_011S_GL_STATS + 52,
    GREYMQUAKE_011S_GL_STATS + 56,
    GREYMQUAKE_011S_GL_STATS + 60,
    GREYMQUAKE_011S_GL_STATS + 64,

    GREYMQUAKE_011S_GL_PRINT1,
    GREYMQUAKE_011S_GL_PRINT2,
  },
  {
    QUAKEWORLD_233_SOFT_TITLE,
    //QUAKEWORLD_233_SOFT_WINDOW,
    QUAKEWORLD_233_SOFT_VERSIONTEXT,
    QUAKEWORLD_233_SOFT_ENGINETEXT,
    QUAKEWORLD_233_SOFT_VERSION,
    QUAKEWORLD_233_SOFT_ENGINE,
    QUAKEWORLD_233_SOFT_MAPNAME,
    QUAKEWORLD_233_SOFT_STATE,

    QUAKEWORLD_233_SOFT_POSITION,
    QUAKEWORLD_233_SOFT_POSITION + 4,
    QUAKEWORLD_233_SOFT_POSITION + 8,

    QUAKEWORLD_233_SOFT_STATS,
    QUAKEWORLD_233_SOFT_STATS + 4,
    QUAKEWORLD_233_SOFT_STATS + 8,
    QUAKEWORLD_233_SOFT_STATS + 12,
    QUAKEWORLD_233_SOFT_STATS + 16,
    QUAKEWORLD_233_SOFT_STATS + 20,
    QUAKEWORLD_233_SOFT_STATS + 24,
    QUAKEWORLD_233_SOFT_STATS + 28,
    QUAKEWORLD_233_SOFT_STATS + 32,
    QUAKEWORLD_233_SOFT_STATS + 36,
    QUAKEWORLD_233_SOFT_STATS + 40,
    QUAKEWORLD_233_SOFT_STATS + 44,
    QUAKEWORLD_233_SOFT_STATS + 48,
    QUAKEWORLD_233_SOFT_STATS + 52,
    QUAKEWORLD_233_SOFT_STATS + 56,
    QUAKEWORLD_233_SOFT_STATS + 60,
    QUAKEWORLD_233_SOFT_STATS + 64,

    QUAKEWORLD_233_SOFT_PRINT1,
    QUAKEWORLD_233_SOFT_PRINT2,
  },
  {
    QUAKEWORLD_233_GL_TITLE,
    //QUAKEWORLD_233_GL_WINDOW,
    QUAKEWORLD_233_GL_VERSIONTEXT,
    QUAKEWORLD_233_GL_ENGINETEXT,
    QUAKEWORLD_233_GL_VERSION,
    QUAKEWORLD_233_GL_ENGINE,
    QUAKEWORLD_233_GL_MAPNAME,
    QUAKEWORLD_233_GL_STATE,

    QUAKEWORLD_233_GL_POSITION,
    QUAKEWORLD_233_GL_POSITION + 4,
    QUAKEWORLD_233_GL_POSITION + 8,

    QUAKEWORLD_233_GL_STATS,
    QUAKEWORLD_233_GL_STATS + 4,
    QUAKEWORLD_233_GL_STATS + 8,
    QUAKEWORLD_233_GL_STATS + 12,
    QUAKEWORLD_233_GL_STATS + 16,
    QUAKEWORLD_233_GL_STATS + 20,
    QUAKEWORLD_233_GL_STATS + 24,
    QUAKEWORLD_233_GL_STATS + 28,
    QUAKEWORLD_233_GL_STATS + 32,
    QUAKEWORLD_233_GL_STATS + 36,
    QUAKEWORLD_233_GL_STATS + 40,
    QUAKEWORLD_233_GL_STATS + 44,
    QUAKEWORLD_233_GL_STATS + 48,
    QUAKEWORLD_233_GL_STATS + 52,
    QUAKEWORLD_233_GL_STATS + 56,
    QUAKEWORLD_233_GL_STATS + 60,
    QUAKEWORLD_233_GL_STATS + 64,

    QUAKEWORLD_233_GL_PRINT1,
    QUAKEWORLD_233_GL_PRINT2,
  }
};

typedef struct keys_s {
  char *name;
  char *desc;
  short code;
} keys_t;

#ifdef WIN32
#ifndef VK_SLEEP
#define VK_SLEEP 0x5F
#endif
#define MAX_KEY_CODES 64
keys_t special_keys[MAX_KEY_CODES] =
{
  {"backspace", "Backspace", VK_BACK},
  {"tab", "Tab", VK_TAB},
  {"enter", "Enter", VK_RETURN},
  {"shift", "Shift", VK_SHIFT},
  {"ctrl", "Control", VK_CONTROL},
  {"alt", "Alt", VK_MENU},
  {"pause", "Pause", VK_PAUSE},
  {"caps", "Caps Lock", VK_CAPITAL},
  {"esc", "Escape", VK_ESCAPE},
  {"space", "Space Bar", VK_SPACE},
  {"pgup", "Page Up", VK_PRIOR},
  {"pgdn", "Page Down", VK_NEXT},
  {"end", "End", VK_END},
  {"home", "Home", VK_HOME},
  {"leftarrow", "Left Arrow", VK_LEFT},
  {"uparrow", "Up Arrow", VK_UP},
  {"rightarrow", "Right Arrow", VK_RIGHT},
  {"downarrow", "Down Arrow", VK_DOWN},
  {"select", "Select", VK_SELECT},
  {"print", "Print", VK_PRINT},
  {"execute", "Execute", VK_EXECUTE},
  {"printscreen", "Print Screen", VK_SNAPSHOT},
  {"ins", "Insert", VK_INSERT},
  {"del", "Delete", VK_DELETE},
  {"help", "Help", VK_HELP},
  {"leftwin", "Left Windows Button", VK_LWIN},
  {"rightwin", "Right Windows Button", VK_RWIN},
  {"menu", "Application Menu", VK_APPS},
  {"sleep", "Sleep", VK_SLEEP},
  {"numpad0", "Numpad 0", VK_NUMPAD0},
  {"numpad1", "Numpad 1", VK_NUMPAD1},
  {"numpad2", "Numpad 2", VK_NUMPAD2},
  {"numpad3", "Numpad 3", VK_NUMPAD3},
  {"numpad4", "Numpad 4", VK_NUMPAD4},
  {"numpad5", "Numpad 5", VK_NUMPAD5},
  {"numpad6", "Numpad 6", VK_NUMPAD6},
  {"numpad7", "Numpad 7", VK_NUMPAD7},
  {"numpad8", "Numpad 8", VK_NUMPAD8},
  {"numpad9", "Numpad 9", VK_NUMPAD9},
  {"numpad*", "Numpad *", VK_MULTIPLY},
  {"numpad+", "Numpad +", VK_ADD},
  {"numpad-", "Numpad -", VK_SUBTRACT},
  {"numpad.", "Numpad .", VK_DECIMAL},
  {"numpad/", "Numpad /", VK_DIVIDE},
  {"numlock", "Numlock", VK_NUMLOCK},
  {"scroll", "Scroll Lock", VK_SCROLL},
  {"leftshift", "Left Shift", VK_LSHIFT},
  {"rightshift", "Right Shift", VK_RSHIFT},
  {"leftctrl", "Left Control", VK_LCONTROL},
  {"rightctrl", "Right Control", VK_RCONTROL},
  {"leftmenu", "Left Application Menu", VK_LMENU},
  {"rightmenu", "Right Application Menu", VK_RMENU},
  {"f1", "F1", VK_F1},
  {"f2", "F2", VK_F2},
  {"f3", "F3", VK_F3},
  {"f4", "F4", VK_F4},
  {"f5", "F5", VK_F5},
  {"f6", "F6", VK_F6},
  {"f7", "F7", VK_F7},
  {"f8", "F8", VK_F8},
  {"f9", "F9", VK_F9},
  {"f10", "F11", VK_F10},
  {"f11", "F12", VK_F11},
  {"f12", "F13", VK_F12},
};
#endif

//From the Quake source code (possibly ZQuake, FuhQuake or vanilla)
#define VectorSubtract(a,b,c)	((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define SKIPBLANKS(ptr) while (*ptr == ' ' || *ptr == 9 || *ptr == 13) ptr++
#define SKIPTOEOL(ptr) while (*ptr != 10 && *ptr == 0) ptr++

typedef void (*func_t) (void);

typedef struct loc_data_s {
	vec3_t coord;
	char name[MAX_LOC_NAME+1];
	struct loc_data_s *next;
} loc_data_t;

typedef enum {
  ca_free,
  ca_disconnected,
  ca_connecting,
  ca_connected,
  ca_shutdown
} client_state_t;

typedef enum {
  sa_free,
  sa_shutdown
} server_state_t;

typedef struct client_stats_s {
  char name[MAX_NAME+1];
  char macro[MAX_MACRO+1];
  vec3_t pos;
  int carrying;
  int armour;
  int health;
  int weapon; //active weapon
  int shells;
  int nails;
  int rockets;
  int cells;
} client_stats_t;

typedef struct cvar_s {
  char name[MAX_KEY+1];
  char string[MAX_VALUE+1];
  int value;
  int type; //0=builtin,1=user
  func_t function;
  struct cvar_s *next;
} cvar_t;

typedef struct sv_client_s {
  char name[MAX_NAME+1];
  char stats[MAX_STAT_SIZE+1];
  int time;
  int id;
  int update;
  client_state_t state;
  struct sockaddr_in addr;
} sv_client_t;

typedef struct sv_server_s {
  int sock;
  struct sockaddr_in addr;
  //int port;
  int active;
  //char password[MAX_PASSWORD+1];
  //int debug;
  //int self;
  int hsz;
  int bytes_in;
  int bytes_out;
  int input_state;
  unsigned long realtime;
  server_state_t state;
  cvar_t *cvars;
#ifdef WIN32
  HANDLE th_input;
  HANDLE th_main;
#endif
#ifdef UNIX
  pthread_t th_input;
  pthread_t th_main;
#endif
} sv_server_t;

typedef struct cl_client_s {
  int active;
  //char name[MAX_NAME+1];
  //char password[MAX_PASSWORD+1];
  char last_map[MAX_FILE_NAME+1];
  vec3_t pos;
  //char full_stat[MAX_FULL_STAT+1];
  //int stat_len;
  int time;
  int id;
  //int debug;
  int last_ping;
  int pong;
  int bytes_in;
  int bytes_out;
  int hsz;
  //int align;
  //int auto_connect;
  //int auto_shutdown;
  //int client_port;
  //int server_port;
  //char server_ip[MAX_IP+1];
  //char server_address[MAX_ADDRESS+1];
  //char loc_dir[MAX_FILE_PATH+1];
  //char report_line[MAX_REPORT_LINE+1];
  short hotkey;
  int hotkey_down;
#ifdef WIN32
  HKL keyboard;
#endif
  int input_state;
  int realtime;
  client_state_t state;
  cvar_t *cvars;
  //client_stats_t stats;
  loc_data_t *nodes;
  //unsigned long mem[MAX_MEM];
  unsigned long quake_exit;
  unsigned long quakeid;
#ifdef WIN32
  SOCKET sock;
  SOCKADDR_IN s2c;
  SOCKADDR_IN c2s;
  SOCKADDR_IN c2q;
  int quake_client;
  HWND quake;
  HANDLE hook;
  HANDLE th_read;
  HANDLE th_client;
  HANDLE th_input;
  //HANDLE th_hook;
  CRITICAL_SECTION mutex;
#endif
} cl_client_t;

int cl_init(void);
int cl_connect(char *address);
void cl_disconnect(void);
int cl_read_cfg(void);
int cl_hook_quake(void);
//void *cl_find_key(char *name, int *size);
void *cl_read_mem(void);
void *cl_client_loop(void);
void *cl_input(void);
void cl_think(void);
void cl_ping(void);
void cl_pong(void);
void cl_bad_connect(char *buff);
void cl_good_connect(char *buff);
void cl_print(char *buff);
void cl_no_update(void);
void cl_server_quit(void);
//void cl_stat_update(char *buff, int sz);
void cl_init_stat(client_stats_t *stats);
void cl_macro(client_stats_t *stats, int type);
void cl_parse_chars(char *s, int red, int len);
void cl_stat_print(char *buff, int sz);
void cl_shutdown(void);
void cl_name_change(void);
void cl_password_change(void);
void cl_hotkey_change(void);
void cl_request_stat(void);
void cl_hotkey(void);
void cl_key_list(void);
void cl_hello(void);
void cl_timeout(void);
int cl_highlight(char *s);
//void cl_have_ip(void);
int cl_strip_ammo(int items);
void cl_show_print(int mode);
void cl_print_net(void);
void cl_check_map(char *s);
int cl_new_node(loc_data_t *node);
void cl_clear_loc(void);
int cl_load_loc(char *loc);
int cl_location(vec3_t loc, char *buff, int len);
int cl_read_proc(unsigned long *memory, char *buff, int size, int num, int quiet);
int cl_write_proc(unsigned long *memory, char *buff, int size, int num);
void cl_red_text(char *str);
int cl_highlight(char *s);
int cl_server_message(char tag, char *msg, int len);
void cl_quake_message(char *msg);
cvar_t *cl_find_cvar(char *name);
void cl_list_cvars(void);
int cl_cvar_value(char *name);
char *cl_cvar_string(char *name);
int cl_new_cvar(cvar_t *cvar);
void cl_change_cvar(cvar_t *cvar, char *value);
void cl_add_cvar(char *name, char *value, int type, func_t function);
void cl_add_user_cvar(char *s);
void cl_remove_cvars(void);

//gcc -o personification personification.c util.c -DUNIX -lm -pthread
int sv_init(int argc, char *argv[]);
void *sv_server_loop(void);
void *sv_input(void);
void sv_think(void);
//void *sv_send_stat(void);
void sv_shutdown(void);
int sv_find_slot(int id);
void sv_connect(char *buff);
void sv_disconnect(char *buff);
void sv_zombies(void);
void sv_update_stat(char *buff, int sz);
void sv_request_stat(char *buff);
void sv_no_update(char *buff);
void sv_ping(char *buff);
void sv_client_message(char tag, int slot, char *msg, int len);
void sv_outside_message(char tag, char *msg);
int sv_compare_addr(struct sockaddr_in *a, struct sockaddr_in *b);
void sv_update_time(int slot);
void sv_status(void);
void sv_print_net(void);
int sv_read_cfg(void);
cvar_t *sv_find_cvar(char *name);
int sv_new_cvar(cvar_t *cvar);
void sv_list_cvars(void);
char *sv_cvar_string(char *name);
int sv_cvar_value(char *name);
void sv_change_cvar(cvar_t *cvar, char *value);
void sv_add_cvar(char *name, char *value, int type, func_t function);
//void sv_add_user_cvar(char *s);
void sv_remove_cvars(void);
void sv_max_clients_change(void);
void sv_password_change(void);

#ifdef WIN32
#define _strnprintf _snprintf
#define _endthread return 0
#define _threadvoid LPVOID
#define _close(s) closesocket(s)
#define _sleep(t) Sleep(t)
#endif
#ifdef UNIX
#define _strnprintf snprintf
#define _endthread pthread_exit(NULL)
#define _threadvoid void
#define _close(s) close(s)
#define _sleep(t) usleep(t * 1000)
#endif

