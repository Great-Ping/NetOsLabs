#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

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

typedef struct Animal{
    //Возраст, на котором животное в последний раз поело
    unsigned int lastEatTime;
    unsigned int age;
    int itemIndex;
    Bool isAlive;
    Genus type;
    Limits limits;
    Position currentPosition;
    pthread_t threadId;
    pthread_mutex_t mutexId;
} Animal;

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
    int* indexes;
    Animal* animals;
    int fd;
    FieldSize size;
    // Statistics stats;
    // pthread_mutex_t transition_mutex;
} Field;


typedef struct Cell {
    int* index;
    // struct int* animalDescriptor; 
    Position position;
} Cell;

Animal* unwrapAnimal(Field field, Cell cell);
Animal* mallocAnimal(Field field, Genus type, Limits limits);

Field readField(int fd, int width, int height);
Field createField(int width, int height);
Cell getCell(Field field, Position position);
void printField(Field field);

Bool setToRandomFreePosition(Animal* animal, Field field);
//Ищет ближайшее свободное место, отностиельно точки
Bool setToNearestFreePosition(Animal *animal, Field field, Position startPosition);

void runAnimalLifeCycle(Animal* animal, Field field);
void* animalLifeCycle(void* animal);
Animal** createAnimals(Field field, int a, int b, int c);

//Методы жизненного цикла
Position selectNextPosition(Position position, FieldSize fieldSize);
void doAction(Animal* animal, Field field, Cell newCell);

//Больше нет ивентов  :(
void move(Animal* animal, Field field, Cell cell);
void multiply(Animal* parent1, Animal* parnet2, Field field);
void eatIt(Animal* predator, Animal* prey, Field field);
Bool giveBirth(Animal* animal, Field field);

typedef struct {
    Animal* animal;
    Field field;
} InitialArgs;
