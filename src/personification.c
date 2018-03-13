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

sv_client_t clients[MAX_CLIENTS];
sv_server_t server;

int main(int argc, char *argv[])
{
  if (!sv_init(argc, argv))
    return 0;
  sv_think();
  sv_shutdown();
  return 1;
}

void *sv_server_loop(void) 
{
  char buff[MAX_SINGLE_STAT+1];
  int sz;

  while (server.active) {
    sv_zombies();
    _sleep(10);
#ifdef WIN32        
    sz = recvfrom(server.sock, buff, sizeof(buff), 0, (LPSOCKADDR)&server.addr, &server.hsz);
#endif
#ifdef UNIX
    sz = recvfrom(server.sock, buff, sizeof(buff), MSG_DONTWAIT, (struct sockaddr *)&server.addr, &server.hsz);
#endif
    if (sz == -1)
      continue;
    server.bytes_in += sz;
    if (sz < sizeof(buff)) {
      buff[sz] = '\0';
    } else {
      buff[sizeof(buff)-sizeof(char)] = '\0';
    }
    if (buff[0] != A2A_PER_ID) {
      if (sv_cvar_value(CVAR_MESSAGES) == 2)
        printf("-- bad packet from %s:%d: %s\n", inet_ntoa(server.addr.sin_addr), ntohs(server.addr.sin_port), buff);
      continue;
    }
    switch (buff[1]) {
      case C2S_CONNECT: sv_connect(buff + 2); break;
      case C2S_DISCONNECT: sv_disconnect(buff + 2); break;
      case C2S_UPDATE_STAT: sv_update_stat(buff + 2, sz - 2); break;
      case C2S_REQUEST_STAT: sv_request_stat(buff + 2); break;
      case C2S_NO_UPDATE: sv_no_update(buff + 2); break;
      case C2S_PING: sv_ping(buff + 2); break;
      default:
        if (sv_cvar_value(CVAR_MESSAGES))
          printf("-- bad packet: unknown c2s %c\n", buff[1]);
        break;
    }
  }
  _endthread;
}

int sv_init(int argc, char *argv[])
{
#ifdef WIN32
  WSADATA wsaData;
#endif
  unsigned long i;
  char s[32];

  printf("%s v%s - Copyright %s %s (%s)\n\n", PSV_HEADER, PSV_VERSION, COPYRIGHT, AUTHOR, WEB);

  server.input_state = 1;
  server.realtime = 0;
  server.bytes_in = 0;
  server.bytes_out = 0;
#ifdef WIN32
  server.hsz = sizeof(SOCKADDR);
#endif
#ifdef UNIX
  server.hsz = sizeof(struct sockaddr_in);
#endif

  _strnprintf(s, sizeof(s), "%d", PSV_SERVER_PORT);
  sv_add_cvar(CVAR_LOCAL_PORT, s, 0, NULL);
  sv_add_cvar(CVAR_HOSTNAME, PSV_HEADER " " PSV_VERSION " Server", 0, NULL);
  _strnprintf(s, sizeof(s), "%d", MAX_CLIENTS);
  sv_add_cvar(CVAR_MAX_CLIENTS, s, 0, sv_max_clients_change);
  sv_add_cvar(CVAR_PASSWORD, "none", 0, sv_password_change);
  sv_add_cvar(CVAR_MESSAGES, "1", 0, NULL);

  sv_read_cfg();

#ifdef WIN32
  if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
    printf("WSAStartup() failed\n");
    return 0;
  }
#endif

  memset(&(server.addr.sin_zero), '\0', sizeof(server.addr.sin_zero));
  server.addr.sin_family = AF_INET;
  server.addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server.addr.sin_port = htons((short)sv_cvar_value(CVAR_LOCAL_PORT));

  for (i = 0; i < MAX_CLIENTS; i++) {
    memset(&(clients[i].addr.sin_zero), '\0', sizeof(clients[i].addr.sin_zero));
    clients[i].addr.sin_family = AF_INET;
    clients[i].state = ca_free;
    clients[i].time = 0;
    clients[i].update = 0;
  }

