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

#include "personified.h"

cl_client_t client;

int main(void)
{
  if (!cl_init())
    return 0;
  cl_think();
  cl_shutdown();
  return 1;
}

void *cl_client_loop(void)
{
  char buff[MAX_FULL_STAT+1];
  char *s;
  int sz;

  while (client.active) {
    Sleep(35);
    if (client.state == ca_free)
      continue;
    cl_timeout();
    cl_ping();
    cl_hotkey();
    sz = recvfrom(client.sock, buff, sizeof(buff), 0, (LPSOCKADDR)&client.s2c, &client.hsz);
    if (sz == -1)
      continue;
    client.bytes_in += sz;
    buff[sz] = '\0';
    s = buff;
    if (*s == A2A_PER_ID) {
      switch (buff[1]) {
        case S2C_PONG: cl_pong(); break;
        case S2C_BAD_CONNECT: cl_bad_connect(buff + 2); break;
        case S2C_GOOD_CONNECT: cl_good_connect(buff + 2); break;
        case S2C_PRINT: cl_print(buff + 2); break;
        case S2C_SHUTDOWN: cl_server_quit(); break;
        //case S2C_STAT_UPDATE: cl_stat_update(buff + 2, sz - 2); break;
        case S2C_STAT_PRINT: cl_stat_print(buff + 2, sz - 2); break;
        default: break;
      }
      continue;
    }
    if (*s == A2A_QUAKE_ID) {
      s += 4;
      if (!strcmp(s, "hello"))
        cl_hello();
      else if (!strcmp(s, "report") || !strcmp(s, "r"))
        cl_request_stat();
      continue;
    }
  }
  return 0;
}

#if 0
memprint(char *title, char *data, int size)
{
  int i;

  printf("%s == ", title);
  for (i = 0; i < size; i++, *data++) {
    printf("%c", (*data < 32) ? '.' : *data);
  }
  printf("\n");
}
#endif

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
  char title[64];

  GetWindowText(hWnd, title, sizeof(title));
  if (strstr(title, QUAKE_WINDOW_TITLE)) {
    client.quake = hWnd;
    if (cl_hook_quake())
      return 0; //done
    client.quake = NULL;
  }
  return 1;
}

int cl_hook_quake(void)
{
  char mem[256];
  char *s;
  int i;

  GetWindowThreadProcessId(client.quake, &client.quakeid);
  client.hook = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, client.quakeid);
  if (!client.hook) {
    //printf("-- detect failed\n");
    return 0;
  }
  for (i = 0; i < MAX_SUPPORTED_QUAKE; i++) {
    cl_read_proc(&quake[i].version, &*mem, sizeof(mem), sizeof(mem), TRUE);
    //memprint(quake[i].title, mem, sizeof(mem));
    if (memcmp(mem, quake[i].versiontext, strlen(quake[i].versiontext)))
      continue;

    cl_read_proc(&quake[i].engine, &*mem, sizeof(mem), sizeof(mem), TRUE);
    //memprint(quake[i].title, mem, sizeof(mem));
    mem[sizeof(mem)-1] = '\0';
    s = mem;
    while (*s)
      *s++ = toupper(*s);
    if (!strstr(mem, quake[i].enginetext))
      continue;

    printf("-- detected: %s\n", quake[i].title);
    client.quake_client = i;
    return 1;
  }
  return 0;
}

void cl_find_quake(void)
{
  printf("-- detecting quake...\n");
  while (client.active) {
    Sleep(500);
    EnumWindows(EnumWindowsProc, 0);
    if (!client.quake)
      continue;
    client.quake = NULL; //for next detect
    client.state = ca_disconnected;
    if (cl_cvar_value(CVAR_AUTO_CONNECT))
      cl_connect(cl_cvar_string(CVAR_SERVER));
    return;
  }
}

void *cl_read_mem(void)
{
  char pkt[MAX_STAT_SIZE+1];
  char buff[64];
  char last_loc[MAX_LOC_NAME+1];
  int last_stat[8];
  int stat[8];
  int time;
  int update;

  memset(stat, '\0', sizeof(stat));
  memset(last_stat, '\0', sizeof(last_stat));
  memset(last_loc, '\0', sizeof(last_loc));
  time = client.realtime;
  
  while (client.active) {
    Sleep(150);

    if (!GetExitCodeProcess(client.hook, &client.quake_exit)) {
      cl_find_quake();
      update = 1;
      continue;
    }

    if (client.quake_exit != STILL_ACTIVE) {
      if (cl_cvar_value(CVAR_AUTO_CLOSE)) {
        client.state += ca_shutdown;
        continue;
      }
      cl_disconnect();
      client.state = ca_free;
      CloseHandle(client.hook);
      client.hook = NULL;
      continue;
    }

    if (client.state != ca_connected)
      continue;

    if (!cl_read_proc(&quake[client.quake_client].state, &*buff, 4, 4, FALSE))
      return 0;
    if (readint(buff) != MEM_STATE_ACTIVE) {
      if (update) {
        cl_no_update();
        update = 0;
      }
      continue;
    }

    update = 1;
    pkt[0] = (unsigned char)client.id;
    if (!cl_read_proc(&quake[client.quake_client].mapname, &*buff, sizeof(buff), sizeof(buff), FALSE))
      return 0;
    cl_check_map(buff);

    if (!cl_read_proc(&quake[client.quake_client].posx, &*buff, 4, 4, FALSE))
      return 0;
    memcatz(pkt, 1, buff, 4, sizeof(pkt));
    client.pos[0] = readfloat(buff);

    if (!cl_read_proc(&quake[client.quake_client].posy, &*buff, 4, 4, FALSE))
      return 0;
    memcatz(pkt, 5, buff, 4, sizeof(pkt));
    client.pos[1] = readfloat(buff);
    
    if (!cl_read_proc(&quake[client.quake_client].posz, &*buff, 4, 4, FALSE))
      return 0;
    memcatz(pkt, 9, buff, 4, sizeof(pkt));
    client.pos[2] = readfloat(buff);
    
    if (!cl_read_proc(&quake[client.quake_client].items, &*buff, 4, 4, FALSE))
      return 0;
    memcatz(pkt, 13, buff, 4, sizeof(pkt));
    stat[0] = cl_strip_ammo(readint(buff));

    if (!cl_read_proc(&quake[client.quake_client].armour, &*buff, 4, 4, FALSE))
      return 0;
    stat[1] = bound(0, 255, readint(buff));
    *buff = stat[1];
    memcatz(pkt, 17, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].health, &*buff, 4, 4, FALSE))
      return 0;
    stat[2] = bound(0, 255, readint(buff));
    *buff = stat[2];
    memcatz(pkt, 18, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].activeweapon, &*buff, 4, 4, FALSE))
      return 0;
    stat[3] = bound(0, 255, readint(buff));
    *buff = stat[3];
    memcatz(pkt, 19, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].shells, &*buff, 4, 4, FALSE))
      return 0;
    stat[4] = bound(0, 255, readint(buff));
    *buff = stat[4];
    memcatz(pkt, 20, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].nails, &*buff, 4, 4, FALSE))
      return 0;
    stat[5] = bound(0, 255, readint(buff));
    *buff = stat[5];
    memcatz(pkt, 21, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].rockets, &*buff, 4, 4, FALSE))
      return 0;
    stat[6] = bound(0, 255, readint(buff));
    *buff = stat[6];
    memcatz(pkt, 22, buff, 1, sizeof(pkt));
    
    if (!cl_read_proc(&quake[client.quake_client].cells, &*buff, 4, 4, FALSE))
      return 0;
    stat[7] = bound(0, 255, readint(buff));
    *buff = stat[7];
    memcatz(pkt, 23, buff, 1, sizeof(pkt));

    if (!cl_location(client.pos, buff, sizeof(buff))) {
      if (!(client.realtime - time))
        continue;
    }
    else if (!strcmp(last_loc, buff) && !memcmp(last_stat, stat, sizeof(stat)))
      continue;

    cl_server_message(C2S_UPDATE_STAT, pkt, MAX_STAT_SIZE + 1);
    memcpy(last_stat, stat, sizeof(stat));
    strlcpy(last_loc, buff, sizeof(buff));
    time = client.realtime;
    if (cl_cvar_value(CVAR_MESSAGES) == 2)
      printf("-> stat update #%d\n", client.realtime);
      //printf("-> stat update #%d (%d,%d,%d %d,%d,%d,%d,%d %s)\n", client.realtime, stat[0], stat[1], stat[2], stat[3], stat[4], stat[5], stat[6], stat[7], last_loc);
  }
  return 0;
}

