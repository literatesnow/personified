Personified v1.25 - Copyright (C) 2004 bliP
Web: http://nisda.net
Email: scrag [at] nisda [dot] net
IRC: #nzgames irc.enterthegame.com
Compiled: v1.0: 2004-06-13, v1.25: 2004-07-21

Personified is a player location reporter for QuakeWorld.
Players connected to personification server can see the location and details
of the other players there.
We originally had:
  bind "x" "say_team $\report location"
  msg_trigger reportloc "report location" -l t
  alias reportloc "say_team I'm at %l (%a/%h)"
This worked well, but it takes up chat space and often other people would have
the trigger that you didn't need to know about. Older clients also do not
have msg_trigger.

Files:
personified.exe - win32 client
personification.exe - win32 server
personification - linux server
personified.cfg - client configuration

Supported Clients:
* FuhQuake 0.31 GL
* FuhQuake 0.31 Soft
* MoreQuakeWorld 0.96 GL
* MoreQuakeWorld 0.96 GL2
* MoreQuakeWorld 0.96 Soft
* MoreQuakeWorld 0.96 Soft2
* GreyMQuake 0.11s GL
* QuakeWorld 2.33 GL
* QuakeWorld 2.33 Soft

Quick start:
1) Start personification (if you're hosting server)
2) Start personified
3) Start the QuakeWorld client and connect to a QuakeWorld server
4) In personified's console, type: connect (personification's ip:port)
5) Either bind a Quake key to "packet 127.0.0.1:27104 report" or set a
   hotkey in personified.

-- Personified --

Console commands/shortcut:
connect/c <ip:port> - if no ip:port is specified, the last server will be used
disconnect/d - disconnect from personification server
quit/q - close personified
load/l - reload personified.cfg
loadloc/o - (re)load mapname.loc
net/n - display bytes received/bytes sent
add/a <name> <value> - add new cvar
cvarlist/v - list all cvars and values

Console variables:
name - your name to be displayed on other's report
server - default personification server
password - default password
loc_dir - directory your locs are stored
local_port - client local port to use
auto_connect - connect to personification after quake is detected
auto_close - quit personified after quake exits
align - wether or not to align text in report (1 = left, 2 = right)
self - display yourself in report messages
messages - display extra messages (0-3)
hotkey - the key to display report in Quake (Use keylist to view names of
  special keys, these are case sensitive)
  To bind custom keys, use \xFF (where FF is the Virtual Key in hex)
highlight - change led to yellow if this text is in the current location
  seperate items with commas, for example: flag,quad,eyes
report - customise the report line
 %N - other player's name
 %e - led (blue: normal, yellow: highlighted, red: has a powerup)
 %l - location
 %p - powerups in " powerup" format
 %P - powerups in gold bracket " [powerup]" format
 %w - current weapon
 %b - best weapon
 %B - best weapon:ammo
 %c - cells
 %n - nails
 %r - rockets
 %s - shells
 %u - need (health/armour/weapon/ammo)
 %A - armour type
 %a - armour
 %h - health
 %i - armour in gold brackets if low
 %o - armour in gold brackets if low
 %t - trailing spaces to align text
 {} - can be used to turn text {white}
 ^  - turns numbers into gold numbers (%a^/%h^)
 $  - "fun" chars and $cvars are parsed normally

Customise variables (name - default):
name_green_armour - ga
name_yellow_armour - ya
name_red_armour - ra
name_no_armour - a
name_armour - armour
name_health - health
name_weapon - weapon
name_weapon_axe - axe
name_weapon_shotgun - sg
name_weapon_super_shotgun - ssg
name_weapon_nailgun - ng
name_weapon_super_nailgun - sng
name_weapon_grenade_launcher - gl
name_weapon_rocket_launcher - rl
name_weapon_lightning_gun - lg
name_powerup_key - flag
name_powerup_quad_damage - quad
name_powerup_pentagram - 666
name_powerup_invisibility - eyes
name_rockets - rockets
name_cells - cells
name_nails - nails
name_shells - shells
need_armour - 50
need_health - 50
need_weapon - 0
need_rockets - 5
need_cells - 20
need_nails - 40
need_shells - 10
name_someplace - someplace
name_nothing - nothing
seperator - /

Hotkey:
Displays report in Quake's console of all the people connected to
personification, only you will see this
example: z: hotkey z
         capslock: hotkey caps
         mouse 1 (VK_LBUTTON): hotkey \x01
         disable: hotkey none

Quake Packet commands:
hello - simple reply, for testing
report - displays report in Quake's console of all the people
         connected to personification, only you will see this

Quake Packet:
usage: packet <ip:port>
ip - personified's address (default: 127.0.0.1)
port - personifed's local port (default: 27104)
bind "x" "packet 127.0.0.1:27104 report"

Example personified.cfg:
name=Someone
server=127.0.0.1:27105
password=none
hotkey=z
loc_dir=c:\quake\qw\locs
report=%N %e%e %l%P (%a^%A/%h^)
auto_connect=1
auto_close=0
align=1
hotkey=none

Fuhquake locs:
To use fuhquake locs, add the following lines to personified.cfg:
loc_name_ra=RA
loc_name_ya=YA
loc_name_ga=GA
loc_name_mh=MEGA
loc_name_separator= 
(note: there is a space character after loc_name_separator)

-- Personification --

Command line:
 -p [port] local server port to use (default: 27105)
 -w [1/0]  server password
 -s [1/0]  self mode
 -d [1/0]  debug mode

Console commands/shortcut:
quit/q - close server
status/s - display connected users and info
net/n - display bytes received/bytes sent
cvarlist/v - display all cvars

Console variables:
password - server password
max_clients - maximum number of clients allowed to connect
hostname - server description
local_port - server local port to use
messages - display extra messages (0-3)

FAQ
Q. Isn't this kinda..cheaty?
A. No, you can achieve the same effect with QuakeWorld's scripting.

Q. My Quake isn't detected?
A. If your client is supported, sometimes you need to start personified
   first before you start Quake and/or connect Quake to a server.

Q. I packet personified and nothing happens?!
A. There's probably no one else there. If you get a reply from packeting
   "hello" then set "self" to 1.

Q. When I packet personified and get "bad packet"?
A. You've got the wrong port and are packeting personification by mistake.

Q. I can't set my hotkey to the key I want?
A. Find the virtual key code from MSDN, and failing that, use packet.

Q. Who's locs are used?
A. Your own. 

Q. You spelt "armour" wrong!
A. No I didn't.

Thanks to:
Suggestions/testing - The Burning and Paradoks.
Maths - Jax and TC.

Please send all questions, bug reports, comments and suggestions
to the email address provided above.

Disclaimer:
ANY USE BY YOU OF THE SOFTWARE IS AT YOUR OWN RISK. THE SOFTWARE IS
PROVIDED FOR USE "AS IS" WITHOUT WARRANTY OF ANY KIND. THIS SOFTWARE
IS RELEASED AS "FREEWARE". REDISTRIBUTION IS ONLY ALLOWED IF THE
SOFTWARE IS UNMODIFIED, NO FEE IS CHARGED FOR THE SOFTWARE, DIRECTLY
OR INDIRECTLY WITHOUT THE EXPRESS PERMISSION OF THE AUTHOR.

