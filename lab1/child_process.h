#ifndef CHILD_PROCESS_H
#define CHILD_PROCESS_H

void child1Process(int pipe1[2], int pipe2[2]);
void child2Process(int pipe2[2], int pipe3[2]);

#endif