#ifdef WIN32
  if ((server.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
#endif
#ifdef UNIX
  if ((server.sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
#endif
    printf("-- invalid socket\n");
    return 0;
  }

#ifdef WIN32
  i = 1;
  if (ioctlsocket(server.sock, FIONBIO, &i) != 0) {
    printf("-- ioctlsocket() failed\n");
    return 0;
  }
#endif

#ifdef WIN32
  if (bind(server.sock, (LPSOCKADDR)&server.addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
#endif
#ifdef UNIX
  if (bind(server.sock, (struct sockaddr *)&server.addr, sizeof(struct sockaddr)) == -1) {
#endif
    _close(server.sock);
    printf("-- couldn\'t bind socket\n");
    return 0;
  }

  server.active = 1;
#ifdef WIN32
  server.th_input = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sv_input, NULL, 0, &i);
  if (!server.th_input) {
#endif
#ifdef UNIX
  if (pthread_create(&server.th_input, NULL, (void *)sv_input, NULL)) {
#endif
    printf("-- couldn\'t create input\n");
    return 0;
  }

#ifdef WIN32
  server.th_main = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sv_server_loop, NULL, 0, &i);
  if (!server.th_main) {
#endif
#ifdef UNIX
  if (pthread_create(&server.th_main, NULL, (void *)sv_server_loop, NULL)) {
#endif
    printf("-- couldn\'t create main loop\n");
    return 0;
  }

  printf("-- started on port %d\n", sv_cvar_value(CVAR_LOCAL_PORT));
  return 1;
}

void sv_think(void)
{
  while (server.state < sa_shutdown) {
    _sleep(1000);
    server.realtime++;
  }
  while (!server.input_state) {
    _sleep(100);
  }
}

void sv_shutdown(void)
{
  printf("-- shutting down\n");
  sv_client_message(S2C_SHUTDOWN, -1, NULL, 0);
  server.active = 0;
  server.state = sa_free;
  sv_remove_cvars();
#ifdef WIN32
  TerminateThread(server.th_input, 0);
  WaitForSingleObject(server.th_input, INFINITE);
  WaitForSingleObject(server.th_main, INFINITE);
  CloseHandle(server.th_main);
  CloseHandle(server.th_input);
#endif
#ifdef UNIX
  pthread_cancel(server.th_input);
  pthread_join(server.th_input, NULL);
  pthread_join(server.th_main, NULL);
#endif
  _close(server.sock);
#ifdef WIN32
  WSACleanup();
#endif
}

void *sv_input(void)
{
  char line[MAX_KEY+1+MAX_VALUE+1+1];
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  char *s;
  char c;
  int i;
  cvar_t *cvar;
#ifdef UNIX
  int th_s;
  int th_t;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &th_s);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &th_t);
#endif

  while (server.active) {
    server.input_state = 1;
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
    server.input_state = 0;
    tokenize(line, key, sizeof(key), value, sizeof(value), ' ');
    cvar = sv_find_cvar(key);
    if (cvar) {
      if (*value)
        sv_change_cvar(cvar, value);
      else
        printf("-- \"%s\" is \"%s\"\n", cvar->name, cvar->string);
    }
    else if (!strcmp(key, "quit") || !strcmp(key, "q"))
      server.state = sa_shutdown;
    else if (!strcmp(key, "net") || !strcmp(key, "n"))
      sv_print_net();
    else if (!strcmp(key, "cvarlist") || !strcmp(key, "v"))
      sv_list_cvars();
    else if (!strcmp(key, "status") || !strcmp(key, "s"))
      sv_status();
    //else if (!strcmp(key, "add") || !strcmp(key, "a"))
    //  sv_add_user_cvar(value);
    else
      printf("-- unknown command \"%s\"\n", key);
  }
  return 0;
}

int sv_find_slot(int id)
{
  int i;

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state != ca_connected)
      continue;
    if (clients[i].id == id)
      return i;
  }
  return -1;
}

