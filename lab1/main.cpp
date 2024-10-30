#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <vector>

#include "child_process.h"
#include "error_handling.h"

int main() {
	int pipe1[2], pipe2[2], pipe3[2];

	if (pipe(pipe1) == -1) {
		myPerror("Ошибка при создании pipe1");
		_exit(errno);
	}
	if (pipe(pipe2) == -1) {
		myPerror("Ошибка при создании pipe2");
		_exit(errno);
	}
	if (pipe(pipe3) == -1) {
		myPerror("Ошибка при создании pipe3");
		_exit(errno);
	}

	pid_t pid1 = fork();
	if (pid1 < 0) {
		myPerror("Ошибка при создании первого дочернего процесса");
		_exit(errno);
	} else if (pid1 == 0) {
		child1Process(pipe1, pipe2);
		_exit(0);
	}

	pid_t pid2 = fork();
	if (pid2 < 0) {
		myPerror("Ошибка при создании второго дочернего процесса");
		_exit(errno);
	} else if (pid2 == 0) {
		child2Process(pipe2, pipe3);
		_exit(0);
	}

	close(pipe1[0]); 
	close(pipe2[0]); 
	close(pipe2[1]); 
	close(pipe3[1]); 

	std::vector<char> input_buffer(1024);
	ssize_t bytes_written;

	while (true) {
		bytes_written = read(STDIN_FILENO, input_buffer.data(), input_buffer.size());
		if (bytes_written < 0) {
			myPerror("Ошибка при чтении ввода от пользователя");
			break;
		} else if (bytes_written == 0) {
			break;
		}

		if (write(pipe1[1], input_buffer.data(), bytes_written) == -1) {
			myPerror("Ошибка при записи в pipe1 в родительском процессе");
			break;
		}

		char result_buffer[1024];
		ssize_t result_read = read(pipe3[0], result_buffer, sizeof(result_buffer));
		if (result_read < 0) {
			myPerror("Ошибка при чтении из pipe3 в родительском процессе");
			break;
		} else if (result_read > 0) {
			result_buffer[result_read] = '\0';
			if (write(STDOUT_FILENO, result_buffer, strlen(result_buffer)) == -1) {
				myPerror("Ошибка при выводе в STDOUT");
				break;
			}
		}

		int status;
		if (waitpid(pid1, &status, 0) == -1) {
			myPerror("Ошибка ожидания завершения child1");
			break;
		}
		if (waitpid(pid2, &status, 0) == -1) {
			myPerror("Ошибка ожидания завершения child2");
			break;
		}
	}

	close(pipe1[1]);
	close(pipe3[0]);

	return 0;
}