void *cl_input(void)
{
  char line[MAX_KEY+1+MAX_VALUE+1+1];
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  char *s;
  char c;
  //void *v;
  int i;
  cvar_t *cvar;

  while (client.active) {
    client.input_state = 1;
    s = line;
    i = 0;
    while ((c = getchar()) != '\n') {
      if (i < sizeof(line)-1) {
        *s++ = c;
        i++;
      }
    }
    *s = '\0';
    if (!strlen(line))
      continue;
    client.input_state = 0;
    tokenize(line, key, sizeof(key), value, sizeof(value), ' ');
    cvar = cl_find_cvar(key);
    if (cvar) {
      if (*value)
        cl_change_cvar(cvar, value);
      else
        printf("-- \"%s\" is \"%s\"\n", cvar->name, cvar->string);
    }
    /*v = cl_find_key(key, &i);
    if (v) {
      if (value[0]) {
        if (i)
          strlcpy((char *)v, value, i);
        else
          *(int *)v = atoi(value);
        printf("-- changed \"%s\" to \"%s\"\n", key, value);
      }
      else {
        if (i)
          printf("-- \"%s\" is \"%s\"\n", key, (char *)v);
        else
          printf("-- \"%s\" is \"%d\"\n", key, *(int *)v);
      }
    }*/
    else if (!strcmp(key, "quit") || !strcmp(key, "q"))
      client.state += ca_shutdown;
    else if (!strcmp(key, "connect") || !strcmp(key, "c"))
      cl_connect(value[0] ? value : cl_cvar_string(CVAR_SERVER));
    else if (!strcmp(key, "disconnect") || !strcmp(key, "d"))
      cl_disconnect();
    else if (!strcmp(key, "load") || !strcmp(key, "l"))
      cl_read_cfg();
    else if (!strcmp(key, "loadloc") || !strcmp(key, "o"))
      cl_load_loc(value);
    else if (!strcmp(key, "net") || !strcmp(key, "n"))
      cl_print_net();
    else if (!strcmp(key, "cvarlist") || !strcmp(key, "v"))
      cl_list_cvars();
    else if (!strcmp(key, "add") || !strcmp(key, "a"))
      cl_add_user_cvar(value);
    else if (!strcmp(key, "keylist") || !strcmp(key, "k"))
      cl_key_list();
    else
      printf("-- unknown command \"%s\"\n", key);
  }
  return 0;
}

int cl_init(void)
{
  WSADATA wsaData;
  unsigned long i = 1;
  char s[MAX_ADDRESS+1];

  printf("%s v%s - Copyright %s %s (%s)\n\n", PER_HEADER, PER_VERSION, COPYRIGHT, AUTHOR, WEB);

  client.active = 1;
  client.state = ca_free;
  client.realtime = 0;
  client.time = 0;
  client.quake_client = -1;
  client.input_state = 1;
  client.bytes_in = 0;
  client.bytes_out = 0;
  client.hotkey = 0;
  client.keyboard = GetKeyboardLayout(0);
  client.hotkey_down = 0;
  client.hook = NULL;
  client.hsz = sizeof(SOCKADDR);

  cl_add_cvar(CVAR_NAME, "unnamed", 0, cl_name_change);
  _snprintf(s, sizeof(s), "127.0.0.1:%d", PSV_SERVER_PORT);
  cl_add_cvar(CVAR_SERVER, s, 0, NULL);
  _snprintf(s, sizeof(s), "%d", PER_CLIENT_PORT);
  cl_add_cvar(CVAR_LOCAL_PORT, s, 0, NULL);
  cl_add_cvar(CVAR_PASSWORD, "none", 0, cl_password_change);
  cl_add_cvar(CVAR_LOC_DIR, ".\\locs", 0, NULL);
  cl_add_cvar(CVAR_REPORT, "%N @ %l (%a/%h)", 0, NULL);
  cl_add_cvar(CVAR_AUTO_CONNECT, "0", 0, NULL);
  cl_add_cvar(CVAR_AUTO_CLOSE, "0", 0, NULL);
  cl_add_cvar(CVAR_ALIGN, "0", 0, NULL);
  cl_add_cvar(CVAR_MESSAGES, "1", 0, NULL);
  cl_add_cvar(CVAR_SELF, "0", 0, NULL);
  cl_add_cvar(CVAR_HIGHLIGHT, "flag", 0, NULL);
  cl_add_cvar(CVAR_HOTKEY, "z", 0, cl_hotkey_change);
  cl_add_cvar(CVAR_NAME_GREEN_ARMOUR, "ga", 0, NULL);
  cl_add_cvar(CVAR_NAME_YELLOW_ARMOUR, "ya", 0, NULL);
  cl_add_cvar(CVAR_NAME_RED_ARMOUR, "ra", 0, NULL);
  cl_add_cvar(CVAR_NAME_NO_ARMOUR, "a", 0, NULL);
  cl_add_cvar(CVAR_NAME_ARMOUR, "armour", 0, NULL);
  cl_add_cvar(CVAR_NAME_HEALTH, "health", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON, "weapon", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_AXE, "axe", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_SHOTGUN, "sg", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_SUPER_SHOTGUN, "ssg", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_NAILGUN, "ng", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_SUPER_NAILGUN, "sng", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_GRENADE_LAUNCHER, "gl", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_ROCKET_LAUNCHER, "rl", 0, NULL);
  cl_add_cvar(CVAR_NAME_WEAPON_LIGHTNING_GUN, "lg", 0, NULL);
  cl_add_cvar(CVAR_NAME_POWERUP_KEY, "flag", 0, NULL);
  cl_add_cvar(CVAR_NAME_POWERUP_QUAD_DAMAGE, "quad", 0, NULL);
  cl_add_cvar(CVAR_NAME_POWERUP_PENTAGRAM, "666", 0, NULL);
  cl_add_cvar(CVAR_NAME_POWERUP_INVISIBILITY, "eyes", 0, NULL);
  //cl_add_cvar(CVAR_NAME_POWERUP_MEGA_HEALTH, "mh", 0);
  //cl_add_cvar(CVAR_NAME_POWERUP_SUIT, "suit", 0);
  cl_add_cvar(CVAR_NAME_ROCKETS, "rockets", 0, NULL);
  cl_add_cvar(CVAR_NAME_CELLS, "cells", 0, NULL);
  cl_add_cvar(CVAR_NAME_NAILS, "nails", 0, NULL);
  cl_add_cvar(CVAR_NAME_SHELLS, "shells", 0, NULL);
  cl_add_cvar(CVAR_NEED_ARMOUR, "50", 0, NULL);
  cl_add_cvar(CVAR_NEED_HEALTH, "50", 0, NULL);
  cl_add_cvar(CVAR_NEED_WEAPON, "0", 0, NULL);
  cl_add_cvar(CVAR_NEED_ROCKETS, "5", 0, NULL);
  cl_add_cvar(CVAR_NEED_CELLS, "20", 0, NULL);
  cl_add_cvar(CVAR_NEED_NAILS, "40", 0, NULL);
  cl_add_cvar(CVAR_NEED_SHELLS, "10", 0, NULL);
  cl_add_cvar(CVAR_NAME_SOMEPLACE, "someplace", 0, NULL);
  cl_add_cvar(CVAR_NAME_NOTHING, "nothing", 0, NULL);
  cl_add_cvar(CVAR_SEPARATOR, "/", 0, NULL);

  cl_read_cfg();
  //cl_list_cvars();

  InitializeCriticalSection(&client.mutex);

  client.th_input = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cl_input, NULL, 0, &i);
  if (!client.th_input) {
    printf("-- couldn\'t create input thread (%lu)\n", GetLastError());
    return 0;
  }

  if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
    printf("-- WSAStartup() failure\n");
    return 0;
  }

  memset(&(client.s2c.sin_zero), '\0', sizeof(client.s2c.sin_zero));
  client.s2c.sin_family = AF_INET;
  client.s2c.sin_addr.s_addr = htonl(INADDR_ANY);
  client.s2c.sin_port = htons((short)cl_cvar_value(CVAR_LOCAL_PORT));

  memset(&(client.c2q.sin_zero), '\0', sizeof(client.c2q.sin_zero));
  client.c2q.sin_family = AF_INET;
  client.c2q.sin_addr.s_addr = inet_addr("127.0.0.1");
  client.c2q.sin_port = htons(QUAKE_CLIENT_PORT);

  if ((client.sock = (SOCKET)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
    printf("-- invalid socket\n");
    return 0;
  }

  i = 1;
  if (ioctlsocket(client.sock, FIONBIO, &i) != 0) {
    printf("-- ioctlsocket() failed\n");
    return 0;
  }

  if (bind(client.sock, (LPSOCKADDR)&client.s2c, sizeof(struct sockaddr)) == SOCKET_ERROR) {
    closesocket(client.sock);
    printf("-- couldn\'t bind socket\n");
    return 0;
  }

  client.th_client = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cl_client_loop, NULL, 0, &i);
  if (!client.th_client) {
    printf("-- couldn\'t create client thread (%lu)\n", GetLastError());
    return 0;
  }

  client.th_read = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cl_read_mem, NULL, 0, &i);
  if (!client.th_read) {
    printf("-- couldn\'t create read thread (%lu)\n", GetLastError());
    return 0;
  }

  return 1;
}

