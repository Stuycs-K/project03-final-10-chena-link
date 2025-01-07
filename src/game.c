#define RED 0;
#define BLUE 1;
#define YELLOW 2;
#define GREEN 3;

struct card{
  int color;
  int num;
}

int main() {
  srand(getpid());
  return 0;
}
//Draws a card
void draw(){
  struct card drawn;
  drawn.color = rand()%4;
  drawn.num = rand()%10;
  //write struct to server
}
//Plays a card
void play(){

}
//Displays screen using sdl
void display(){

}
//Call out uno
//Semaphore
void call(){

}
