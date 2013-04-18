#ifndef _REENTRANT 
#define _REENTRANT 
#endif 

#include <sys/types.h>	/* definierar typen pid_t */
#include <errno.h>		/* definierar bland annat perror() */
#include <stdio.h>		/* definierar bland annat stderr */
#include <stdlib.h>		/* definierar bland annat rand() och RAND_MAX, getenv() */
#include <unistd.h>		/* definierar bland annat pipe() */
#include <sys/wait.h>
#include <signal.h>		/* deinierar signalnamn med mera */
#define TRUE ( 1 )		/* den logiska konstanten TRUE */

#include <sys/stat.h>	/* definierar S_IREAD och S_IWRITE */
#include <fcntl.h>		/* definierar bland annat open() och O_RDONLY */

#define PIPE_READ 0
#define PIPE_WRITE 1


pid_t parent_pid; /* f�r parentprocessen */
pid_t child_pid; /* f�r child-processens PID vid fork() */

pid_t printenv_pid = -1;
pid_t grep_pid = -1;
pid_t sort_pid = -1;
pid_t pager_pid = -1;

void printenv(int pipeline[]);
void sort(int pipeline[], int pipeline2[]);
void handlePager(int pipeline[]);
void createPipe(int pipeline[]);
void waitForChild();
void closeFileDesc(int fd);
void grep(char **argv, int pipeline1[], int pipeline2[]);
void duplicate(int from, int to);

int main(int argc, char **argv, char **envp) {	
	parent_pid = (pid_t) getpid();
	
	/* skapa en pipe 1 */
	int stageOnePipe[2];	
	createPipe(stageOnePipe);
	
	/* ------------------Printenv pipe-------------------*/		
	/* skapa en child-process */
	printenv_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"printenv (pid %ld) started\n", (long int) getpid() );
		printenv(stageOnePipe);  
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* st�ng skrivsidan av pipen */
	closeFileDesc(stageOnePipe[PIPE_WRITE] );	
	
	
	/* ------------------Grep pipe-------------------*/		
	/* Om antalet parametrar �r st�rre �n 1 s� ska ett till pipelinesteg till grep l�ggas till */
	/* skapa pipe 2 */
	int stageTwoPipe[2];	
	int useStageTwoPipe = 0; //Indikerare om sort ska l�sa fr�n stageTwoPipe eller stageOnePipe.
	if(argc > 1) {
		fprintf( stderr,"Adding grep stage\n");
		useStageTwoPipe = 1;
		createPipe(stageTwoPipe);		
		
		/* skapa en child-process */
		grep_pid = child_pid = fork();		
		if( 0 == child_pid ) {
			fprintf( stderr,"grep (pid %ld) started\n", (long int) getpid() );
			grep(argv, stageOnePipe, stageTwoPipe);
		}
		
		/* kolla om fork() misslyckades */
		if( -1 == child_pid ) {perror( "Cannot fork()" ); exit( errno );}
		
		/* st�ng on�diga fildeskriptors*/
		closeFileDesc(stageOnePipe[PIPE_READ] );
		closeFileDesc(stageTwoPipe[PIPE_WRITE] );		
	}
	
	/* skapa pipe 2 */
	int stageThreePipe[2];	
	createPipe(stageThreePipe);	
	
	/* ------------------Sort pipe-----------------------*/
	/* skapa en child-process */
	sort_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"sort (pid %ld) started\n", (long int) getpid() );
		if( 1 == useStageTwoPipe )	sort(stageTwoPipe, stageThreePipe);
		else 						sort(stageOnePipe, stageThreePipe);		
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* st�ng on�diga filedescriptors */
	if(1 == useStageTwoPipe)		closeFileDesc(stageTwoPipe[PIPE_READ] );	
	else 							closeFileDesc(stageOnePipe[PIPE_READ] );
	closeFileDesc(stageThreePipe[PIPE_WRITE]);	
	
	
	/* ------------------Pager pipe-----------------------*/
	/* skapa en child-process */
	pager_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"pager (pid %ld) started\n", (long int) getpid() );
		handlePager(stageThreePipe);
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* st�ng on�diga filedescriptors */
	closeFileDesc(stageThreePipe[PIPE_READ]);
	
	/* V�nta p� barnen */
	if( 1 == useStageTwoPipe ) //Om grep har k�rts s� m�te vi v�nta p� ett extra barn
		waitForChild();	
	waitForChild();
	waitForChild();
	waitForChild();	
	
	return 0;
}


/**
 * K�rs bara av childprocessen. Pipar utdata fr�n printenv() till sort
 * genom att duplicera pipelinens skriv�nde till stdout
 */
void printenv(int pipeline[]) {

	/* st�ng l�ssidan av pipen */
	closeFileDesc(pipeline[PIPE_READ] );	
	
	/* ers�tt stdout med duplicerad skriv-�nde p� pipen */
	duplicate( pipeline[ PIPE_WRITE ], STDOUT_FILENO );	
	
	/* st�ng l�ssidan av den dupade pipen */
	closeFileDesc(pipeline[PIPE_WRITE] );	
	
	execlp( "printenv", "printenv", (char *) 0 );
	
	/* exec returnerar bara om n�got fel har uppst�tt
	   och om exec returnerar s� �r returv�rdet alltid -1 */
	   
	perror( "Cannot exec printenv" );
	exit( errno );
}