int cl_read_cfg(void)
{
  FILE *fp;
  char line[MAX_KEY+1+MAX_VALUE+1+1];
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  char *s, *p;
  char c;
  int i, j;
  cvar_t *cvar;
  //void *v;

  fp = fopen(PER_CFG_FILE, "r");
  if (!fp) {
    if (cl_cvar_value(CVAR_MESSAGES) == 2)
      printf("-- couldn\'t load " PER_CFG_FILE " \n");
    return 0;
  }

  j = 0;
  while (1) {
    s = line;
    i = 0;
    while (1) {
      c = fgetc(fp);
      if (c == '\n' || c == EOF) {
        j++;
        break;
      }
      if (i < sizeof(line)) {
        *s++ = c;
        i++;
      }
    }
    *s = '\0';
    if (!line[0])
      goto _skip;
    if (!strncmp(line, "//", 2)) //comment
      goto _skip;
    if (!strchr(line, '=')) {
      printf("-- error in %s on line %d\n", PER_CFG_FILE, j);
      goto _skip;
    }
    s = line;
    p = key;
    while (*s && *s != '=') {
      while (*s && *s == ' ' || *s == '\"' || *s == '\t' || *s == '\r')
        *s++;
      *p++ = *s++;
    }
    *p = '\0';
    *s++;
    p = value;
    while (*s) {
      while (*s && *s == '\"' || *s == '\t' || *s == '\r')
        *s++;
      *p++ = *s++;
    }
    *p = '\0';
    cvar = cl_find_cvar(key);
    if (cvar)
      cl_change_cvar(cvar, value);
    else
      cl_add_cvar(key, value, 1, NULL);
      //printf("\"%s\"=\"%s\"\n", key, value);
    /*v = cl_find_key(key, &i);
    if (!v)
      printf("-- unknown key \"%s\" on line %d\n", key, j);
    else {
      if (i)
        strlcpy((char *)v, value, i);
      else
        *(int *)v = atoi(value);
    }*/
_skip:
    if (c == EOF)
      break;
  }
  fclose(fp);

  return 1;
}

void cl_disconnect(void)
{
  if (client.state != ca_connected && (client.state != (ca_connected + ca_shutdown)))
    return;

  printf("-- disconnected\n");
  cl_server_message(C2S_DISCONNECT, &(unsigned char)client.id, 1);
  client.state = ca_disconnected;
}

int cl_connect(char *addr)
{
  char msg[MAX_NAME+MAX_PASSWORD+6+3+1];
  char *s;
  //char ip[MAX_IP+1];
  //int port;
  int i;
  cvar_t *cvar;
  struct hostent *host;

  if (client.state < ca_disconnected) {
    printf("-- can\'t connect until quake is detected\n");
    return 0;
  }

  if (client.state == ca_connected)
    cl_disconnect();

  if (!*addr)
    addr = cl_cvar_string(CVAR_SERVER);
 
  memset(&(client.c2s.sin_zero), '\0', sizeof(client.c2s.sin_zero));
  client.c2s.sin_family = AF_INET;
  s = strchr(addr, ':');
  if (!s)
    client.c2s.sin_port = htons((short)PSV_SERVER_PORT);
  else {
    client.c2s.sin_port = htons((short)atoi(&addr[s-addr+1]));
    *s = '\0'; 
  }

  if (isip(addr))
    client.c2s.sin_addr.s_addr = inet_addr(addr);
  else {
    EnterCriticalSection(&client.mutex);
    host = gethostbyname(addr);
    LeaveCriticalSection(&client.mutex);
    if (host) {
      memcpy((char *)&client.c2s.sin_addr, (char *)host->h_addr, host->h_length);
      if (cl_cvar_value(CVAR_MESSAGES) == 2)
        printf("-- resolved %s to %s\n", addr, inet_ntoa(client.c2s.sin_addr));
    }
    else {
      printf("-- can\'t resolve %s\n", addr);
      return 0;
    }
  }
  
  _snprintf(msg, sizeof(msg), "%s:%d", inet_ntoa(client.c2s.sin_addr), ntohs(client.c2s.sin_port));
  if ((cvar = cl_find_cvar(CVAR_SERVER)))
    cl_change_cvar(cvar, msg);

  client.state = ca_connecting;
  i = client.realtime;
  while ((client.realtime - i) < 3) {
    printf("-> connecting to %s:%d...\n", inet_ntoa(client.c2s.sin_addr), ntohs(client.c2s.sin_port));
    /*msg[0] = PROTOCOL_VERSION;
    s = cl_cvar_string(CVAR_NAME);
    memcatz(msg, 1, s, bound(0, MAX_NAME, strlen(s)), sizeof(msg));
    s = cl_cvar_string(CVAR_PASSWORD);
    memcatz(msg, strlen(msg) + 1, s, bound(0, MAX_PASSWORD, strlen(s)), sizeof(msg));*/
    _snprintf(msg, sizeof(msg), "%s\\%s\\%s\\", cl_cvar_string(CVAR_NAME), cl_cvar_string(CVAR_PASSWORD), PROTOCOL_VERSION);
    if (!cl_server_message(C2S_CONNECT, msg, strlen(msg)))
      return 0;
    Sleep(1000);
    if (client.state == ca_disconnected || !client.active)
      return 0;
    if (client.state == ca_connected)
      return 1;
  }
  printf("-- couldn\'t connect\n");
  client.state = ca_disconnected;
  return 0;
}

