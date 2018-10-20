#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <uchar.h>
#include <math.h>

struct {
  unsigned char * area;
  uint32_t firstIndex;
  uint32_t size;
} arena;

uint32_t readBlock(uint32_t index){
  uint32_t i;
  uint32_t returnValue;
  for(i = 6 ; i >= 3 ; i--){
    returnValue = returnValue << 8 | arena.area[index + i - 3];
  }
  return returnValue;
}

void writeBlock(uint32_t index, uint32_t value){
  uint32_t i;
  for(i = 3 ; i <= 6 ; i++){
    arena.area[index + i - 3] = (unsigned char)((value & (255 << (8 * (i-3)))) >> (8 * (i-3)));
  }
}

void initializeCommand(uint32_t value){
  arena.area = calloc(value, 1);
  arena.size = value;
}

void finalizeCommand(){
  free (arena.area);
  arena.firstIndex = 0;
}

void dumpCommand(){
  uint32_t i, j;
  uint32_t size = ceil(arena.size / 16.0);
  for(j = 0 ; j < size ; j++){
    printf("%08X\t" , j * 16);
    for(i = 0 ; i < 16 ; i++){
      if(j * 16 + i < arena.size){
        printf("%02X ", arena.area[16 * j + i]);
        if(i == 7){
          printf(" ");
        }
      }
    }
    printf("\n");
  }

}

uint32_t allocCommand(uint32_t value){
  if(value < arena.size){
    uint32_t thisIndex = arena.firstIndex;
    uint32_t nextIndex = readBlock(thisIndex);
    //uint32_t prevIndex = readBlock(thisIndex+4);
    uint32_t blockLength = readBlock(thisIndex+8);

    if(thisIndex != 0){
      //printf("D");
      if(thisIndex >= 12 + value){
        writeBlock(0, thisIndex);
        writeBlock(4, 0);
        writeBlock(8, value);
        writeBlock(thisIndex + 4, 0);
        arena.firstIndex = 0;
        return 12;
      }
    }

    while(nextIndex != 0){
      if(nextIndex >= thisIndex + 24 + blockLength + value){
        //printf("C");
        uint32_t newBlockIndex = thisIndex + 12 + blockLength;
        writeBlock(newBlockIndex, nextIndex);
        writeBlock(newBlockIndex + 4, thisIndex);
        writeBlock(newBlockIndex + 8, value);
        writeBlock(thisIndex, newBlockIndex);
        writeBlock(nextIndex + 4, newBlockIndex);

        return newBlockIndex + 12;
      }

      blockLength = readBlock(nextIndex+8);
      thisIndex = nextIndex;
      nextIndex = readBlock(nextIndex);
      //prevIndex = thisIndex;

    }

    if(nextIndex == 0){
      if(blockLength == 0){
        //printf("A");
        if(arena.size >= 12 + value){
            writeBlock(8, value);
            arena.firstIndex = 0;
            return 12;
        }
      } else {
        //printf("B");
        uint32_t newBlockIndex = thisIndex + 12 + blockLength;
        if(arena.size >= newBlockIndex + 12 + value){
            writeBlock(newBlockIndex + 4, thisIndex);
            writeBlock(newBlockIndex + 8, value);
            writeBlock(thisIndex, newBlockIndex);
            return newBlockIndex + 12;
        }
      }
    }
  }
  return 0;
}

void freeCommand(uint32_t value){
  value = value - 12;
  uint32_t i;
  uint32_t nextIndex = readBlock(value);
  uint32_t prevIndex = readBlock(value+4);
  uint32_t blockLenght = readBlock(value+8);
  if(value == arena.firstIndex){
    arena.firstIndex = nextIndex;
    writeBlock(value+4, 0);
  }
  else {
    writeBlock(prevIndex, nextIndex);
  }
  if(nextIndex != 0){
    writeBlock(nextIndex + 4, prevIndex);
  }


  for(i = 0 ; i < 12+blockLenght ; i++){
    arena.area[value + i] = 0;
  }
}

void fillCommand(uint32_t a, uint32_t b, uint32_t c){
  a=a-12;
  uint32_t nextIndex = readBlock(a);
  uint32_t blockLenght = readBlock(a+8);
  while(b){
    uint32_t i;
    for(i=0;i<blockLenght && b;i++){
      //printf("%d %d\n",a+12+i, b);
      arena.area[a+12+i] = (unsigned char)c;
      b--;
    }
    if(nextIndex == 0 && b){
      return;
    }
    a = nextIndex;
    nextIndex = readBlock(nextIndex);
    blockLenght = readBlock(a+8);
  }
}

void invalidCommand(){
  printf("Invalid command");
  exit(0);
}

void commandProcessor(char* command)
{
    const char* delimiters = " \n";

    char* commandName = strtok(command, delimiters);
    if (!commandName) {
      invalidCommand();
    }

    if (strcmp(commandName, "INITIALIZE") == 0) {
        char* commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t size = atoi(commandValueString);
        initializeCommand( size );

    } else if (strcmp(commandName, "FINALIZE") == 0) {
        finalizeCommand();

    } else if (strcmp(commandName, "DUMP") == 0) {
        dumpCommand();

    } else if (strcmp(commandName, "ALLOC") == 0) {
        char* commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t commandValue = atoi(commandValueString);
            printf("%d\n", allocCommand(commandValue));

    } else if (strcmp(commandName, "FREE") == 0) {
        char* commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t commandValue = atoi(commandValueString);
        freeCommand(commandValue);

    } else if (strcmp(commandName, "FILL") == 0) {
        char* commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t commandValueOne = atoi(commandValueString);
        commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t commandValueTwo = atoi(commandValueString);
        commandValueString = strtok(NULL, delimiters);
        if (!commandValueString) {
            invalidCommand();
        }
        uint32_t commandValueThree = atoi(commandValueString);
        fillCommand(commandValueOne, commandValueTwo, commandValueThree);

    } else {
        invalidCommand();
    }

    return;
}

int main(void)
{

    ssize_t read;
    char* line = NULL;
    size_t len;

    while ((read = getline(&line, &len, stdin)) != -1) {
        printf("%s", line);
        commandProcessor(line);
    }

    free(line);

    return 0;
}
