#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <semaphore.h>

//Род животного
typedef enum {
    A,
    B,
    C
} Genus;

//Состояние животного, во многом для дебага
typedef enum {
    ALIVE,
    EATS,
    MULTIPLIES,
    DEAD
} AnimalState;

//позиция на поле
typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    //Лимит животного, сколько он может прожит без еды
    unsigned int maxHungerStrike;   
    unsigned int maxAge;
} Limits;

typedef struct {
    //Возраст, на котором животное в последний раз поело
    unsigned int lastEatTime;
    unsigned int age;
    Genus type;
    Limits* limits;
    AnimalState state;
    Position currentPosition;
    // pthread_t thread;
} Animal;

void printAnimal(Animal* animal);
void* animalLifeCycle(void* animal);

void eatIt(Animal* predator, Animal* prey);
void multiply(Animal* parent1, Animal* parnet2);
void move(Animal* animal, Field* field, Position newPosition);

Position selectNextPosition(Position position, FieldSize fieldSize);

typedef Animal** Cell;
typedef struct {
    int width;
    int height
} FieldSize;
//Представление поля
typedef struct {
    Animal** pointers;
    FieldSize size;
    // pthread_mutex_t transition_mutex;
} Field;

Cell getCell(Field* field, Position position);
void printField(Field* field);