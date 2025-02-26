#include "main.h"

int main(int argc, char const *argv[])
{ 
    const int aCount = 10;
    const int bCount = 0;
    const int cCount = 5;
    const int sum = aCount + bCount + cCount;

    setbuf(stdout, 0);
    
    Field* field = newField(20, 10);
    Animal** animals = createAnimals(field, aCount, bCount, cCount);
    printField(field);

    for(int i = 0; i < sum; i++)
    {
        runAnimalLifeCycle(animals[i], field);
    }
    free(animals);
    
    while (TRUE)
    {
        system("clear");
        printField(field);
        usleep(50000);
    }

    return 0;
}

Field* newField(int width, int height)
{
    Field* field = malloc(sizeof(Field));
    field->pointers = calloc(width * height, sizeof(Animal*));
    
    field->stats.dead = 0;
    field->stats.discardedAnimals = 0;
    field->stats.eatenAnimals = 0;
    field->stats.newbornAnimals = 0;

    FieldSize size = {width, height};
    field->size = size;
    return field;
}

Animal** createAnimals(Field* field, int a, int b, int c)
{
    int sum = a+b+c;
    Animal** animals = calloc(sum, sizeof(Animal*));

    Limits limits = {
        .maxAge = 30,
        .maxHungerStrike = 10,
        .stepTimeSpan = 2000000 + rand() % 3000000
    };

    for (int i = 0; i < sum; i++)
    {
        animals[i] = newAnimal(
            AType,
            limits
        );
        setToRandomFreePosition(animals[i], field);
    }

    for (int i = a; i < sum - c; i++)
        animals[i]->type = BType;

    for (int i = a + b; i < sum; i++)
        animals[i]->type = CType;

    return animals;
}

void runAnimalLifeCycle(Animal* animal, Field* field)
{
    //а вдруг, вдруг мы попытались родиться вне поля
    if (animal->currentCell.ptr == NULL){
        field->stats.discardedAnimals++;
        free(animal);
        return;
    }

    field->stats.newbornAnimals++;

    // animal->limits.stepTimeSpan -= rand()%2000000;

    InitialArgs* args = malloc(sizeof(InitialArgs));
    args->animal = animal;
    args->field = field;

    pthread_create(&animal->threadId, NULL, &animalLifeCycle, args);
}

void updateCycle(Animal* animal, Field* field)
{
    animal->age++;
    if ((animal->age - animal->lastEatTime) >= animal->limits.maxHungerStrike)
    {
        animal->isDead = TRUE;
        field->stats.dead++;
        return;
    }

    if (animal->age >= animal->limits.maxAge)
    {
        animal->isDead = TRUE;
        field->stats.dead++;
        return;
    }

}

void* animalLifeCycle(void *args)
{
    Animal* animal = ((InitialArgs*)args)->animal;
    Field* field = ((InitialArgs*)args)->field;
    free(args);

    while (!animal->isDead){
        Position nextPosition = selectNextPosition(
            animal->currentCell.position, 
            field->size
        );

        Cell cell = getCell(field, nextPosition);
        doAction(animal, field, cell);

        usleep(animal->limits.stepTimeSpan);
        updateCycle(animal, field);
    }

    Cell cell = animal->currentCell;
    if ((*cell.ptr) == animal){
        (*cell.ptr) = NULL;
    } 

    free(animal);
    return NULL;
}


void doAction(Animal *animal, Field *field, Cell newCell)
{
    Animal* cellOwner = (*newCell.ptr);
    if (cellOwner == NULL)
    {
        move(animal, field, newCell);
    }
    else if ((*newCell.ptr) == animal)
    {
        usleep(animal->limits.stepTimeSpan);
    }
    else if (cellOwner->type == animal->type)
    {
        multiply(animal, cellOwner, field);
    }
    else if (cellOwner->type == ((animal->type - 1 + 4) % 4))
    {
        eatIt(animal, cellOwner, field);
    }
    else
    {
        eatIt(cellOwner, animal, field);
    }
}


void eatIt(Animal *predator, Animal *prey, Field* field)
{   
    field->stats.eatenAnimals++;
    predator->lastEatTime = predator->age;

    pthread_mutex_lock(&prey->mutexId);
    prey->isDead = TRUE;
    pthread_mutex_unlock(&prey->mutexId);
}

void move(Animal *animal, Field *field, Cell cell)
{
    (*cell.ptr) = animal;

    Cell oldCell = animal->currentCell;
    animal->currentCell = cell;
    usleep(animal->limits.stepTimeSpan);

    (*oldCell.ptr) = NULL;
}