//connect NAME.PASSWORD.VERSION
void sv_connect(char *buff)
{
  int i, j, sz, slot;
  char *s, *p;
  char msg[32+MAX_NAME+1];
  char pw[MAX_PASSWORD+1];
  char ver[MAX_VERSION+1];

  if (sv_cvar_value(CVAR_MESSAGES))
    printf("<- connect from %s:%d\n", inet_ntoa(server.addr.sin_addr), ntohs(server.addr.sin_port));
  j = 0;
  slot = -1;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state == ca_connected) {
      j++;
    }
    if (clients[i].state == ca_connected && sv_compare_addr(&server.addr, &clients[i].addr)) {
      //sv_outside_message(S2C_BAD_CONNECT, "dup connect");
      if (sv_cvar_value(CVAR_MESSAGES) == 2)
        printf("-- dup connect: ignored\n");
      return;
    }
    if (clients[i].state == ca_free) {
      slot = i;
      break;
    }
  }
  if (slot == -1 || (j >= sv_cvar_value(CVAR_MAX_CLIENTS))) {
    sv_outside_message(S2C_BAD_CONNECT, "server full");
    if (sv_cvar_value(CVAR_MESSAGES))
      printf("-> server full\n");
    return;
  }
  s = buff;
  for (i = 0; i < 3; i++) {
    switch (i) {
      case 0: p = clients[slot].name;
              sz = sizeof(clients[slot].name);
              break;
      case 1: p = pw;
              sz = sizeof(pw);
              break;
      case 2: p = ver;
              sz = sizeof(ver);
              break;
      default: return;
    }
    j = 0;
    while (*s && *s != '\\') {
      if (j > sz - 2) {
        while (*s && *s != '\\')
          *s++;
        break;
      }
      *p++ = *s++;
      j++;
    }
    *p = '\0';
    *s++;
  }
  if (strcmp(ver, PROTOCOL_VERSION)) {
    _strnprintf(msg, sizeof(msg), "protocol mismatch: %s != %s", ver, PROTOCOL_VERSION);
    sv_outside_message(S2C_BAD_CONNECT, msg);
    if (sv_cvar_value(CVAR_MESSAGES))
      printf("-> wrong protocol for %s (%s != %s)\n", clients[slot].name, ver, PROTOCOL_VERSION);
    return;
  }
  if (strcmp(sv_cvar_string(CVAR_PASSWORD), "none") && strcmp(pw, sv_cvar_string(CVAR_PASSWORD))) {
    sv_outside_message(S2C_BAD_CONNECT, "bad password");
    if (sv_cvar_value(CVAR_MESSAGES))
      printf("-> bad password for %s\n", clients[slot].name);
    return;
  }
  clients[slot].addr.sin_addr.s_addr = server.addr.sin_addr.s_addr;
  clients[slot].addr.sin_port = server.addr.sin_port;
  do {
    srand(time(NULL));
    i = rand() % 126;
    for (j = 0; j < MAX_CLIENTS; j++) {
      if (clients[j].id == i) {
        i = -1;
        break;
      }
    }
  } while (i == -1);
  clients[slot].id = i;
  memset(clients[slot].stats, '\0', sizeof(clients[slot].stats));
  clients[slot].state = ca_connected;
  sv_update_time(slot);
  //if (sv_cvar_value(CVAR_MESSAGES))
    //printf("-- %s connected"/*(id:%d,time:%d)*/"\n", clients[slot].name/*, slot, clients[slot].time*/);
  _strnprintf(msg, sizeof(msg), "%c%s", clients[slot].id, sv_cvar_string(CVAR_HOSTNAME));
  sv_client_message(S2C_GOOD_CONNECT, slot, msg, strlen(msg));
  _strnprintf(msg, sizeof(msg), "%s connected", clients[slot].name);
  sv_client_message(S2C_PRINT, -1, msg, strlen(msg));
  if (sv_cvar_value(CVAR_MESSAGES))
    printf("-> %s\n", msg);
}

void sv_clear_player(int slot)
{
  clients[slot].state = ca_free;
  memset(clients[slot].stats, 0, sizeof(clients[slot].stats));
}

//ID
void sv_disconnect(char *buff)
{
  int slot;
  char msg[13+MAX_NAME+1];

  if ((slot = sv_find_slot(*buff)) == -1) {
    return;
  }
  if (sv_cvar_value(CVAR_MESSAGES))
    printf("<- %s disconnected"/*(id:%d)*/"\n", clients[slot].name/*, slot*/);
  sv_clear_player(slot);
  _strnprintf(msg, sizeof(msg), "%s disconnected", clients[slot].name);
  sv_client_message(S2C_PRINT, -1, msg, strlen(msg));
}