void cl_hello(void)
{
  char msg[32];

  _snprintf(msg, sizeof(msg), "hi %s!\n", inet_ntoa(client.s2c.sin_addr));//, ntohs(client.s2c.sin_port));
  redtext(msg);
  cl_quake_message(msg);
}

void cl_hotkey(void)
{
  if (!client.hotkey)
    return;

  if (!GetAsyncKeyState(client.hotkey))
    client.hotkey_down = 0;
  else if (!client.hotkey_down) {
    cl_request_stat();
    client.hotkey_down = 1;
  }
}

void cl_think(void)
{
  while (client.state < ca_shutdown) {
    Sleep(1000);
    client.realtime++;
  }
  while (!client.input_state) {
    Sleep(100);
  }
}

void cl_timeout(void)
{
  if (client.state != ca_connected)
    return;

  if (client.realtime - client.time > 30) {
    printf("-- server connection timed out\n");
    client.state = ca_disconnected;
  }
}

void cl_ping(void)
{
  if (client.state != ca_connected)
    return;

  if ((client.realtime - client.time > 10) && (client.realtime - client.last_ping > 5)) {
    cl_server_message(C2S_PING, &(unsigned char)client.id, 1);
    client.last_ping = client.realtime;
    if (cl_cvar_value(CVAR_MESSAGES) == 3)
      printf("-> ping\n");
  }
}

void cl_pong(void)
{
  client.time = client.realtime;
  if (cl_cvar_value(CVAR_MESSAGES) == 3)
    printf("<- pong\n");
}

void cl_no_update(void)
{
  if (client.state != ca_connected)
    return;

  cl_server_message(C2S_NO_UPDATE, &(unsigned char)client.id, 1);
  if (cl_cvar_value(CVAR_MESSAGES) == 2)
    printf("-> quake not connected; no update\n");
}


void cl_bad_connect(char *buff)
{
  printf("<- can\'t connect: %s\n", buff);
  client.state = ca_disconnected;
}

void cl_good_connect(char *buff)
{
  client.id = *buff++;
  client.state = ca_connected;
  client.time = client.realtime;
  if (*buff)
    printf("<- connected to %s\n", buff);
  else
    printf("<- connected\n");
}

void cl_print(char *buff)
{
  printf("<- %s\n", buff);
}

void cl_server_quit(void)
{
  printf("<- server shutdown; disconnected\n");
  client.state = ca_disconnected;
}

void cl_show_print(int mode) 
{
  unsigned char print1on = MEM_PRINT1_ON;
  unsigned char print2on = MEM_PRINT2_ON;
  unsigned char print1off = MEM_PRINT1_OFF;
  unsigned char print2off = MEM_PRINT2_OFF;

  if (mode) {
    cl_write_proc(&quake[client.quake_client].print1, &print1on, 1, 1);
    cl_write_proc(&quake[client.quake_client].print2, &print2on, 1, 1);
  }
  else {
    cl_write_proc(&quake[client.quake_client].print1, &print1off, 1, 1);
    cl_write_proc(&quake[client.quake_client].print2, &print2off, 1, 1);
  }
}

int cl_write_proc(unsigned long *memory, char *buff, int size, int num)
{
  int write = -1;
  //int f;

  /*f = */WriteProcessMemory(client.hook, (LPVOID)*memory, buff, size, &write);
  /*if (!f || write != num) {
    if (cl_cvar_value("debug"))
      printf("-- write error\n");
    if (client.auto_shutdown)
      client.state += ca_shutdown;
    else
      cl_disconnect();
    return 0;
  }*/
  return 1;
}

int cl_read_proc(unsigned long *memory, char *buff, int size, int num, int quiet)
{
  int read = -1;
  int f;

  f = ReadProcessMemory(client.hook, (LPCVOID)*memory, buff, size, &read);
  if (!f || read != num) {
    if (quiet)
      return 1;
    if (cl_cvar_value(CVAR_MESSAGES) == 2)
      printf("-- read error\n");
    if (cl_cvar_value(CVAR_AUTO_CLOSE))
      client.state += ca_shutdown;
    else
      cl_disconnect();
    return 0;
  }
  return 1;
}

/*void cl_stat_update(char *buff, int sz)
{
  printf("<- stat update\n");
  memcpy(client.full_stat, buff, sz);
}*/

void cl_init_stat(client_stats_t *stats)
{
  stats->armour = 0;
  stats->health = 0;
  stats->cells = 0;
  stats->rockets = 0;
  stats->nails = 0;
  stats->shells = 0;
  stats->pos[0] = 0;
  stats->pos[1] = 0;
  stats->pos[2] = 0;
  stats->carrying = 0;
  stats->name[0] = '\0';
  stats->macro[0] = '\0';
}

void cl_request_stat(void)
{
  unsigned char buff[2];

  if (client.state != ca_connected)
    return;

  buff[0] = (unsigned char)client.id;
  buff[1] = bound(0, 1, cl_cvar_value(CVAR_SELF));
  cl_server_message(C2S_REQUEST_STAT, buff, 2);
}

