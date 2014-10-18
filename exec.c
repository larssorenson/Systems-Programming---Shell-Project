#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void main()
{
	char* args[4] = {"shell", "ls", "-al", NULL};
	int pipein[2];
	int pipeout[2];
	int stdin = dup(0);
	int stdout = dup(1);
	pipe(pipein);
	pipe(pipeout);
	int pid = fork();
	for(int x = 1; x < 3; x+=1)
		write(pipein[1], args[x], strlen(args[x]));
	write(pipein[1], "\nexit\n", 6);
	if(pid==0)
	{
		dup2(pipein[0], 0);
		dup2(pipeout[1], 1);
		execv(args[0], args);
	}
	char c = 0;
	while(read(pipeout[0], &c, 1) > 0)
	{
		printf("%c", c);
	}
	dup2(stdin, 0);
	dup2(stdout, 1);
}