void sv_zombies(void)
{
  int i;
  char msg[10+MAX_NAME+1];

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state != ca_connected)
      continue;
    if ((server.realtime - clients[i].time) > 15) {
      sv_clear_player(i);
      _strnprintf(msg, sizeof(msg), "%s timed out", clients[i].name);
      sv_client_message(S2C_PRINT, -1, msg, strlen(msg));
      if (sv_cvar_value(CVAR_MESSAGES))
        printf("-> %s"/* (id:%d)*/"\n", msg/*, i*/);
    }
  }
}

//<YOUR_ID[1]><POSX[4]><POSY[4]><POSZ[4]><HEALTH[4]><ARMOUR[4]><ITEMS[4]>
void sv_update_stat(char *buff, int sz)
{
  int slot;

  if (sz != MAX_STAT_SIZE + 1)
    return;
  if ((slot = sv_find_slot(*buff++)) == -1)
    return;
  if (sv_cvar_value(CVAR_MESSAGES) == 2)
    printf("<- update stats: %s"/* (id:%d)*/"\n", clients[slot].name/*, slot*/);

  memcpy(clients[slot].stats, buff, sz);
  sv_update_time(slot);
  clients[slot].update = 1;
}

/*void sv_print_stat(char *buff)
{
  client_stats_t stats;
  int i, j;
  char byte[4];

  for (i = 0; i < 6; i++) {
    for (j = 0; j < 4; j++)
      byte[j] = *buff++;
    switch (i) {
      case 0: stats.pos[0] = readfloat(byte); break;
      case 1: stats.pos[1] = readfloat(byte); break;
      case 2: stats.pos[2] = readfloat(byte); break;
      case 3: stats.carrying = readint(byte); break;
      case 4: _strnprintf(stats.armour, sizeof(stats.armour), "%d", readint(byte)); break;
      case 5: _strnprintf(stats.health, sizeof(stats.health), "%d", readint(byte)); break;
    }  
  }
  printf("(%f,%f,%f) %d,%s,%s\n",
    stats.pos[0],
    stats.pos[1],
    stats.pos[2],
    stats.carrying,
    stats.armour,
    stats.health);
}*/

void sv_no_update(char *buff)
{
  int slot;

  if ((slot = sv_find_slot(*buff++)) == -1)
    return;
  if (sv_cvar_value(CVAR_MESSAGES) == 2)
    printf("<- no update: %s"/* (id:%d)*/"\n", clients[slot].name/*, slot*/);
  clients[slot].update = 0;
}

void sv_request_stat(char *buff)
{
  int slot, len, names, self;
  int i, j;
  char pkt[MAX_FULL_STAT+1];
  char *s, *p;

  if ((slot = sv_find_slot(*buff++)) == -1)
    return;
  if (sv_cvar_value(CVAR_MESSAGES) == 2)
    printf("<- request stats: %s"/* (id:%d)*/"\n", clients[slot].name/*, slot*/);
  sv_update_time(slot);

  self = *buff;
  s = pkt+1;
  len = 0;
  names = 0;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state != ca_connected)
      continue;
    if (!clients[i].update)
      continue;
    if ((i == slot) && !self)
      continue;
    p = clients[i].name;
    while (*p)
      *s++ = *p++;
    *s++ = '\0';
    j = strlen(clients[i].name);
    len += j + 1;
    if (j > names)
      names = j;
    p = clients[i].stats;
    for (j = 0; j < MAX_STAT_SIZE; j++)
      *s++ = *p++;
    *s++ = '\0';
    len += MAX_STAT_SIZE + 1;
  }
  *s = '\0';
  pkt[0] = (char)names;
  if (!len)
    return;
  sv_client_message(S2C_STAT_PRINT, slot, pkt, len + 1 + 1);
}