int cl_highlight(char *s)
{
  char high[MAX_VALUE+1];
  char *p;
  char *last;

  strlcpy(high, cl_cvar_string(CVAR_HIGHLIGHT), sizeof(high));
  strlcat(high, ",", sizeof(high));
  last = high;
  while ((p = strchr(high, ','))) {
    *p = '\0';
    if (strstr(s, last))
      return 1;
    *p = ' ';
    last = p + 1;
  }
  return 0;
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
void cl_macro(client_stats_t *stats, int type)
{
  stats->macro[0] = '\0';

  switch (type) {
    case MACRO_ARMOUR_TYPE:
      strlcpy(stats->macro, 
        (stats->carrying & IT_ARMOR1) ? cl_cvar_string(CVAR_NAME_GREEN_ARMOUR) : 
        (stats->carrying & IT_ARMOR2) ? cl_cvar_string(CVAR_NAME_YELLOW_ARMOUR) :
        (stats->carrying & IT_ARMOR3) ? cl_cvar_string(CVAR_NAME_RED_ARMOUR) : "",
        //cl_cvar_string(CVAR_NAME_NO_ARMOUR),
        sizeof(stats->macro));
      break;

    case MACRO_ARMOUR:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->armour);
      break;

    case MACRO_HEALTH:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->health);
      break;

    case MACRO_GOLD_ARMOUR:
      _snprintf(stats->macro, sizeof(stats->macro), "%s%s:%d%s",
        (stats->armour < 30) ? "\x90" : "",
        (stats->carrying & IT_ARMOR1) ? cl_cvar_string(CVAR_NAME_GREEN_ARMOUR) : 
        (stats->carrying & IT_ARMOR2) ? cl_cvar_string(CVAR_NAME_YELLOW_ARMOUR) :
        (stats->carrying & IT_ARMOR3) ? cl_cvar_string(CVAR_NAME_RED_ARMOUR) :
        cl_cvar_string(CVAR_NAME_NO_ARMOUR),
        stats->armour,
        (stats->armour < 30) ? "\x91" : "");
      break;

    case MACRO_GOLD_HEALTH:
      _snprintf(stats->macro, sizeof(stats->macro), "%s%d%s",
        (stats->armour < 50) ? "\x90" : "",
        stats->health,
        (stats->armour < 50) ? "\x91" : "");
      break;

    case MACRO_CURRENT_WEAPON:
      switch (stats->weapon) {
        case 0:
	      case IT_AXE: strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_AXE), sizeof(stats->macro)); break;
	      case IT_SHOTGUN: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SHOTGUN), stats->shells); break;
	      case IT_SUPER_SHOTGUN: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SUPER_SHOTGUN), stats->shells); break;
	      case IT_NAILGUN: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_NAILGUN), stats->nails); break;
	      case IT_SUPER_NAILGUN: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SUPER_NAILGUN), stats->nails); break;
	      case IT_GRENADE_LAUNCHER: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_GRENADE_LAUNCHER), stats->rockets); break;
	      case IT_ROCKET_LAUNCHER: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_ROCKET_LAUNCHER), stats->rockets); break;
	      case IT_LIGHTNING: _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_LIGHTNING_GUN), stats->cells); break;
        default: strlcpy(stats->macro, "unknown", sizeof(stats->macro)); break;
      }
      break;

    case MACRO_BEST_WEAPON:
      if ((stats->carrying & IT_LIGHTNING) && (stats->cells > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_LIGHTNING_GUN), sizeof(stats->macro));
      else if ((stats->carrying & IT_ROCKET_LAUNCHER) && (stats->rockets > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_ROCKET_LAUNCHER), sizeof(stats->macro));
      else if ((stats->carrying & IT_GRENADE_LAUNCHER) && (stats->rockets))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_GRENADE_LAUNCHER), sizeof(stats->macro));
      else if ((stats->carrying & IT_SUPER_NAILGUN) && (stats->nails > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_SUPER_NAILGUN), sizeof(stats->macro));
      else if ((stats->carrying & IT_NAILGUN) && (stats->nails > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_NAILGUN), sizeof(stats->macro));
      else if ((stats->carrying & IT_SUPER_SHOTGUN) && (stats->shells > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_SUPER_SHOTGUN), sizeof(stats->macro));
      else if ((stats->carrying & IT_SHOTGUN) && (stats->shells > 0))
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_SHOTGUN), sizeof(stats->macro));
      else
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_AXE), sizeof(stats->macro));
      break;

    case MACRO_BEST_WEAPON_AMMO:
      if ((stats->carrying & IT_LIGHTNING) && (stats->cells > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_LIGHTNING_GUN), stats->cells);
      else if ((stats->carrying & IT_ROCKET_LAUNCHER) && (stats->rockets > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_ROCKET_LAUNCHER), stats->rockets);
      else if ((stats->carrying & IT_GRENADE_LAUNCHER) && (stats->rockets))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_GRENADE_LAUNCHER), stats->rockets);
      else if ((stats->carrying & IT_SUPER_NAILGUN) && (stats->nails > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SUPER_NAILGUN), stats->nails);
      else if ((stats->carrying & IT_NAILGUN) && (stats->nails > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_NAILGUN), stats->nails);
      else if ((stats->carrying & IT_SUPER_SHOTGUN) && (stats->shells > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SUPER_SHOTGUN), stats->shells);
      else if ((stats->carrying & IT_SHOTGUN) && (stats->shells > 0))
        _snprintf(stats->macro, sizeof(stats->macro), "%s:%d", cl_cvar_string(CVAR_NAME_WEAPON_SHOTGUN), stats->shells);
      else
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON_AXE), sizeof(stats->macro));
      break;

    case MACRO_SHELLS:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->shells);
      break;

    case MACRO_NAILS:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->nails);
      break;

    case MACRO_ROCKETS:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->rockets);
      break;

    case MACRO_CELLS:
      _snprintf(stats->macro, sizeof(stats->macro), "%d", stats->cells);
      break;

    case MACRO_POWERUPS:
    case MACRO_GOLD_POWERUPS:
      _snprintf(stats->macro, sizeof(stats->macro), "%s", (type == MACRO_GOLD_POWERUPS) ? " \x90" : " ");
      if (stats->carrying & (IT_KEY1|IT_KEY2)) {
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_KEY), sizeof(stats->macro));
      }
      if (stats->carrying & IT_QUAD) {
        if (stats->macro[1] && strcmp(stats->macro, " \x90"))
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_QUAD_DAMAGE), sizeof(stats->macro));
      }
      if (stats->carrying & IT_INVULNERABILITY) {
        if (stats->macro[1] && strcmp(stats->macro, " \x90"))
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_PENTAGRAM), sizeof(stats->macro));
      }
      if (stats->carrying & IT_INVISIBILITY) {
        if (stats->macro[1] && strcmp(stats->macro, " \x90"))
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_INVISIBILITY), sizeof(stats->macro));
      }
      /*if (stats->carrying & IT_SUPERHEALTH) {
        if (stats->macro[1] && strcmp(stats->macro, " \x90"))
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_MEGA_HEALTH), sizeof(stats->macro));
      }*/
      /*if (stats->carrying & IT_SUIT) {
        if (stats->macro[1] && strcmp(stats->macro, " \x90"))
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_POWERUP_SUIT), sizeof(stats->macro));
      }*/
      if (type == MACRO_GOLD_POWERUPS) {
        strlcat(stats->macro, "\x91", sizeof(stats->macro));
        if (!strcmp(stats->macro, " \x90\x91"))
          stats->macro[0] = '\0';
      }
      else {
        if (!stats->macro[1])
          stats->macro[0] = '\0';
      }
      break;

    case MACRO_LED:
      cl_location(stats->pos, stats->macro, sizeof(stats->macro));
      strlcpy(stats->macro,
        (stats->carrying & (IT_KEY1|IT_KEY2|IT_QUAD|IT_INVULNERABILITY|IT_INVISIBILITY|IT_SUPERHEALTH/*|IT_SUIT*/))
        ? "\x87" : cl_highlight(stats->macro) ? "\x88" : "\x89", sizeof(stats->macro)); //red, yellow blue
      break;

    case MACRO_DISTANCE:
      _snprintf(stats->macro, sizeof(stats->macro), "%0.0f", distance(stats->pos, client.pos));
      break;

    case MACRO_NEED:
      if ((stats->carrying & IT_ARMOR1) && stats->armour < cl_cvar_value(CVAR_NEED_ARMOUR) ||
          (stats->carrying & IT_ARMOR2) && stats->armour < cl_cvar_value(CVAR_NEED_ARMOUR) ||
          (stats->carrying & IT_ARMOR3) && stats->armour < cl_cvar_value(CVAR_NEED_ARMOUR) ||
          (!(stats->carrying & (IT_ARMOR1|IT_ARMOR2|IT_ARMOR3))))
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_ARMOUR), sizeof(stats->macro));
      
      if (stats->health < cl_cvar_value(CVAR_NEED_HEALTH)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_HEALTH), sizeof(stats->macro));
      }
      
      if (!(stats->carrying & (IT_SUPER_NAILGUN|IT_GRENADE_LAUNCHER|IT_ROCKET_LAUNCHER|IT_LIGHTNING)) &&
          cl_cvar_value(CVAR_NEED_WEAPON)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));      
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_WEAPON), sizeof(stats->macro));
      }

      if (stats->rockets < cl_cvar_value(CVAR_NEED_ROCKETS)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_ROCKETS), sizeof(stats->macro));
      }
      
      if (stats->cells < cl_cvar_value(CVAR_NEED_CELLS)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_CELLS), sizeof(stats->macro));
      }
      
      if (stats->nails < cl_cvar_value(CVAR_NEED_NAILS)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_NAILS), sizeof(stats->macro));
      }
      
      if (stats->shells < cl_cvar_value(CVAR_NEED_SHELLS)) {
        if (stats->macro[0])
          strlcat(stats->macro, cl_cvar_string(CVAR_SEPARATOR), sizeof(stats->macro));
        strlcat(stats->macro, cl_cvar_string(CVAR_NAME_SHELLS), sizeof(stats->macro));
      }
      
      if (!stats->macro[0])
        strlcpy(stats->macro, cl_cvar_string(CVAR_NAME_NOTHING), sizeof(stats->macro));
      break;

    case MACRO_LOCATION:
      cl_location(stats->pos, stats->macro, sizeof(stats->macro));
      break;

    case MACRO_NAME:
      strlcpy(stats->macro, stats->name, sizeof(stats->macro));
      break;

    default: break;
  }
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
void cl_parse_chars(char *s, int red, int len)
{
  char buff[MAX_LINE*MAX_CLIENTS+1];
  char name[MAX_KEY+1];
  char *p, *q, *sv;
  unsigned char c;
  int i, j, b;
  cvar_t *cvar;

  sv = s;

  //$cvar
  p = buff;
  for (j = 0; *s && j < len; j++) {
    if (*s == '$') {
      *s++;
      i = 0;
      q = name;
      cvar = NULL;
      //while (*s && (isalpha(*s) && isdigit(*s) || *s == '_' || *s == '-')) {
      while (*s && (*s > 32) && *s != '$') {
        if (i < sizeof(name)) {
          *q++ = *s++;
          i++;
        }
        *q = '\0';
        cvar = cl_find_cvar(name);
        if (cvar)
          break;
      }
      *q = '\0';
      if (!cvar) {
        *p++ = '$';
        q = name;
        for(; *q && j < sizeof(buff); j++)
          *p++ = *q++;
        continue;
      }
      i = 0;
      q = cvar->string;
      for(; *q && j < sizeof(buff); j++)
        *p++ = *q++;
    }
    else {
      *p++ = *s++;
    }
  }
  *p = '\0';

  //extra chars and red text
  s = sv;
  p = buff;
  for (j = 0; *p && j < len; j++)
    *s++ = *p++;
  *s = '\0';

  s = sv;
  p = buff;
  for (j = 0, b = 0; *s && j < len; j++) {
    if (*s == '{' || *s == '}') {
      b = !b;
      *s++;
      continue;
    }
		if (*s == '$' && s[1] == 'x') {
			// check for $x10, $x8a, etc
      if ((i = hex2dec(s[2], s[3])) == -1)
        goto _skip;
			if (!i)
				i = ' ';
			*p++ = i;
			s += 4;
			continue;
		}
		if (*s == '$' && s[1]) {
			c = 0;
			switch (s[1]) {
        case ':': c = '\x0A'; break;
        case '[': c = '\x10'; break;
        case ']': c = '\x11'; break;
        case 'G': c = '\x86'; break;
        case 'R': c = '\x87'; break;
        case 'Y': c = '\x88'; break;
        case 'B': c = '\x89'; break;
        case '(': c = '\x80'; break;
        case '=': c = '\x81'; break;
        case ')': c = '\x82'; break;
        case 'a': c = '\x83'; break;
        case '<': c = '\x1D'; break;
        case '-': c = '\x1E'; break;
        case '>': c = '\x1F'; break;
        case ',': c = '\x1C'; break;
        case '.': c = '\x9C'; break;
        case 'b': c = '\x8B'; break;
        case '\\': c = '\x0D'; break;
        case 'c':
        case 'd': c = '\x8D'; break;
      }
			if (s[1] >= '0' && s[1] <= '9')
				c = s[1] - '0' + 0x12;
			if (c) {
				*p++ = c;
				s += 2;
				continue;
			}
		}
_skip:
    *p++ = (red && !b && (*s > 31 && *s < 128)) ? 128 + *s++ : *s++;
	}
	*p = '\0';

  s = sv;
  p = buff;
  for (j = 0; *p && j < len; j++)
    *s++ = *p++;
  *s = '\0';
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
void cl_stat_print(char *buff, int sz)
{
  client_stats_t stats;
  char *s, *p;
  unsigned char *q;
  char byte[MAX_BYTE_SIZE];
  char all[MAX_LINE*MAX_CLIENTS+1];
  int i, j, names;
  int align;

  s = all;
  align = cl_cvar_value(CVAR_ALIGN);
  names = *buff++;
  if (names < 0 || names > MAX_NAME)
    names = MAX_NAME;

  while (1) {
    cl_init_stat(&stats);
    i = 0;
    p = stats.name;
    j = strlen(buff);
    while (i < names) {
      if (align == 1) //left
        *p++ = (i > j - 1) ? ' ' : *buff++;
      else if (align == 2) //right
        *p++ = (i < names - j) ? ' ' : *buff++;
      else
        *p++ = *buff++;
      i++;
    }
    *p = '\0';
    *buff++;

    //int
    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++)
        byte[j] = *buff++;
      switch (i) {
        case 0: stats.pos[0] = readfloat(byte); break;
        case 1: stats.pos[1] = readfloat(byte); break;
        case 2: stats.pos[2] = readfloat(byte); break;
        case 3: stats.carrying = readint(byte); break;
      }
    }

    //unsigned char
    for (i = 0; i < 7; i++) {
      *byte = *buff++;
      switch (i) {
        case 0: stats.armour = (unsigned char)*byte; break;
        case 1: stats.health = (unsigned char)*byte; break;
        case 2: stats.weapon = (unsigned char)*byte; break;
        case 3: stats.shells = (unsigned char)*byte; break;
        case 4: stats.nails = (unsigned char)*byte; break;
        case 5: stats.rockets = (unsigned char)*byte; break;
        case 6: stats.cells = (unsigned char)*byte; break;
      }  
    }
    
    i = 0;
    //b = 0;
    p = cl_cvar_string(CVAR_REPORT);
    while (*p) {
      /*if (*p == '{' || *p == '}') {
        b = !b;
        *p++;
      }*/
      //$[{%N}$] %e%e%e @ %l%p%P %d^m cur:%w^ best:%b^ bestammo:%B^ cells:%c^ nail:%n^ rckt:%r^ shells:%s^ need:%u^ me:%A%a^/%h^ %i^ %o^ $x0A
      if (*p == '%' && p[1]) {
        switch (p[1]) {
          case 'N': cl_macro(&stats, MACRO_NAME); break;
          case 'e': cl_macro(&stats, MACRO_LED); break;
          case 'l': cl_macro(&stats, MACRO_LOCATION); break;
          case 'p': cl_macro(&stats, MACRO_POWERUPS); break;
          case 'P': cl_macro(&stats, MACRO_GOLD_POWERUPS); break;
          //case 'd': cl_macro(&stats, MACRO_DISTANCE); break;
          case 'w': cl_macro(&stats, MACRO_CURRENT_WEAPON); break;
          case 'b': cl_macro(&stats, MACRO_BEST_WEAPON); break;
          case 'B': cl_macro(&stats, MACRO_BEST_WEAPON_AMMO); break;
          case 'c': cl_macro(&stats, MACRO_CELLS); break;
          case 'n': cl_macro(&stats, MACRO_NAILS); break;
          case 'r': cl_macro(&stats, MACRO_ROCKETS); break;
          case 's': cl_macro(&stats, MACRO_SHELLS); break;
          case 'u': cl_macro(&stats, MACRO_NEED); break;
          case 'A': cl_macro(&stats, MACRO_ARMOUR_TYPE); i += 3 - strlen(stats.macro); break;
          case 'a': cl_macro(&stats, MACRO_ARMOUR); i += 3 - strlen(stats.macro); break;
          case 'h': cl_macro(&stats, MACRO_HEALTH); i += 3 - strlen(stats.macro); break;
          case 'i': cl_macro(&stats, MACRO_GOLD_ARMOUR); i += 3 - strlen(stats.macro); break;
          case 'o': cl_macro(&stats, MACRO_GOLD_HEALTH); i += 3 - strlen(stats.macro); break;          
          case 't': while (i > 0) {
                      *s++ = ' ';
                      i--;
                    }
                    break;
          default: *s++ = *p++;
                   continue;
                   break;
        }
        q = stats.macro;

        while (q && *q) {
          if (p[2] == '^' && isdigit(*q))
            *q += 98; //gold
          //else if (!b && (*q > 31 && *q < 128))
          //  *q += 128; //red
          *s++ = *q++;
        }
        p += (p[2] == '^') ? 3 : 2;
      }
      else {
        *s++ = /*(!b) ? 128 + *p++ :*/ *p++;
      }
    }
    *s++ = '\n';
    *s = '\0';

    *buff++;
    if (!*buff)
      break;
  }
  
  cl_parse_chars(all, TRUE, sizeof(all));
  cl_quake_message(all);
}

