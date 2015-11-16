/* mshell - a job manager */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "pipe.h"
#include "jobs.h"
#include "cmd.h"

void do_pipe2(char *cmds[MAXCMDS][MAXARGS], int nbcmd, int bg) {
	int fds[2];
	pid_t pid;
	int state;

	state = bg ? BG : FG;

	if((pipe(fds)) == -1) {
		printf("Impossible de creer un nouveau tube\n");
	}

	switch(pid = fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, 0);
			if((dup2(fds[1], STDOUT_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la première commande\n");
			}
			close(fds[0]);
			close(fds[1]);
			execvp(cmds[0][0], cmds[0]);
			exit(EXIT_FAILURE);
	}

	switch(fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, pid);
			if((dup2(fds[0], STDIN_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la seconde commande\n");
			}
			close(fds[0]);
			close(fds[1]);
			execvp(cmds[1][0], cmds[1]);
			exit(EXIT_FAILURE);
	}

	close(fds[0]);
	close(fds[1]);

	if((jobs_addjob(pid, state, cmds[0][0])) == -1) {
		printf("Impossible d'ajouter le groupement de pid\n");
	}

	if(!bg) waitfg(pid);
}

void do_pipe3(char *cmds[MAXCMDS][MAXARGS], int nbcmd, int bg) {
	int fds1[2], fds2[2];
	int state;
	pid_t pid;

	state = bg ? BG : FG;

	if((pipe(fds1)) == -1) {
		printf("Impossible de creer un nouveau tube\n");
	}

	switch(pid = fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, 0);
			if((dup2(fds1[1], STDOUT_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la première commande\n");
			}
			close(fds1[0]);
			close(fds1[1]);
			execvp(cmds[0][0], cmds[0]);
			exit(EXIT_FAILURE);
	}

	if((pipe(fds2)) == -1) {
		printf("Impossible de creer un nouveau tube\n");
	}

	close(fds1[1]);

	switch(fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, pid);
			if((dup2(fds1[0], STDIN_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la seconde commande et premier dup2\n");
			}
			if((dup2(fds2[1], STDOUT_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la seconde commande et second dup2\n");	
			}
			close(fds1[0]);
			close(fds2[0]);
			close(fds2[1]);
			execvp(cmds[1][0], cmds[1]);
			exit(EXIT_FAILURE);
	}

	close(fds1[0]);

	switch(fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, pid);
			if((dup2(fds2[0], STDIN_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la troisieme commande\n");
			}
			close(fds2[0]);
			close(fds2[1]);
			execvp(cmds[1][0], cmds[2]);
			exit(EXIT_FAILURE);
	}

	if((jobs_addjob(pid, state, cmds[0][0])) == -1) {
		printf("Impossible d'ajouter le groupement de pid\n");
	}

	close(fds2[0]);
	close(fds2[1]);

	if(!bg) waitfg(pid);
}

void do_pipeN(char *cmds[MAXCMDS][MAXARGS], int nbcmd, int bg) {
	int fds1[2], fds2[2];
	int state, i, active_pipe;
	pid_t pid;

	state = bg ? BG : FG;

	/* Premiere commande du tube */
	if((pipe(fds1)) == -1) {
		printf("Impossible de creer un nouveau tube\n");
	}

	active_pipe = 1;

	switch(pid = fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
		case 0:
			setpgid(0, 0);
			if((dup2(fds1[1], STDOUT_FILENO)) == -1) {
				printf("Impossible de dupliquer le descripteur de fichier pour la première commande\n");
			}
			close(fds1[0]);
			close(fds1[1]);
			execvp(cmds[0][0], cmds[0]);
			exit(EXIT_FAILURE);
	}
	/* Fin premier commande du tube */

	/* Iteration sur n - 1 commande */
	for(i = 1; i < nbcmd - 1; i++) {
		if(active_pipe == 1) {
			if((pipe(fds2)) == -1) printf("Impossible de creer un nouveau tube\n");
			active_pipe = 2;

			close(fds1[1]);

			switch(fork()) {
				case -1:
					printf("Impossible de creer un fils\n");
					exit(EXIT_FAILURE);
				case 0:
					setpgid(0, pid);
					if((dup2(fds1[0], STDIN_FILENO)) == -1) {
						printf("Impossible de dupliquer le descripteur de fichier\n");
					}
					if((dup2(fds2[1], STDOUT_FILENO)) == -1) {
						printf("Impossible de dupliquer le descripteur de fichier\n");	
					}
					close(fds1[0]);
					close(fds2[0]);
					close(fds2[1]);
					execvp(cmds[i][0], cmds[i]);
					exit(EXIT_FAILURE);
			}

			close(fds1[0]);
		} else {
			if((pipe(fds1)) == -1) printf("Impossible de creer un nouveau tube\n");
			active_pipe = 1;

			close(fds2[1]);

			switch(fork()) {
				case -1:
					printf("Impossible de creer un fils\n");
					exit(EXIT_FAILURE);
				case 0:
					setpgid(0, pid);
					if((dup2(fds2[0], STDIN_FILENO)) == -1) {
						printf("Impossible de dupliquer le descripteur de fichier\n");
					}
					if((dup2(fds1[1], STDOUT_FILENO)) == -1) {
						printf("Impossible de dupliquer le descripteur de fichier\n");	
					}
					close(fds2[0]);
					close(fds1[0]);
					close(fds1[1]);
					execvp(cmds[i][0], cmds[i]);
					exit(EXIT_FAILURE);
			}

			close(fds2[0]);
		}

	}
	/* Fin Iteration sur n -1 commande */

	/* Derniere commande */
	switch(fork()) {
		case -1:
			printf("Impossible de creer un fils\n");
			exit(EXIT_FAILURE);
		case 0:
			setpgid(0, pid);
			if(active_pipe == 1) {
				if((dup2(fds1[0], STDIN_FILENO)) == -1) {
					printf("Impossible de dupliquer le descripteur\n");
				}
				close(fds1[0]);
				close(fds1[1]);
			} else {
				if((dup2(fds2[0], STDIN_FILENO)) == -1) {
					printf("Impossible de dupliquer le descripteur\n");
				}
				close(fds2[0]);
				close(fds2[1]);
			}
			execvp(cmds[1][0], cmds[2]);
			exit(EXIT_FAILURE);
	}
	/* Fin derniere commande */

	if(active_pipe == 1) {
		close(fds1[0]);
		close(fds1[1]);
	} else {
		close(fds2[0]);
		close(fds2[1]);
	}

	if((jobs_addjob(pid, state, cmds[0][0])) == -1) {
		printf("Impossible d'ajouter le groupement de pid\n");
	}

	if(!bg) waitfg(pid);
}

void do_pipe(char *cmds[MAXCMDS][MAXARGS], int nbcmd, int bg) {
    if(nbcmd == 2) do_pipe2(cmds, nbcmd, bg);
    else if(nbcmd == 3) do_pipe3(cmds, nbcmd, bg);
    else do_pipeN(cmds, nbcmd, bg);

    return;
}