void sv_ping(char *buff)
{
  int slot;

  if ((slot = sv_find_slot(*buff)) == -1)
    return;
  if (sv_cvar_value(CVAR_MESSAGES) == 3)
    printf("<- ping: %s"/* (id:%d)*/"\n", clients[slot].name/*, slot*/);
  sv_update_time(slot);
  if (sv_cvar_value(CVAR_MESSAGES) == 3) 
    printf("-> pong\n");
  sv_client_message(S2C_PONG, slot, NULL, 0);
}

void sv_client_message(char tag, int slot, char *msg, int len)
{
  char buff[MAX_FULL_STAT+2+1];
  int i;
  int sz;

  buff[0] = A2A_PER_ID;
  buff[1] = tag;
  if (msg)
    memcatz(buff, 2, msg, len, sizeof(buff));

  if (slot > -1) {
    if (clients[slot].state != ca_connected)
      return;
#ifdef WIN32
    if ((sz = sendto(server.sock, buff, len + 2, 0, (LPSOCKADDR)&clients[slot].addr, sizeof(SOCKADDR))) == -1) {
#endif
#ifdef UNIX
    if ((sz = sendto(server.sock, buff, len + 2, 0, (struct sockaddr *)&clients[slot].addr, sizeof(struct sockaddr))) == -1) {
#endif
      if (sv_cvar_value(CVAR_MESSAGES) == 2)
        printf("-- client sendto() error\n");
    }
    else {
      server.bytes_out += sz;
    }
    return;
  }

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state != ca_connected)
      continue; 
#ifdef WIN32
    if ((sz = sendto(server.sock, buff, len + 2, 0, (LPSOCKADDR)&clients[i].addr, sizeof(SOCKADDR))) == -1) {
#endif
#ifdef UNIX
    if ((sz = sendto(server.sock, buff, len + 2, 0, (struct sockaddr *)&clients[i].addr, sizeof(struct sockaddr))) == -1) {
#endif
      if (sv_cvar_value(CVAR_MESSAGES) == 2)
        printf("-- client sendto() error\n");
    }
    else {
        server.bytes_out += sz;
    }
  }
}

void sv_outside_message(char tag, char *msg)
{         
  char buff[MAX_SINGLE_STAT+1];
  int sz;

  _strnprintf(buff, sizeof(buff), "%c%c%s", A2A_PER_ID, tag, msg);
#ifdef WIN32
  if ((sz = sendto(server.sock, buff, strlen(buff), 0, (LPSOCKADDR)&server.addr, sizeof(SOCKADDR))) == -1) {
#endif
#ifdef UNIX
  if ((sz = sendto(server.sock, buff, strlen(buff), 0, (struct sockaddr *)&server.addr, sizeof(struct sockaddr))) == -1) {
#endif
    if (sv_cvar_value(CVAR_MESSAGES) == 2)
      printf("-- outside sendto() error\n");
  }
  else {
    server.bytes_out += sz;
  }
}

void sv_update_time(int slot)
{
  clients[slot].time = server.realtime;
}

void sv_print_net(void)
{
  printf("-- %d bytes in, %d bytes out in %lu seconds\n", server.bytes_in, server.bytes_out, server.realtime);
}

int sv_compare_addr(struct sockaddr_in *a, struct sockaddr_in *b)
{
  if (a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port)
    return 1;
  return 0;
}

void sv_status(void)
{
  int i;//, j;
  int users;

  users = 0;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state == ca_connected)
      users++;
  }
  if (!users) {
    printf("-- no users\n");
    return;
  }

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].state != ca_connected)
      continue;
    printf("-- %s %s:%d (id:%d,time:%d)\n",
      clients[i].name,
      inet_ntoa(clients[i].addr.sin_addr),
      ntohs(clients[i].addr.sin_port),
      i,
      clients[i].time);
    /*for (j = 0; j < MAX_STAT_SIZE; j++)
      printf("%c", (clients[i].stats[j] < ' ') ? '.' : clients[i].stats[j]);
    printf("\n");*/
  }
  printf("-- %d %s\n", users, (users == 1) ? "user" : "users");
}