int cl_server_message(char tag, char *msg, int len)
{
  char buff[MAX_SINGLE_STAT+2+1];
  int sz;

  buff[0] = A2A_PER_ID;
  buff[1] = tag;
  if (msg)
    memcatz(buff, 2, msg, len, sizeof(buff));

  if ((sz = sendto(client.sock, buff, len + 2, 0, (LPSOCKADDR)&client.c2s, sizeof(SOCKADDR))) == -1) {
    printf("-- server sendto() error\n");
    return 0;
  }
  client.bytes_out += sz;

  return 1;
}

void cl_quake_message(char *msg)
{
  char buff[MAX_LINE*MAX_CLIENTS+5+1];

  _snprintf(buff, sizeof(buff), "%c%c%c%c%c%s",
    A2A_QUAKE_ID,
    A2A_QUAKE_ID,
    A2A_QUAKE_ID,
    A2A_QUAKE_ID,
    'n',
    msg);
  cl_show_print(FALSE);
  sendto(client.sock, buff, strlen(buff), 0, (LPSOCKADDR)&client.c2q, sizeof(SOCKADDR));
  Sleep(50);
  cl_show_print(TRUE);
}

void cl_shutdown(void)
{
  //if (client.quake_exit == STILL_ACTIVE)
  //  cl_show_print(TRUE);
  if (client.state == (ca_connected + ca_shutdown))
    cl_disconnect();
  printf("-- shutting down\n");
  client.active = 0;
  client.state = ca_free;
  cl_remove_cvars();
  cl_clear_loc();
  TerminateThread(client.th_input, 0); //th_input blocks
  WaitForSingleObject(client.th_read, INFINITE);
  WaitForSingleObject(client.th_client, INFINITE);
  closesocket(client.sock);
  if (client.hook)
    CloseHandle(client.hook);
  CloseHandle(client.th_input);
  CloseHandle(client.th_read);
  CloseHandle(client.th_client);
  DeleteCriticalSection(&client.mutex);
  WSACleanup();
}