void multiply(Animal *mainParent, Animal *parnet2, Field *field)
{
    pthread_mutex_lock(&parnet2->mutexId);
    Bool canMultiply = !parnet2->isDead;
    pthread_mutex_unlock(&parnet2->mutexId);

    pthread_mutex_lock(&mainParent->mutexId);
    canMultiply &= !mainParent->isDead;
    pthread_mutex_unlock(&mainParent->mutexId);


    if (canMultiply)
        giveBirth(mainParent, field);
}


Bool giveBirth(Animal *animal, Field *field)
{
    Animal* child = newAnimal(animal->type, animal->limits);
    Bool hasCell = setToRandomFreePosition(child, field);
    runAnimalLifeCycle(child, field);
    return hasCell;
}

Animal *newAnimal(Genus type, Limits limits)
{
    Cell cell = {NULL, -1, -1};
    Animal* animal = malloc(sizeof(Animal));

    animal->age = 0;
    animal->lastEatTime = 0;
    animal->type = type;
    animal->limits = limits;
    animal->currentCell = cell;
    pthread_mutex_init(&animal->mutexId, NULL);    

    return animal;
}

void printAnimal(Animal *animal)
{
    printf(
        "animal(type: %i, age: %i, lastEatTime: %i, cell.x:%i, cell.y:%i)\n",
        animal->type,
        animal->age,
        animal->lastEatTime,
        animal->currentCell.position.x,
        animal->currentCell.position.y
    );
}

Cell getCell(Field* field, Position position)
{
    Cell cell = {NULL, -1, -1};
    if (position.x >= field->size.width || position.y >= field->size.height)
        return cell;

    int realPosition = position.x * field->size.height + position.y;
    cell.ptr = field->pointers + realPosition;
    cell.position = position;
    
    return cell;
}

void printField(Field *field)
{
    printf("discarded: %i newborn: %i eaten: %i, dead: %i\n", 
        field->stats.discardedAnimals, 
        field->stats.newbornAnimals, 
        field->stats.eatenAnimals,
        field->stats.dead
    );
    for (int j = 0; j < field->size.height; j++)
    {
        for (int i = 0; i < field->size.width; i++)
        {
            Position position = {i, j};
            Cell cell = getCell(field, position);
            Animal* animal = (*cell.ptr);
            int type = 0;

            if (animal != NULL)
                type = animal->type;
            
            switch (type)
            {
                case 1:
                    printf("\033[34m%2i\033[0m", animal->age);
                    break;
                case 2:
                    printf("\033[35m%2i\033[0m", animal->age);
                    break;
                case 3:
                    printf("\033[32m%2i\033[0m", animal->age);
                    break;
                default:
                    printf("░░");
            } 
        }
        printf("\n");
    }
}

Bool setToRandomFreePosition(Animal *animal, Field *field)
{
    Position startPosition = {
        rand() % field->size.width, 
        rand() % field->size.height
    };
    return setToNearestFreePosition(animal, field, startPosition);
}

Bool setToNearestFreePosition(Animal *animal, Field *field, Position startPosition)
{
    int generation = 0;
    int lastGeneration = -1;
    while (generation < field->size.height || generation < field->size.width){
        for (int i = -generation; i <= generation; i += 1)
        {
            Position newPosition = { startPosition.x + i, 0};
            if (newPosition.x < 0) 
                continue;
            if (newPosition.x >= field->size.width) 
                break;

            for (int j = -generation; j <= generation; j += 1)
            {
                if (abs(j) <= lastGeneration && abs(i) < lastGeneration){
                    j = abs(j);
                    continue;
                }

                newPosition.y = startPosition.y + j;
                if (newPosition.y < 0)
                    continue;
                if (newPosition.y >= field->size.height)
                    break;

                Cell cell = getCell(field, newPosition);
                if ((*cell.ptr) != NULL)
                    continue;

                (*cell.ptr) = animal;
                animal->currentCell = cell;
                return TRUE;
            }
        }
        lastGeneration = generation;
        generation++;
    }

    return FALSE;
}

Position selectNextPosition(Position position, FieldSize fieldSize)
{
    const int offsetsCount = 6;
    Position offsets[] = {{-1, -1}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}, {1, 1}};
    int initial = rand() % offsetsCount - 1;
    int i = initial;
    while(TRUE)
    {
        Position newPosition = {
            position.x + offsets[i].x,
            position.y + offsets[i].y
        };

        i = ( i + 1 ) % offsetsCount;
        if (i == initial)
            return offsets[0];
        
        if (newPosition.x < 0)
            continue;
        if (newPosition.y < 0)
            continue;
            
        if (newPosition.x >= fieldSize.width)            
            continue;
        if (newPosition.y >= fieldSize.height)
            continue;

        return newPosition;
        
    }

    return offsets[0];
}
