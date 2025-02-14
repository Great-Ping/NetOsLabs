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

// ИДЕЯ, уведомлять животное о том что мы признали что нас съел он
typedef enum {
    // Смерть
    YouAreDead,
    // Отправляется животным которое пытается войти в нашу клетку
    // Но не может нас съесть 
    // Или размножится с нами
    // По факту жертва 
    KnockToCell,
    // Предложение размножения
    Ywf,
} EventType;

typedef struct {
    EventType eventType;
    struct Animal* sender;
} Event;

typedef struct EventNode {
    Event event; 
    struct EventNode* next;
} EventNode;

typedef struct Cell {
    struct Animal** ptr; 
    Position position;
} Cell;

typedef struct Animal{
    //Возраст, на котором животное в последний раз поело
    unsigned int lastEatTime;
    unsigned int age;
    Bool isDead;
    Genus type;
    Limits* limits;
    Cell currentCell;
    EventNode* events;
    pthread_t threadId;
    pthread_mutex_t mutexId;
} Animal;

void sendEvent(Animal* recipient, Event event);
Animal* newAnimal(Genus type, Limits* limits);
void printAnimal(Animal* animal);
void* animalLifeCycle(void* animal);

typedef struct {
    int width;
    int height;
} FieldSize;

typedef struct{
    //Отброшенно из за переполнения
    int* discardedAnimals;
    //Родилось
    int* newbornAnimals;
    //Съедено
    int* eatenAnimals;
    //Умерло по естественным причинам
    int* dead;
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
//По спирали ищет ближайшее свободное место
Bool setToNearestFreePosition(Animal *animal, Field *field, Position startPosition);

//Методы жизненного цикла
Position selectNextPosition(Position position, FieldSize fieldSize);
//Не совсем MOVE, скорее какое то действие совершаемое для достижения позиции
void move(Animal* animal, Field* field, Position newPosition);
Bool giveBirth(Animal* animal, Field* field);
void processEvents(Animal *animal, Field *field);
//ЭТИ ФУНКЦИИ ПРОСТО ОТПРАВЛЯЮТ ИВЕНТЫ
void multiply(Animal* parent1, Animal* parnet2, Field* field);
void eatIt(Animal* predator, Animal* prey, Field* field);
typedef struct {
    Animal* animal;
    Field* field;
} InitialArgs;