void cl_print_net(void)
{
  printf("-- %d bytes in, %d bytes out in %d seconds\n", client.bytes_in, client.bytes_out, client.realtime);
}

int cl_strip_ammo(int items)
{
  int it;

  it = items;
  if (items & IT_SHELLS)
    it -= IT_SHELLS;
  if (items & IT_NAILS)
    it -= IT_NAILS;
  if (items & IT_ROCKETS)
    it -= IT_ROCKETS;
  if (items & IT_CELLS)
    it -= IT_CELLS;

  return it;
}

int cl_new_cvar(cvar_t *cvar)
{
  cvar = (cvar_t *)malloc(sizeof(cvar_t));
  if (!cvar)
    return 0;

  cvar->name[0] = '\0';
  cvar->string[0] = '\0';
  cvar->value = 0;
  cvar->type = 1;
  cvar->function = NULL;

  cvar->next = client.cvars;
  client.cvars = cvar;

  return 1;
}

cvar_t *cl_find_cvar(char *name)
{
  cvar_t *cvar;

  for (cvar = client.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar;
  }
  return NULL;
}

void cl_list_cvars(void)
{
  cvar_t *cvar;
  int i;
 
  for (i = 0, cvar = client.cvars; cvar; cvar = cvar->next, i++) {
    printf("-- %c %s = \"%s\"", (!cvar->type) ? '*' : ' ', cvar->name, cvar->string);
    if (cl_cvar_value(CVAR_MESSAGES) == 2)
      printf(" (%d)", cvar->value);
    printf("\n");
  }
  printf("-- %d cvar%s (*standard cvars)\n", i, (i != 1) ? "s" : "");
}

char *cl_cvar_string(char *name)
{
  cvar_t *cvar;

  for (cvar = client.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar->string;
  }
  return NULL;
}

int cl_cvar_value(char *name)
{
  cvar_t *cvar;

  for (cvar = client.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar->value;
  }
  return 0;
}

void cl_change_cvar(cvar_t *cvar, char *value)
{
  strlcpy(cvar->string, value, MAX_VALUE+1);
  cvar->value = atoi(value);
  if (cvar->function)
    cvar->function();
}

void cl_add_cvar(char *name, char *value, int type, func_t function)
{
  cvar_t *cvar;

  if (cl_find_cvar(name)) {
    printf("-- cvar \"%s\" already exists\n", name);
    return;
  }

  if (!cl_new_cvar(client.cvars)) {
    printf("-- failed to create cvar %s\n", name);
    return;
  }
  cvar = client.cvars;
  strlcpy(cvar->name, name, sizeof(cvar->name));
  cvar->function = (function) ? function : NULL;
  cvar->type = type;
  cl_change_cvar(cvar, &*value);
}

void cl_add_user_cvar(char *s)
{
  char key[MAX_KEY+1];
  char value[MAX_KEY+1];

  if (!*s) {
    printf("-- empty cvar name\n");
    return;
  }
  
  tokenize(s, key, sizeof(key), value, sizeof(value), ' ');
  cl_add_cvar(key, value, 1, NULL);
}