void grep(char **argv, int readPipeline[], int writePipeline[]) {	
	
	/* st�ng on�diga pipes */
	closeFileDesc(writePipeline[PIPE_READ]);	
	
	/* S�tt ihop argumenten */
	argv[0] = "grep";		
	
	/* ers�tt stdin med duplicerad l�s-�nde p� l�spipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan st�nga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );

	/* ers�tt stdout med duplicerad skriv-�nde p� skrivpipen */
	duplicate( writePipeline[ PIPE_WRITE ], STDOUT_FILENO );	
	
	/* kan st�nga den ursprungliga pipen */
	closeFileDesc(writePipeline[PIPE_WRITE] );
	
	/* S�ker i PATH efter filnamnet och tar som andra argumenetet en pekare till en nullterminerad string-array (argumenten) */
	execvp("grep", argv);
	
	perror( "Cannot exec grep" );
	exit(errno);
}


/**
 * K�rs bara av childprocessen. L�ser indatan fr�n pipeenv() fr�n readPipeLine
 * och skriver resultatet till den filedescriptorn som pekar p� den tempor�ra filen
 */
void sort(int readPipeline[], int writePipeline[]) {
	/* kan st�nga on�diga pipes*/
	closeFileDesc(writePipeline[PIPE_READ]);	
	
	/* ers�tt stdin med duplicerad l�s-�nden p� readPipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan st�nga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );	
	
	/* ers�tt stdout med deplicerad skriv�nde p� andra pipen */
	duplicate( writePipeline[PIPE_WRITE], STDOUT_FILENO );	
	
	/* kan st�nga den ursprungliga FD */
	closeFileDesc(writePipeline[PIPE_WRITE]);	
	
	
	execlp( "sort", "sort", (char *) 0 );
	
	
	/* exec returnerar bara om n�got fel har uppst�tt
	   och om exec returnerar s� �r returv�rdet alltid -1 */
	   
	perror( "Cannot exec sort" );
	exit( errno );
}

/**
 * K�rs bara av childprocessen. Kallar p� en pager som ska ta namnet av den tempor�ra filen
 * som parameter.
 */
void handlePager(int readPipeline[]) {
	/* ers�tt stdin med duplicerad l�s-�nden p� readPipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan st�nga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );	

	/* -------------Unders�k om det finns en PAGER--------- */
	char* pager = getenv("PAGER");
	if(NULL != pager) {
		fprintf( stderr,"Found PAGER: %s\n",pager);
		execlp( pager, pager, NULL );
		
		/* N�got gick fel med pagern*/
		perror("Cannot exec pager.");
		/* Pr�va less */
		fprintf( stderr,"Trying with less\n");
	}
	else {
		fprintf( stderr,"No environment variable for PAGER\n");	
	}
	
	execlp( "less", "less", NULL );
	
	/* N�got gick fel med less, pr�va med more */
	fprintf( stderr,"Trying with more\n");
	
	execlp( "more", "more", NULL );
	
	/* N�got gick fel med more. Ingen pager fungerade: avsluta */
	perror("Cannot exec more.");		
	exit( errno );	
}

/**
 * Initierar en pipe och kollar efter errors
 */
void createPipe(int pipeline[]) {
	int return_value = pipe( pipeline );
	/* avsluta programmet om pipe() misslyckas */
	if( -1 == return_value ) 	{
		perror( "Cannot create pipe" ); exit( errno ); 
	}
}

/**
 * V�nta p� ett barn
 */
void waitForChild() {
	int status;
	
	child_pid = wait( &status );
	if( -1 == child_pid ) { 
		perror( "wait() failed unexpectedly" ); exit( errno ); 	
	}
	
	/* child-processen har k�rt klart */
	if( WIFEXITED( status ) ) {
		int child_status = WEXITSTATUS( status );
		
		/* child fick problem */
		if( 0 != child_status ) {
			/* Grep returnerar med exit(1) om den inte hittar n�gon matchning. */
			if(child_pid == grep_pid && child_status == 1) {
				fprintf( stderr,"No match found\n");
				/* Kill pager */
				kill(pager_pid,9);
			}
			/* Grep returnerar med exit(2) om den f�r konstiga parametrar. */
			else if(child_pid == grep_pid && child_status == 2) {
				fprintf( stderr,"Bad input\n");	
				kill(pager_pid,9);
			}
			/* Broken pipe */
			else if(child_status == 13) {
				fprintf( stderr,"Pipe broke\n");	
				kill(pager_pid,9);
			}
			else
				fprintf( stderr,"Child (pid %ld) failed with exit code %d\n", (long int) child_pid, child_status );
			
		}
	}
	else {
		/* child-processen avbr�ts av signal */
		if( WIFSIGNALED( status ) )	{
			int child_signal = WTERMSIG( status );
			fprintf( stderr,"Child (pid %ld) was terminated by signal no. %d\n",(long int) child_pid, child_signal );
		}
	}
}



/**
 * St�nger en fd och kollar returnv�rdet.
 */
void closeFileDesc(int fd) {
	int return_value = close( fd );
	if( -1 == return_value ) { 
		perror( "Cannot close pipe" ); exit( errno ); 		
	}
}

/**
 * Kallar p� dup2 och kollar returnv�rdet. Tar samma parametrar som dup2
 */
void duplicate(int from, int to) {
	int return_value = dup2( from, to );
	if( -1 == return_value ) { 
		perror( "Cannot dup" ); exit( 1 ); 
	}
}
