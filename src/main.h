#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
struct Cell;

typedef enum {
    FALSE,
    TRUE
} Bool;

//AType < BType < CType < AType
//Род животного
typedef enum {
    NoneType,
    AType,
    BType,
    CType
} Genus;

//позиция на поле
typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    //Лимит животного, сколько он может прожит без еды
    unsigned int maxHungerStrike;   
    unsigned int maxAge;
    unsigned int stepTimeSpan;
} Limits;

struct Animal;

typedef struct Cell {
    struct Animal** ptr;
    // struct int* animalDescriptor; 
    Position position;
} Cell;

void cellUnwrapAnimal(Cell cell);

typedef struct Animal{
    //Возраст, на котором животное в последний раз поело
    unsigned int lastEatTime;
    unsigned int age;
    Bool isDead;
    Genus type;
    Limits limits;
    Cell currentCell;
    pthread_t threadId;
    pthread_mutex_t mutexId;
} Animal;

Animal* newAnimal(Genus type, Limits limits);
void printAnimal(Animal* animal);

typedef struct {
    int width;
    int height;
} FieldSize;

typedef struct{
    //Отброшенно из за переполнения
    int discardedAnimals;
    //Родилось
    int newbornAnimals;
    //Съедено
    int eatenAnimals;
    //Умерло по естественным причинам
    int dead;
} Statistics;

//Представление поля
typedef struct {
    Animal** pointers;
    Statistics stats;
    FieldSize size;
    // pthread_mutex_t transition_mutex;
} Field;

Field* newField(int width, int height);
Cell getCell(Field* field, Position position);
void printField(Field* field);

Bool setToRandomFreePosition(Animal* animal, Field* field);
//Ищет ближайшее свободное место, отностиельно точки
Bool setToNearestFreePosition(Animal *animal, Field *field, Position startPosition);

void runAnimalLifeCycle(Animal* animal, Field* field);
void* animalLifeCycle(void* animal);
Animal** createAnimals(Field* field, int a, int b, int c);

//Методы жизненного цикла
Position selectNextPosition(Position position, FieldSize fieldSize);
void doAction(Animal* animal, Field* field, Cell newCell);

//Больше нет ивентов  :(
void move(Animal* animal, Field* field, Cell cell);
void multiply(Animal* parent1, Animal* parnet2, Field* field);
void eatIt(Animal* predator, Animal* prey, Field* field);
Bool giveBirth(Animal* animal, Field* field);

typedef struct {
    Animal* animal;
    Field* field;
} InitialArgs;