int sv_read_cfg(void)
{
  FILE *fp;
  char line[MAX_KEY+1+MAX_VALUE+1+1];
  char key[MAX_KEY+1];
  char value[MAX_VALUE+1];
  char *s, *p;
  char c;
  int i, j;
  cvar_t *cvar;

  fp = fopen(PSV_CFG_FILE, "r");
  if (!fp) {
    if (sv_cvar_value(CVAR_MESSAGES) == 2)
      printf("-- couldn\'t load " PSV_CFG_FILE " \n");
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
    if (!strncmp(line, "//", 2))
      goto _skip;
    if (!strchr(line, '=')) {
      printf("-- error in %s on line %d\n", PSV_CFG_FILE, j);
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
    cvar = sv_find_cvar(key);
    if (cvar)
      sv_change_cvar(cvar, value);
    else
      sv_add_cvar(key, value, 1, NULL);
_skip:
    if (c == EOF)
      break;
  }
  fclose(fp);

  return 1;
}

int sv_new_cvar(cvar_t *cvar)
{
  cvar = (cvar_t *)malloc(sizeof(cvar_t));
  if (!cvar)
    return 0;

  cvar->name[0] = '\0';
  cvar->string[0] = '\0';
  cvar->value = 0;
  cvar->type = 1;
  cvar->function = NULL;

  cvar->next = server.cvars;
  server.cvars = cvar;

  return 1;
}

cvar_t *sv_find_cvar(char *name)
{
  cvar_t *cvar;

  for (cvar = server.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar;
  }
  return NULL;
}

void sv_list_cvars(void)
{
  cvar_t *cvar;
  int i;
 
  for (i = 0, cvar = server.cvars; cvar; cvar = cvar->next, i++) {
    printf("-- %c %s = \"%s\"", (!cvar->type) ? '*' : ' ', cvar->name, cvar->string);
    if (sv_cvar_value(CVAR_MESSAGES) == 2)
      printf(" (%d)", cvar->value);
    printf("\n");
  }
  printf("-- %d cvar%s (*standard cvars)\n", i, (i != 1) ? "s" : "");
}

char *sv_cvar_string(char *name)
{
  cvar_t *cvar;

  for (cvar = server.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar->string;
  }
  return NULL;
}

int sv_cvar_value(char *name)
{
  cvar_t *cvar;

  for (cvar = server.cvars; cvar; cvar = cvar->next) {
    if (!strcmp(cvar->name, name))
      return cvar->value;
  }
  return 0;
}

void sv_change_cvar(cvar_t *cvar, char *value)
{
  strlcpy(cvar->string, value, MAX_VALUE+1);
  cvar->value = atoi(value);
  if (cvar->function)
    cvar->function();
}

void sv_add_cvar(char *name, char *value, int type, func_t function)
{
  cvar_t *cvar;

  if (sv_find_cvar(name)) {
    printf("-- cvar \"%s\" already exists\n", name);
    return;
  }

  if (!sv_new_cvar(server.cvars)) {
    printf("-- failed to create cvar %s\n", name);
    return;
  }
  cvar = server.cvars;
  strlcpy(cvar->name, name, sizeof(cvar->name));
  cvar->function = (function) ? function : NULL;
  cvar->type = type;
  sv_change_cvar(cvar, &*value);
}

#if 0
void sv_add_user_cvar(char *s)
{
  char key[MAX_KEY+1];
  char value[MAX_KEY+1];

  if (!*s) {
    printf("-- empty cvar name\n");
    return;
  }
  
  tokenize(s, key, sizeof(key), value, sizeof(value), ' ');
  sv_add_cvar(key, value, 1, NULL);
}
#endif

void sv_remove_cvars(void)
{
  cvar_t *cvar;

  while (server.cvars) {
    cvar = server.cvars;
    server.cvars = server.cvars->next;
    free(cvar);
  }
}

void sv_max_clients_change(void)
{
  cvar_t *c;
  char s[16];

  c = sv_find_cvar(CVAR_MAX_CLIENTS);
  if (!c)
    return;
  c->value = bound(0, MAX_CLIENTS, c->value);
  _strnprintf(s, sizeof(s), "%d", c->value);
  strlcpy(c->string, s, sizeof(c->string));
}


void sv_password_change(void)
{
  char *value;

  value = sv_cvar_string(CVAR_PASSWORD);
  value[MAX_PASSWORD] = '\0';
}

