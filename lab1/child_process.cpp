#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstring>

#include "error_handling.h"

void removeNewlineSymb(char* buffer, ssize_t& length) {
	if (length > 0 && buffer[length - 1] == '\n') {
		buffer[length - 1] = '\0';
		--length;
	}
}

void child1Process(int pipe1[2], int pipe2[2]) {
	close(pipe1[1]);
	close(pipe2[0]);

	char buffer[1024];
	ssize_t bytesRead = read(pipe1[0], buffer, sizeof(buffer));
	if (bytesRead < 0) {
		myPerror("Ошибка при чтении из pipe1 в child1");
		_exit(errno);
	} else if (bytesRead > 0) {
		buffer[bytesRead] = '\0';
		removeNewlineSymb(buffer, bytesRead);

		for (int i = 0; i < bytesRead; ++i) {
			buffer[i] = tolower(buffer[i]);
		}
		if (write(pipe2[1], buffer, strlen(buffer)) == -1) {
			myPerror("Ошибка при записи в pipe2 в child1");
			_exit(errno);
		}
	}

	close(pipe1[0]); 
	close(pipe2[1]); 
}

void child2Process(int pipe2[2], int pipe3[2]) {
	close(pipe2[1]);
	close(pipe3[0]);

	char buffer[1024];
	ssize_t bytesRead = read(pipe2[0], buffer, sizeof(buffer));
	if (bytesRead < 0) {
		myPerror("Ошибка при чтении из pipe2 в child2");
		_exit(errno);
	} else if (bytesRead > 0) {
		buffer[bytesRead] = '\0'; 
		removeNewlineSymb(buffer, bytesRead);

		for (int i = 0; i < bytesRead; ++i) {
			if (isspace(buffer[i])) {
				buffer[i] = '_';
			}
		}

		if (write(pipe3[1], buffer, strlen(buffer)) == -1) {
			myPerror("Ошибка при записи в pipe3 в child2");
			_exit(errno);
		}
	}

	close(pipe2[0]);
	close(pipe3[1]);
}