void cl_remove_cvars(void)
{
  cvar_t *cvar;

  while (client.cvars) {
    cvar = client.cvars;
    client.cvars = client.cvars->next;
    free(cvar);
  }
}

void cl_key_list(void)
{
  int i;

  for (i = 0; i < MAX_KEY_CODES; i++)
    printf("-- * %s (%s - %x)\n", special_keys[i].name, special_keys[i].desc, special_keys[i].code);
  printf("-- %d special keys\n", MAX_KEY_CODES);
}

void cl_hotkey_change(void)
{
  char *key;
  int i;

  key = cl_cvar_string(CVAR_HOTKEY);

  if (!strcmp(key, "none")) {
    if (cl_cvar_value(CVAR_MESSAGES) == 2)
      printf("-- hotkey disabled\n");
    client.hotkey = 0;
    return;
  }

  for (i = 0; i < MAX_KEY_CODES; i++) {
    if (!strcmp(key, special_keys[i].name)) {
      client.hotkey = special_keys[i].code;
      break;
    }
  }
  if (i == MAX_KEY_CODES) {
    if (strlen(key) == 1) //all single keys
      client.hotkey = VkKeyScanEx(*key, client.keyboard);
    else if (!strncmp(key, "\\x", 2)) // \x custom
      client.hotkey = bound(0, 0xFF, hex2dec(key[2], key[3]));
    //else if (!strncmp(key, "f", 1)) //f1 - f24 (..there's a f24??!)
    //  client.hotkey = bound(VK_F1, VK_F24, VK_F1 + atoi(key + 1) - 1);
    //else if (!strncmp(key, "numpad", 6)) //numpad
    //  client.hotkey = bound(VK_NUMPAD0, VK_NUMPAD9, VK_NUMPAD0 + atoi(key + 6));
    else {
      printf("-- unknown key %s, use keylist for listing\n", key);
      return;
    }
  }
    
  if (cl_cvar_value(CVAR_MESSAGES) == 2)
    printf("-- hotkey virtual key code is now: %d dec, %x hex\n", client.hotkey, client.hotkey);
  GetAsyncKeyState(client.hotkey);
}

void cl_name_change(void)
{
  char *value;

  value = cl_cvar_string(CVAR_NAME);
  value[MAX_NAME] = '\0';
}

void cl_password_change(void)
{
  char *value;

  value = cl_cvar_string(CVAR_PASSWORD);
  value[MAX_PASSWORD] = '\0';
}

void cl_check_map(char *s)
{
  int i;
  char map[MAX_FILE_PATH+1];
  char *p;

  if (!*s)
    return;

  i = 0;
  p = map;
  while (*s && (i < MAX_FILE_PATH)) {
    if (*s < 32 || *s > 126)
      return;
    if (*s == '.') {
      break;
    }
    *p++ = *s++;
  }
  *p = 0;
  if (strcmp(client.last_map, map)) {
    printf("-- map: %s \n", map);
    strlcpy(client.last_map, map, sizeof(client.last_map));
    cl_load_loc(map);
  }
}

int cl_new_node(loc_data_t *node)
{
  node = (loc_data_t *)malloc(sizeof(loc_data_t));
  if (!node)
    return 0;

  node->coord[0] = 0.0;
  node->coord[1] = 0.0;
  node->coord[2] = 0.0;
  node->name[0] = '\0';
  node->next = client.nodes;
  client.nodes = node;

  return 1;
}

void cl_clear_loc(void)
{
  loc_data_t *node;

  while (client.nodes) {
    node = client.nodes;
    client.nodes = node->next;
    free(node);
  }
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
int cl_load_loc(char *loc)
{
  FILE *fp;
	char *s;
  char *dir;
  char c;
  char map[MAX_FILE_PATH+1];
  char location[MAX_LOC_NAME+1];
  char locline[MAX_LOC_LINE+1];
	int i, n, sign, line, nameindex, overflow, loc_numentries;
	vec3_t coord;

  if (!*loc && !*client.last_map) {
    printf("-- cannot load loc, unknown map\n"); 
    return 0;
  }
  dir = cl_cvar_string(CVAR_LOC_DIR);
  _snprintf(map, sizeof(map), "%s\\%s.loc", (*dir) ? dir : ".", (*loc) ? loc : client.last_map);
  fp = fopen(map, "r");
  if (!fp) {
    printf("-- can\'t open loc: %s\n", map);
    return 0;
  }
	
  cl_clear_loc();
  loc_numentries = 0;
	line = 1;
	while (1) {
    s = locline;
    c = ' ';
    while (1) {
      c = fgetc(fp);
      if (c == '\n')
        break;
      if (c == EOF)
        goto _endoffile;
      *s++ = c;
    }
    *s = 0;
    s = locline;
		SKIPBLANKS(s);
		if (!*s) {
			goto _endoffile;
		} else if (*s == '\n') {
			s++;
			goto _endofline;
		} else if (*s == '/' && s[1] == '/') {
			SKIPTOEOL(s);
			goto _endofline;
		}
		for (i = 0; i < 3; i++)	{
			n = 0;
			sign = 1;
			while (1) {
				switch (*s++) {
				case ' ': case '\t':
					goto _next;
				case '-':
					if (n) {
						printf("-- loc error on line %d: unexpected '-'\n", line);
						SKIPTOEOL(s);
						goto _endofline;
					}
					sign = -1;
					break;
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					n = n * 10 + (s[-1] - '0');
					break;
				default:	// including eol or eof
					printf("-- loc error on line %d: couldn't parse coords\n", line);
					SKIPTOEOL(s);
					goto _endofline;
				}
			}
_next:
			n *= sign;
			coord[i] = (float)(n / 8.0);
			SKIPBLANKS(s);
		}

		overflow = 0;
    nameindex = 0;
		while (1) {
			switch (*s)	{
			case '\r':
				s++;
				break;
			case '\n':
			case '\0':
				location[nameindex] = 0;
        cl_new_node(client.nodes);
        client.nodes->coord[0] = coord[0];
        client.nodes->coord[1] = coord[1];
        client.nodes->coord[2] = coord[2];
        strcpy(client.nodes->name, location);
        loc_numentries++;
				if (*s == '\n')
					s++;
				goto _endofline;
			default:
				if (nameindex < MAX_LOC_NAME - 1) {
					location[nameindex++] = *s;
				} else if (!overflow) {
					overflow = 1;
					printf("-- loc warning (line %d): truncating loc name to %d chars\n", line, MAX_LOC_NAME - 1);
				}
				s++;
			}
		}
_endofline:
		line++;
	}
_endoffile:

	if (loc_numentries)
  	printf("-- loaded %d loc points\n", loc_numentries);
  else {
		cl_clear_loc();
		printf("-- empty loc file\n");
	}
  fclose(fp);

	return 1;
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
int cl_location(vec3_t loc, char *buff, int len)
{
  char *s;
	float dist, mindist;
	vec3_t vec;
	loc_data_t *node, *best;
  int i, found;

  found = 0;
  if (!client.nodes)
    s = cl_cvar_string(CVAR_NAME_SOMEPLACE);
  else {
  	best = NULL;
	  for (node = client.nodes; node; node = node->next) {
		  VectorSubtract(loc, node->coord, vec);
		  dist = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
		  if (!best || dist < mindist) {
  			best = node;
	  		mindist = dist;
      }
		}
    s = best->name;
    found = 1;
	}

  i = 0;
  for (i = 0; *s && i < len; i++)
    *buff++ = *s++;
  *buff = 0;

  return found;
}

