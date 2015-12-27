#include <stdlib.h>
#include <stdio.h>
#include <termbox.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define ENEMY_COUNT 10
#define LIMIT 16666 * 2

typedef struct {
  int x;
  int y;
  int speed;
} Bullet;

typedef struct {
  int x;
  int y;
  Bullet bullet;
  int dir;
  int hp;
} Entity;

Entity player;
Entity enemies[ENEMY_COUNT];
unsigned long frames;

void render_player(Entity * e){
  tb_change_cell(e->x-1,e->y,'/',255,255);
  tb_change_cell(e->x,e->y,'*',255,255);
  tb_change_cell(e->x+1,e->y,'\\',255,255);
  tb_change_cell(e->x,e->y-1,'|',255,255);
}

void render_enemy(Entity * e){
  tb_change_cell(e->x-1,e->y,'\\',255,255);
  tb_change_cell(e->x,e->y,'-',255,255);
  tb_change_cell(e->x+1,e->y,'/',255,255);
}

int collides(Bullet * b, Entity * e){
  return b->speed != 0 && b->x >= e->x-1 && b->x <= e->x+1 && b->y == e->y;
}

void start_bullet(Entity * e, int speed){
  if (e->bullet.speed != 0 && e->bullet.x > 0 && e->bullet.x < tb_width() && e->bullet.y < tb_height() && e->bullet.y > 0)
    return;
  e->bullet.x = e->x;
  e->bullet.y = e->y;
  e->bullet.speed = speed;
}

void loop_bullet(Bullet * b) {
  if(b->speed == 0) return;
  if(b->x < 0 || b->x > tb_width() || b->y < 0 || b->y > tb_height())
    b->speed = 0;
  else b->y += b->speed;
}

void loop_enemy(Entity * e){
  if (e->hp > 0){
    if (frames % 8 == 0) {
      if (e->x + e->dir < tb_width() && e->x + e->dir > 0){
        e->x += e->dir;
      } else {
        e->y += 2; e->dir *= -1;
      }
    }
    if (player.x == e->x) start_bullet(e, 1);
  }
  if (collides(&e->bullet, &player)) {
    player.hp -= 15;
    e->bullet.speed = 0;
  }
  loop_bullet(&e->bullet);
}

int loop() {
  if (player.hp <= 0) return 0;
  struct tb_event ev;
  frames++;
  tb_peek_event(&ev, 0);
  if(ev.type == TB_EVENT_KEY){
    switch(ev.key) {
      case TB_KEY_ESC: return 0; break;
      case TB_KEY_ARROW_LEFT: if(player.x > 0) player.x--; break;
      case TB_KEY_ARROW_RIGHT: if(player.x < tb_width()-1) player.x++; break;
      case TB_KEY_ARROW_UP: if(player.y > 0) player.y--; break;
      case TB_KEY_ARROW_DOWN: if(player.y < tb_height()-4) player.y++; break;
      case TB_KEY_SPACE:
        start_bullet(&player, -1);
        break;
    }
  }
  loop_bullet(&player.bullet);
  for(int i=0;i<ENEMY_COUNT;i++){
    if(collides(&player.bullet, &enemies[i])) enemies[i].hp = 0;
    loop_enemy(&enemies[i]);
  }
  return 1;
}

void render_bullet(Bullet * b){
  if (b->speed == 0) return;
  tb_change_cell(b->x,b->y,'|',255,255);
}

void render_hud(){
  for(int i=0;i<tb_width();i++) tb_change_cell(i,tb_height()-2,'-',255,255);
  char msg[50];
  if (player.hp < 0) player.hp = 0;
  sprintf(msg, "Press ESC to exit | HP: %d%%",player.hp);
  for(int i=0;i<strlen(msg);i++) tb_change_cell(i,tb_height()-1,msg[i],255,255);
}

int render() {
  tb_clear();
  tb_select_output_mode(TB_OUTPUT_NORMAL);
  for(int i=0;i<ENEMY_COUNT;i++){
    if(enemies[i].hp > 0){
      render_enemy(&enemies[i]);
    }
    render_bullet(&enemies[i].bullet);
  }
  render_player(&player);
  render_bullet(&player.bullet);
  render_hud();
  tb_present();
}

void init_entity(Entity * e, int x, int y){
  e->x = x;
  e->y = y;
  e->hp = 1;
  e->dir = 1;
  e->bullet.x = 0;
  e->bullet.y = 0;
  e->bullet.speed = 0;
}

int init() {
  tb_init();
  frames = 0;
  init_entity(&player, floor(tb_width() / 2), tb_height() - 4);
  player.dir = 3;
  player.hp = 100;
  for(int i=0;i<ENEMY_COUNT;i++) init_entity(&enemies[i], 2 + i * 5, 3);
}

int cleanup() {
  tb_shutdown();
}

int main() {
  int ok = 1;
  long t1, t2;
  struct timespec ts;
  init();
  while(ok){
    clock_gettime(CLOCK_REALTIME,&ts);
    t1 = ts.tv_nsec / 1000;
    ok = loop();
    render();
    clock_gettime(CLOCK_REALTIME,&ts);
    t2 = ts.tv_nsec / 1000;
    long diff = t2 - t1;
    if(diff < LIMIT) usleep(LIMIT - diff);
  }
  cleanup();
  if (player.hp < 0) printf("You Lost!");
}
