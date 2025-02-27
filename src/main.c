#include "main.h"

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FIELD_SHARED_MEMORY_NAME "LAB2_SHM"


int main(int argc, char const *argv[])
{ 
    const int aCount = 5;
    const int bCount = 4;
    const int cCount = 3;
    const int sum = aCount + bCount + cCount;

    setbuf(stdout, 0);
    
    FieldSize size = {.width = 20, .height = 10};
    Field field = createField(size);
    Animal** animals = createAnimals(field, aCount, bCount, cCount);
    
    int lastId = 0;
    for(int i = 0; i < sum; i++)
    {
        int itemIndex = animals[i] -> itemIndex;
        int id = fork();
        if (id != 0)
        {
            runAnimalLifeCycle(itemIndex, size);
            return 0;
        }
        lastId = id;
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

Field readField(int fd, FieldSize size)
{
    int cells = size.width * size.height;

    int total_size = (sizeof(int) + sizeof(Animal)) * cells; 
    void* shared_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int* indexes = (int*) shared_memory;
    Animal* animals = (Animal*) (indexes + cells);

    Field field = {
        .indexes = indexes,
        .animals = animals,
        .fd = fd,
        .size = size
    };

    return field;
}

Field createField(FieldSize size)
{
    shm_unlink(FIELD_SHARED_MEMORY_NAME);
    int fd = shm_open(FIELD_SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, FILE_MODE);

    Field field = readField(fd, size);
    int cells = size.width * size.height;
    
    ftruncate(fd, (sizeof(int) + sizeof(Animal)) * cells);
    close(fd);

    for (int i = 0; i < cells; i++)
    {
        field.indexes[i] = -1;
        field.animals[i].isAlive = FALSE;
        field.animals[i].itemIndex = i;

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&field.animals[i].mutexId, &attr);
    }
    return field;
}

Animal** createAnimals(Field field, int a, int b, int c)
{
    int sum = a+b+c;
    Animal** animals = calloc(sum, sizeof(Animal*));

    Limits limits = {
        .maxAge = 30,
        .maxHungerStrike = 10,
        .stepTimeSpan = 1000000 + rand() % 1000000
    };

    for (int i = 0; i < sum; i++)
    {
        animals[i] = mallocAnimal(
            field,
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

void runAnimalLifeCycle(int animalIndex, FieldSize size)
{
    int fd = shm_open(FIELD_SHARED_MEMORY_NAME, O_RDWR, FILE_MODE);
    Field field = readField(fd, size);
    close(fd);

    Animal* animal = field.animals + animalIndex;
    animalLifeCycle(animal, field);
}

void updateCycle(Animal* animal, Field field)
{
    animal->age++;
    if ((animal->age - animal->lastEatTime) >= animal->limits.maxHungerStrike)
    {
        animal->isAlive = FALSE;
        return;
    }

    if (animal->age >= animal->limits.maxAge)
    {
        animal->isAlive = FALSE;
        return;
    }

}

void animalLifeCycle(Animal* animal, Field field)
{
    while (animal->isAlive){
        Position nextPosition = selectNextPosition(
            animal->currentPosition, 
            field.size
        );

        Cell cell = getCell(field, nextPosition);
        doAction(&animal, &field, cell);

        usleep(animal->limits.stepTimeSpan);
        updateCycle(animal, field);
    }

    Cell cell = getCell(field, animal->currentPosition);
    if (unwrapAnimal(field, cell) == animal)
    {
        (*cell.index) = -1;
    } 
}


void doAction(Animal** animal, Field* field, Cell newCell)
{
    Animal* cellOwner = unwrapAnimal(*field, newCell);
    if (cellOwner == NULL)
    {
        move(*animal, *field, newCell);
    }
    else if (cellOwner == *animal)
    {
        usleep((*animal)->limits.stepTimeSpan);
    }
    else if (cellOwner->type == (*animal)->type)
    {
        multiply(animal, cellOwner, field);
    }
    else if (cellOwner->type == (((*animal)->type - 1 + 4) % 4))
    {
        eatIt(*animal, cellOwner, *field);
    }
    else
    {
        eatIt(cellOwner, *animal, *field);
    }
}


void eatIt(Animal *predator, Animal *prey, Field field)
{   
    predator->lastEatTime = predator->age;

    pthread_mutex_lock(&prey->mutexId);
    prey->isAlive = FALSE;
    pthread_mutex_unlock(&prey->mutexId);
}

void move(Animal *animal, Field field, Cell cell)
{
    (*cell.index) = animal->itemIndex;

    Cell oldCell = getCell(field, animal->currentPosition);
    animal->currentPosition = cell.position;
    usleep(animal->limits.stepTimeSpan);

    (*oldCell.index) = -1;
}

void multiply(Animal** mainParent, Animal *parnet2, Field* field)
{
    pthread_mutex_lock(&parnet2->mutexId);
    Bool canMultiply = parnet2->isAlive;
    pthread_mutex_unlock(&parnet2->mutexId);

    pthread_mutex_lock(&(*mainParent)->mutexId);
    canMultiply &= (*mainParent)->isAlive;
    pthread_mutex_unlock(&(*mainParent)->mutexId);


    if (canMultiply)
        giveBirth(mainParent, field);
}


void fork_animal(Animal** parent, Field* field, Animal* child)
{
    int childIndex = child->itemIndex;
    FieldSize size = field->size;
    if (fork() == 0)
        return;
    
    int fd = shm_open(FIELD_SHARED_MEMORY_NAME, O_RDWR, FILE_MODE);
    (*field) = readField(fd, size);
    close(fd);

    (*parent) = field->animals + childIndex;
};

Bool giveBirth(Animal **animal, Field* field)
{
    Animal* child = mallocAnimal(*field, (*animal)->type, (*animal)->limits);
    if (child == NULL)
        return FALSE;
        
    if (!setToRandomFreePosition(child, *field))
        return FALSE;

    fork_animal(animal, field, child);
    return TRUE;
}

void printAnimal(Animal *animal)
{
    printf(
        "animal(type: %i, age: %i, lastEatTime: %i, cell.x:%i, cell.y:%i)\n",
        animal->type,
        animal->age,
        animal->lastEatTime,
        animal->currentPosition.x,
        animal->currentPosition.y
    );
}

Cell getCell(Field field, Position position)
{
    Cell cell = {NULL, -1, -1};
    if (position.x >= field.size.width || position.y >= field.size.height)
        return cell;

    int realPosition = position.x * field.size.height + position.y;
    cell.index = field.indexes + realPosition;
    cell.position = position;
    
    return cell;
}

void printField(Field field)
{
    // printf("discarded: %i newborn: %i eaten: %i, dead: %i\n", 
    //     field.stats.discardedAnimals, 
    //     field.stats.newbornAnimals, 
    //     field.stats.eatenAnimals,
    //     field.stats.dead
    // );
    for (int j = 0; j < field.size.height; j++)
    {
        for (int i = 0; i < field.size.width; i++)
        {
            Position position = {i, j};
            Cell cell = getCell(field, position);
            Animal* animal = unwrapAnimal(field, cell);
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

Bool setToRandomFreePosition(Animal *animal, Field field)
{
    Position startPosition = {
        rand() % field.size.width, 
        rand() % field.size.height
    };
    return setToNearestFreePosition(animal, field, startPosition);
}

Bool setToNearestFreePosition(Animal *animal, Field field, Position startPosition)
{
    int generation = 0;
    int lastGeneration = -1;
    while (generation < field.size.height || generation < field.size.width){
        for (int i = -generation; i <= generation; i += 1)
        {
            Position newPosition = { startPosition.x + i, 0};
            if (newPosition.x < 0) 
                continue;
            if (newPosition.x >= field.size.width) 
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
                if (newPosition.y >= field.size.height)
                    break;

                Cell cell = getCell(field, newPosition);
                if ((*cell.index) >= 0)
                    continue;

                (*cell.index) = animal->itemIndex;
                animal->currentPosition = cell.position;
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

Animal* tryReviveAnimal(Field field){
    int size = field.size.width * field.size.height;
    int offset = size * sizeof(int);

    for (int i = 0; i < size; i++)
    {
        Animal* selected = field.animals + i;
        if (selected->isAlive)
            continue;

        selected->isAlive = TRUE;
        return selected;
    }

    return NULL;
}

Animal *unwrapAnimal(Field field, Cell cell)
{
    if ((*cell.index) < 0)
        return NULL;

    return field.animals + (*cell.index);
}

Animal *mallocAnimal(Field field, Genus type, Limits limits)
{
    Position cell = {-1, -1};
    Animal* animal = tryReviveAnimal(field);

    if (animal == NULL){
        return NULL;
    }

    animal->age = 0;
    animal->lastEatTime = 0;
    animal->type = type;
    animal->limits = limits;
    animal->currentPosition = cell;
    return animal;
}
