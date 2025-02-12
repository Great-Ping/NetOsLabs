#include "main.h"

int main(int argc, char const *argv[])
{
    return 0;
}

void printAnimal(Animal* animal)
{
}

void* animalLifeCycle(void *animal)
{
    return NULL;
}

void eatIt(Animal *predator, Animal *prey)
{
}

void multiply(Animal *parent1, Animal *parnet2)
{
}

void move(Animal *animal, Field *field, Position newPosition)
{
}

//can be null
Cell getCell(Field *field, Position position)
{
    if (position.x >= field->size.width || position.y >= field->size.height)
        return NULL;

    int realPosition = position.x * field->size.height + position.y;

    return field->pointers + realPosition;
}

void printField(Field *field)
{
}
